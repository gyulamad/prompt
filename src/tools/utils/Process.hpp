#pragma once

#include <string>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <mutex>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/wait.h>
#include <sys/select.h>

#include "ERROR.h"
#include "system.hpp"

#include "../datetime/get_time_ms.hpp"

using namespace std;
using namespace tools::datetime;

namespace tools::utils {
    int __pcnt = 0;

    class Pipe {
        int fd[2];
    public:
        Pipe() {
            if (pipe(fd) == -1)
                throw ERROR("Failed to create pipe");
        }
        ~Pipe() {
            close(fd[0]);
            close(fd[1]);
        }
        int read_fd() const { return fd[0]; }
        int write_fd() const { return fd[1]; }
    };

    class Process {
    private:
        Pipe to_child, from_child, err_child;
        // int to_child[2];    // Pipe for parent-to-child communication
        // int from_child[2];  // Pipe for child-to-parent communication
        // int err_child[2];   // Pipe for child's stderr
        pid_t pid;
        string program;
        string username;
        string directory;
        bool read_error;
        bool read_output;
        __useconds_t __useconds;
        size_t buffsize;
        
        void start_child_process() {
            pid = fork();
            if (pid == -1)
                throw ERROR("Failed to fork process");

            if (pid == 0) { // Child process
                close(to_child.write_fd());
                close(from_child.read_fd());
                close(err_child.read_fd());

                dup2(to_child.read_fd(), STDIN_FILENO);
                dup2(from_child.write_fd(), STDOUT_FILENO);
                dup2(err_child.write_fd(), STDERR_FILENO);

                close(to_child.read_fd());
                close(from_child.write_fd());
                close(err_child.write_fd());

                // Change to the specified directory
                if (!directory.empty()) {
                    if (chdir(directory.c_str())) {
                        perror("chdir failed");
                        exit(1);
                    }
                }

                // Change to the specified user
                if (!username.empty()) {
                    struct passwd *pw = getpwnam(username.c_str());
                    if (pw == nullptr) {
                        perror("getpwnam failed");
                        exit(1);
                    }
                    if (setgid(pw->pw_gid) || setuid(pw->pw_uid)) {
                        perror("setgid/setuid failed");
                        exit(1);
                    }
                }

                execlp(program.c_str(), program.c_str(), nullptr);
                perror("execlp failed");
                exit(1);
            } else { // Parent process
                close(to_child.read_fd());
                close(from_child.write_fd());
                close(err_child.write_fd());
            }
        }

    public:
        Process(
            const string& program = "bash", 
            const string& username = "", 
            const string& directory = "", 
            bool read_error = true, 
            bool read_output = true, 
            __useconds_t __useconds = 20000L,
            size_t buffsize = 4096
        ): 
            program(program),
            username(username),
            directory(directory),
            read_error(read_error),
            read_output(read_output),
            __useconds(__useconds),
            buffsize(buffsize)
        {
            // cout << "++[__pcnt:" <<__pcnt << "]" << endl;
            __pcnt++;
            start_child_process();
        }

        virtual ~Process() {
            kill();
            __pcnt--;
            // cout << "--[__pcnt:" <<__pcnt << "]" << endl;
        }

        // void write(const string& input) {
        //     if (::write(to_child[1], input.c_str(), input.size()) == -1)
        //         throw ERROR("Failed to write to child");
        //     usleep(__useconds);
        // }
        virtual void write(const string& input) {
            if (::write(to_child.write_fd(), input.c_str(), input.size()) == -1)
                throw ERROR("Failed to write to child");
            usleep(__useconds);
        }

        virtual void writeln(const string& input) {
            write(input + "\n");
        }

        // virtual string read_chunk(int fd) {
        //     if (buffsize > 65535) throw ERROR("Too large buffer size");
        //     char buffer[buffsize];
        //     ssize_t n = ::read(fd, buffer, sizeof(buffer) - 1);
        //     if (n == -1) {
        //         if (errno == EAGAIN || errno == EWOULDBLOCK)
        //             return ""; // No data available
        //         throw ERROR("Failed to read from child: " + string(strerror(errno)) + " - program: " + program);
        //     }
        //     buffer[n] = '\0';
        //     return string(buffer);
        // }
        virtual string read_chunk(int fd) {
            if (buffsize > 65535) throw ERROR("Too large buffer size");
            vector<char> buffer(buffsize);
            ssize_t n = ::read(fd, buffer.data(), buffer.size() - 1);
            if (n == -1) {
                #if (EAGAIN == EWOULDBLOCK)
                    if (errno == EAGAIN)
                #else
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                #endif
                    return ""; // No data available
                throw ERROR("Failed to read from child: " + string(strerror(errno)) + " - program: " + program);
            }
            buffer[n] = '\0';
            return string(buffer.data());
        }

        // string read() {
        //     string results = "";
        //     string chunk = "";

        //     while (true) {
        //         int status = ready();
        //         if (status == 0) break; // No data available

        //         if ((status & PIPE_STDOUT) && read_output) {
        //             chunk = read_chunk(from_child[0]);
        //             if (!chunk.empty()) results += chunk;
        //         }
        //         if ((status & PIPE_STDERR) && read_error) {
        //             chunk = read_chunk(err_child[0]);
        //             if (!chunk.empty()) results += chunk;
        //         }
        //         if (chunk.empty()) break; // No more data
        //     }

        //     return results;
        // }
        // non-blocking read but add timeout for blocking read
        virtual string read(int timeout_ms = 0) {
            string results = "";
            string chunk = "";

            long long read_start_ms = get_time_ms();

            while (true) {
                int status = ready();
                if (status == 0) { // No data available
                    if (!timeout_ms) break; // non-blocking read
                    
                    // blocking read with timeout
                    long long now_ms = get_time_ms();
                    if (now_ms > read_start_ms + timeout_ms) break; // timeout interrupts
                
                    continue;
                }
                

                if ((status & PIPE_STDOUT) && read_output) {
                    chunk = read_chunk(from_child.read_fd());
                    if (!chunk.empty()) results += chunk;
                }
                if ((status & PIPE_STDERR) && read_error) {
                    chunk = read_chunk(err_child.read_fd());
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

        // int ready() {
        //     usleep(__useconds);

        //     fd_set readfds;
        //     FD_ZERO(&readfds);
        //     if (read_output) FD_SET(from_child[0], &readfds);
        //     if (read_error) FD_SET(err_child[0], &readfds);

        //     struct timeval timeout = {0, 0}; // Non-blocking check

        //     int result = select(max(from_child[0], err_child[0]) + 1, &readfds, nullptr, nullptr, &timeout);
        //     if (result == -1)
        //         throw ERROR("Failed to check readiness with select (non-blocking)");

        //     int status = 0;
        //     if (read_output && FD_ISSET(from_child[0], &readfds)) status |= PIPE_STDOUT;
        //     if (read_error && FD_ISSET(err_child[0], &readfds)) status |= PIPE_STDERR;
        //     return status;
        // }
        virtual int ready() {
            usleep(__useconds);

            fd_set read_fds;
            FD_ZERO(&read_fds);
            if (read_output) FD_SET(from_child.read_fd(), &read_fds);
            if (read_error) FD_SET(err_child.read_fd(), &read_fds);

            struct timeval timeout = {0, 0}; // Non-blocking check
            int max_fd = max(from_child.read_fd(), err_child.read_fd());

            int result = select(max_fd + 1, &read_fds, nullptr, nullptr, &timeout);
            if (result == -1) {
                throw ERROR("select failed");
            }

            int status = 0;
            if (read_output && FD_ISSET(from_child.read_fd(), &read_fds)) {
                status |= PIPE_STDOUT;
            }
            if (read_error && FD_ISSET(err_child.read_fd(), &read_fds)) {
                status |= PIPE_STDERR;
            }

            return status;
        }

        // void kill() {
        //     if (pid > 0) {
        //         ::kill(pid, SIGKILL);
        //         waitpid(pid, nullptr, 0);
        //         pid = -1;
        //     }
        // }
        virtual void kill() {
            if (pid > 0) {
                if (::kill(pid, SIGTERM) == -1) {
                    perror("kill(SIGTERM) failed");
                } else {
                    int status;
                    if (waitpid(pid, &status, WNOHANG) == 0) {
                        sleep(1);
                        if (waitpid(pid, &status, WNOHANG) == 0) {
                            if (::kill(pid, SIGKILL) == -1) {
                                perror("kill(SIGKILL) failed");
                            }
                            waitpid(pid, nullptr, 0);
                        }
                    }
                }
                pid = -1;
            }
        }

        virtual void reset() {
            kill();                // Clean up the current process and resources
            start_child_process();    // Restart the process
        }


        // for blocking executions:

        // string read_blocking() {
        //     string results = "";
        //     string chunk = "";

        //     while (true) {
        //         int status = ready_blocking(); // Use a blocking version of ready()
        //         if (status == 0) break; // No data available (shouldn't happen in blocking mode)

        //         if ((status & PIPE_STDOUT) && read_output) {
        //             chunk = read_chunk(from_child[0]);
        //             if (!chunk.empty()) results += chunk;
        //         }
        //         if ((status & PIPE_STDERR) && read_error) {
        //             chunk = read_chunk(err_child[0]);
        //             if (!chunk.empty()) results += chunk;
        //         }
        //         if (chunk.empty()) break; // No more data
        //     }

        //     return results;
        // }
        virtual string read_blocking(int timeout = 10) {
            string results = "";
            string chunk = "";

            auto start_time = chrono::steady_clock::now();
            while (true) {
                // Check if the timeout has been reached
                auto current_time = chrono::steady_clock::now();
                auto elapsed_time = chrono::duration_cast<chrono::seconds>(current_time - start_time).count();
                if (elapsed_time >= timeout) {
                    results += "\nExecution timed out after " + to_string(timeout) + "s"; // Timeout reached
                    break;
                }

                int status = ready_blocking(); // Use a blocking version of ready()
                if (status == 0) break; // No data available (shouldn't happen in blocking mode)

                if ((status & PIPE_STDOUT) && read_output) {
                    chunk = read_chunk(from_child.read_fd());
                    if (!chunk.empty()) results += chunk;
                }
                if ((status & PIPE_STDERR) && read_error) {
                    chunk = read_chunk(err_child.read_fd());
                    if (!chunk.empty()) results += chunk;
                }
                if (chunk.empty()) break; // No more data
            }

            return results;
        }

        // int ready_blocking() {
        //     fd_set readfds;
        //     FD_ZERO(&readfds);
        //     if (read_output) FD_SET(from_child[0], &readfds);
        //     if (read_error) FD_SET(err_child[0], &readfds);

        //     // Block indefinitely until data is available
        //     int result = select(max(from_child[0], err_child[0]) + 1, &readfds, nullptr, nullptr, nullptr);
        //     if (result == -1)
        //         throw ERROR("Failed to check readiness with select (blocking)");

        //     int status = 0;
        //     if (read_output && FD_ISSET(from_child[0], &readfds)) status |= PIPE_STDOUT;
        //     if (read_error && FD_ISSET(err_child[0], &readfds)) status |= PIPE_STDERR;
        //     return status;
        // }
        virtual int ready_blocking() {
            fd_set readfds;
            FD_ZERO(&readfds);
            if (read_output) FD_SET(from_child.read_fd(), &readfds);
            if (read_error) FD_SET(err_child.read_fd(), &readfds);

            // Block indefinitely until data is available
            int result = select(max(from_child.read_fd(), err_child.read_fd()) + 1, &readfds, nullptr, nullptr, nullptr);
            if (result == -1)
                throw ERROR("Failed to check readiness with select (blocking)");

            int status = 0;
            if (read_output && FD_ISSET(from_child.read_fd(), &readfds)) status |= PIPE_STDOUT;
            if (read_error && FD_ISSET(err_child.read_fd(), &readfds)) status |= PIPE_STDERR;
            return status;
        }

        // static string execute(const string& command, const string& program = "bash") {
        //     Process process(program); // Create a new Process instance
        //     process.writeln(command); // Write the command
        //     close(process.to_child[1]); // Close the write end to signal EOF
        //     return process.read_blocking(); // Read the output
        // }
        static mutex cout_mutex; // Mutex for synchronizing cout

        static string execute(const string& command, const string& program = "bash", bool show = false, int timeout = 10) {
            static mutex process_mutex; // Mutex for Process class (if needed)
            lock_guard<mutex> lock(process_mutex); // Lock for Process class

            Process process(program); // Create a new Process instance
            process.writeln(command); // Write the command
            close(process.to_child.write_fd()); // Close the write end to signal EOF
            string output = "";
            if (!show) output = process.read_blocking(timeout); // Read the output
            else {
                string outp = process.read();
                output += outp;
                {
                    lock_guard<mutex> cout_lock(cout_mutex); // Lock for cout
                    cout << outp << flush;
                }
            }
            process.kill();
            return output;
        }

    };
    mutex Process::cout_mutex;

}