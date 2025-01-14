#pragma once

#include <string>
#include <sstream>
#include <array>
#include <memory>

#include "ERROR.hpp"

using namespace std;

class Shell {
private:
    int timeout_seconds = -1; // Timeout for commands
    string last_error;
    int last_exit_code = 0;

public:
    // Constructor
    Shell() = default;

    // Set the timeout for commands
    void timeout(int seconds) {
        timeout_seconds = seconds;
    }

    // Execute a command
    string exec(const string& cmd, bool throws = false) {
        last_error.clear(); // Clear previous error
        ostringstream result;
        array<char, 128> buffer;

        // Add timeout support by wrapping the command with a timeout mechanism
        string command = cmd;
        if (timeout_seconds > 0) {
            command = cmd; // "timeout " + to_string(timeout_seconds) + "s " + cmd;
        }

        // Use an explicit function pointer for pclose to avoid warnings
        using PipeCloser = int (*)(FILE*);
        unique_ptr<FILE, PipeCloser> pipe(popen(command.c_str(), "r"), pclose);

        if (!pipe) {
            last_error = "Failed to open pipe for command execution."; // Command was:\n" + cmd;
            last_exit_code = -1;
            const string errmsg = last_error;
            throw ::ERROR(last_error);
        }

        // Read the output from the pipe
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result << buffer.data();
        }

        // Get the exit status of the command
        int status = pclose(pipe.release());
        last_exit_code = WEXITSTATUS(status);

        if (last_exit_code != 0) {
            last_error = "Command failed with exit code " + to_string(last_exit_code);
        }

        if (throws && !last_error.empty()) {
            throw ::ERROR(last_error);
        }

        return result.str();
    }

    // Get the last error
    string error() const {
        return last_error;
    }

    // Get the last exit code
    int result() const {
        return last_exit_code;
    }
};
