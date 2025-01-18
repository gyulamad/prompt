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

class Proc {
private:
    int to_child[2];
    int from_child[2];
    pid_t pid;
    string program;

    void cleanup() {
        if (pid > 0) {
            kill(); // Terminate the child process if it's running
        }
        close(to_child[1]);
        close(from_child[0]);
    }

    void start_child_process() {
        if (pipe(to_child) == -1 || pipe(from_child) == -1)
            throw runtime_error("Failed to create pipes");

        pid = fork();
        if (pid == -1)
            throw runtime_error("Failed to fork process");

        if (pid == 0) { // Child process
            close(to_child[1]);
            close(from_child[0]);

            dup2(to_child[0], STDIN_FILENO);   // Redirect stdin
            dup2(from_child[1], STDOUT_FILENO); // Redirect stdout
            dup2(from_child[1], STDERR_FILENO); // Redirect stderr to stdout

            close(to_child[0]);
            close(from_child[1]);

            execlp(program.c_str(), program.c_str(), nullptr);
            perror("execlp failed");
            exit(1);
        } else { // Parent process
            close(to_child[0]);
            close(from_child[1]);
        }
    }

public:
    Proc(const string& program) : program(program) {
        start_child_process();
    }

    ~Proc() {
        cleanup();
    }

    void write(const string& input, __useconds_t __useconds = 20000L) {
        if (::write(to_child[1], input.c_str(), input.size()) == -1)
            throw runtime_error("Failed to write to child");
        usleep(__useconds);
    }

    void writeln(const string& input, __useconds_t __useconds = 20000L) {
        write(input + "\n", __useconds);
    }

    string read_chunk(const size_t buffsize = 4096) {
        char buffer[buffsize];
        ssize_t n = ::read(from_child[0], buffer, sizeof(buffer) - 1);
        if (n == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return ""; // No data available
            throw runtime_error("Failed to read from child");
        }
        buffer[n] = '\0';
        return string(buffer);
    }

    string read(__useconds_t __useconds = 20000L) {
        string results = "";
        string chunk = "";
        while (ready(__useconds) && !(chunk = read_chunk()).empty()) results += chunk;
        return results;
    }

    bool ready(__useconds_t __useconds = 20000L) {
        usleep(__useconds);

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(from_child[0], &readfds);

        struct timeval timeout = {0, 0}; // Non-blocking check

        int result = select(from_child[0] + 1, &readfds, nullptr, nullptr, &timeout);
        if (result == -1)
            throw runtime_error("Failed to check readiness with select");
        return result > 0; // Returns true if data is available
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
};

