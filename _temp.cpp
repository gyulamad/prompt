#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <pty.h>
#include <cstring>
#include <errno.h>
#include <thread>
#include <atomic>

#include "ERROR.hpp"

using namespace std;

class PTY {
private:
    int master_fd;
    pid_t child_pid;
    string error_buffer;

public:
    PTY() : master_fd(-1), child_pid(-1) {
        int slave_fd;
        if (openpty(&master_fd, &slave_fd, nullptr, nullptr, nullptr) == -1) 
            throw ERROR("Failed to open pty");

        child_pid = fork();
        if (child_pid == -1) {
            throw ERROR("Failed to fork process");
        } else if (child_pid == 0) { // Child process
            close(master_fd);

            // Create a new session and set the slave as the controlling terminal
            if (setsid() == -1) 
                exit(EXIT_FAILURE);
            if (ioctl(slave_fd, TIOCSCTTY, 0) == -1) 
                exit(EXIT_FAILURE);

            // Redirect stdin, stdout, and stderr to the slave
            dup2(slave_fd, STDIN_FILENO);
            dup2(slave_fd, STDOUT_FILENO);
            dup2(slave_fd, STDERR_FILENO);

            close(slave_fd);

            execl("/bin/bash", "/bin/bash", nullptr); // Replace with desired shell
            exit(EXIT_FAILURE); // If exec fails
        } else { // Parent process
            close(slave_fd);
            
            // Set non blocking
            int flags = fcntl(master_fd, F_GETFL, 0);
            if (flags == -1) 
                throw ERROR("Failed to get flags for fd");
            if (fcntl(master_fd, F_SETFL, flags | O_NONBLOCK) == -1) 
                throw ERROR("Failed to set non-blocking mode for fd");
        }
    }

    ~PTY() {
        if (master_fd != -1) close(master_fd);
        if (child_pid > 0) {
            kill(child_pid, SIGKILL);
            waitpid(child_pid, nullptr, 0);
        }
    }

    void send(const string &input) {
        if (write(master_fd, input.c_str(), input.size()) == -1) 
            throw ERROR("Failed to write to pty");
    }

    void sendKey(char key) {
        if (write(master_fd, &key, 1) == -1) 
            throw ERROR("Failed to write key to pty");
    }

    string read() {
        char buffer[1024];
        ssize_t bytes_read = ::read(master_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return ""; // Non-blocking mode
            throw ERROR("Error reading from pty");
        } else if (bytes_read == 0) {
            return ""; // End of file
        }
        buffer[bytes_read] = '\0';
        return string(buffer);
    }

    int result() {
        int status;
        if (waitpid(child_pid, &status, WNOHANG) == 0) return -1; // Still running
        return WEXITSTATUS(status);
    }

    static void readFromPTY(PTY &pty, atomic<bool> &running) {
        while (running) {
            string output = pty.read();
            if (!output.empty()) cout << output << flush;
            usleep(10000); // Avoid busy-waiting
        }
    }
};

class Terminal {
private:
    PTY pty;
    atomic<bool> running = atomic<bool>(true);
    thread reader = thread(PTY::readFromPTY, ref(pty), ref(running)); // Start a thread to read output from the PTY
    termios original_termios, raw_termios;
public:
    Terminal() {
        // Switch terminal to raw mode to capture individual key presses
        tcgetattr(STDIN_FILENO, &original_termios);
        raw_termios = original_termios;
        raw_termios.c_lflag &= ~(ECHO | ICANON); // Disable echo and canonical mode
        tcsetattr(STDIN_FILENO, TCSANOW, &raw_termios);
    }

    ~Terminal() {
        // Restore original terminal settings
        tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);

        // Stop the reader thread and wait for it
        running = false;
        reader.join();
    }

    void sendKey(char key) {
        pty.sendKey(key);
    }
};

void test_PTY() {
    Terminal terminal;

    try {
        char key;
        while (true) {
            if (read(STDIN_FILENO, &key, 1) > 0) {
                //if (key == '\x04') break; // Exit on Ctrl+D
                terminal.sendKey(key);
            }
        }
    } catch (const exception &e) {
        cerr << "Error: " << e.what() << endl;
    }
}

int main() {
    try {
        test_PTY();
    } catch (const exception &e) {
        cerr << "Error: " << e.what() << endl;
    }
    return 0;
}
