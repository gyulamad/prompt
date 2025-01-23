#pragma once

#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <stdexcept>
#include <string>
#include <sys/select.h>
#include <cstring>
#include <fcntl.h>

using namespace std;

namespace tools {

    class Process {
    private:
        int to_child[2];    // Pipe for parent-to-child communication
        int from_child[2];  // Pipe for child-to-parent communication
        int err_child[2];   // Pipe for child's stderr
        pid_t pid;
        string program;
        bool read_error;
        bool read_output;
        __useconds_t __useconds;
        size_t buffsize;

        void cleanup() {
            if (pid > 0) {
                kill(); // Terminate the child process if it's running
            }
            close(to_child[0]);
            close(to_child[1]);
            close(from_child[0]);
            close(from_child[1]);
            close(err_child[0]);
            close(err_child[1]);
        }

        void start_child_process() {
            if (pipe(to_child) == -1 || pipe(from_child) == -1 || pipe(err_child) == -1)
                throw runtime_error("Failed to create pipes");

            pid = fork();
            if (pid == -1)
                throw runtime_error("Failed to fork process");

            if (pid == 0) { // Child process
                close(to_child[1]);
                close(from_child[0]);
                close(err_child[0]);

                dup2(to_child[0], STDIN_FILENO);   // Redirect stdin
                dup2(from_child[1], STDOUT_FILENO); // Redirect stdout
                dup2(err_child[1], STDERR_FILENO); // Redirect stderr

                close(to_child[0]);
                close(from_child[1]);
                close(err_child[1]);

                execlp(program.c_str(), program.c_str(), nullptr);
                perror("execlp failed");
                exit(1);
            } else { // Parent process
                close(to_child[0]);
                close(from_child[1]);
                close(err_child[1]);

                // Set pipes to non-blocking mode
                // fcntl(from_child[0], F_SETFL, O_NONBLOCK);
                // fcntl(err_child[0], F_SETFL, O_NONBLOCK);
            }
        }

    public:
        Process(
            const string& program = "bash", 
            bool read_error = true, 
            bool read_output = true, 
            __useconds_t __useconds = 20000L,
            size_t buffsize = 4096
        ): 
            program(program),
            read_error(read_error),
            read_output(read_output),
            __useconds(__useconds),
            buffsize(buffsize)
        {
            start_child_process();
        }

        ~Process() {
            cleanup();
        }

        void write(const string& input) {
            if (::write(to_child[1], input.c_str(), input.size()) == -1)
                throw runtime_error("Failed to write to child");
            usleep(__useconds);
        }

        void writeln(const string& input) {
            write(input + "\n");
        }

        string read_chunk(int fd) {
            char buffer[buffsize];
            ssize_t n = ::read(fd, buffer, sizeof(buffer) - 1);
            if (n == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    return ""; // No data available
                throw runtime_error("Failed to read from child, program: " + program);
            }
            buffer[n] = '\0';
            return string(buffer);
        }

        string read() {
            string results = "";
            string chunk = "";

            while (true) {
                int status = ready();
                if (status == 0) break; // No data available

                if ((status & PIPE_STDOUT) && read_output) {
                    chunk = read_chunk(from_child[0]);
                    if (!chunk.empty()) results += chunk;
                }
                if ((status & PIPE_STDERR) && read_error) {
                    chunk = read_chunk(err_child[0]);
                    if (!chunk.empty()) results += chunk;
                }
                if (chunk.empty()) break; // No more data
            }

            return results;
        }

        enum PipeStatus {
            PIPE_STDOUT = 1 << 0, // Bit 0: stdout has data
            PIPE_STDERR = 1 << 1  // Bit 1: stderr has data
        };

        int ready() {
            usleep(__useconds);

            fd_set readfds;
            FD_ZERO(&readfds);
            if (read_output) FD_SET(from_child[0], &readfds);
            if (read_error) FD_SET(err_child[0], &readfds);

            struct timeval timeout = {0, 0}; // Non-blocking check

            int result = select(max(from_child[0], err_child[0]) + 1, &readfds, nullptr, nullptr, &timeout);
            if (result == -1)
                throw runtime_error("Failed to check readiness with select (non-blocking)");

            int status = 0;
            if (read_output && FD_ISSET(from_child[0], &readfds)) status |= PIPE_STDOUT;
            if (read_error && FD_ISSET(err_child[0], &readfds)) status |= PIPE_STDERR;
            return status;
        }

        void kill() {
            if (pid > 0) {
                ::kill(pid, SIGKILL);
                waitpid(pid, nullptr, 0);
                pid = -1;
            }
        }

        void reset() {
            cleanup();                // Clean up the current process and resources
            start_child_process();    // Restart the process
        }


        // for blocking executions:

        string read_blocking() {
            string results = "";
            string chunk = "";

            while (true) {
                int status = ready_blocking(); // Use a blocking version of ready()
                if (status == 0) break; // No data available (shouldn't happen in blocking mode)

                if ((status & PIPE_STDOUT) && read_output) {
                    chunk = read_chunk(from_child[0]);
                    if (!chunk.empty()) results += chunk;
                }
                if ((status & PIPE_STDERR) && read_error) {
                    chunk = read_chunk(err_child[0]);
                    if (!chunk.empty()) results += chunk;
                }
                if (chunk.empty()) break; // No more data
            }

            return results;
        }

        int ready_blocking() {
            fd_set readfds;
            FD_ZERO(&readfds);
            if (read_output) FD_SET(from_child[0], &readfds);
            if (read_error) FD_SET(err_child[0], &readfds);

            // Block indefinitely until data is available
            int result = select(max(from_child[0], err_child[0]) + 1, &readfds, nullptr, nullptr, nullptr);
            if (result == -1)
                throw runtime_error("Failed to check readiness with select (blocking)");

            int status = 0;
            if (read_output && FD_ISSET(from_child[0], &readfds)) status |= PIPE_STDOUT;
            if (read_error && FD_ISSET(err_child[0], &readfds)) status |= PIPE_STDERR;
            return status;
        }

        static string execute(const string& command, const string& program = "bash") {
            Process process(program); // Create a new Process instance
            process.writeln(command); // Write the command
            close(process.to_child[1]); // Close the write end to signal EOF
            return process.read_blocking(); // Read the output
        }

    };

}