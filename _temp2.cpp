#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <fcntl.h>
#include <signal.h>
#include <poll.h>

class TerminalEmulator {
private:
    termios original_termios;
    pid_t child_pid;
    int master_fd;

    void restore_terminal() {
        tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
    }

    void setup_terminal() {
        tcgetattr(STDIN_FILENO, &original_termios);
        termios raw = original_termios;
        raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
        raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
        raw.c_oflag &= ~(OPOST);
        raw.c_cflag &= ~(CSIZE | PARENB);
        raw.c_cflag |= CS8;
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    }

public:
    int run(const std::string& command) {
        setup_terminal();

        // Open pseudo-terminal
        master_fd = posix_openpt(O_RDWR | O_NOCTTY);
        if (master_fd < 0) {
            std::cerr << "Failed to open pseudo-terminal" << std::endl;
            return 1;
        }

        grantpt(master_fd);
        unlockpt(master_fd);
        int slave_fd = open(ptsname(master_fd), O_RDWR);

        // Get terminal size
        winsize ws;
        if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) != -1) {
            ioctl(slave_fd, TIOCSWINSZ, &ws);
        }

        child_pid = fork();
        if (child_pid == 0) {
            close(master_fd);
            
            // Setup slave as controlling terminal
            setsid();
            ioctl(slave_fd, TIOCSCTTY, 0);

            // Redirect standard streams
            dup2(slave_fd, STDIN_FILENO);
            dup2(slave_fd, STDOUT_FILENO);
            dup2(slave_fd, STDERR_FILENO);
            close(slave_fd);

            // Execute command
            execl("/bin/sh", "sh", "-c", command.c_str(), nullptr);
            exit(1);
        }

        close(slave_fd);

        std::vector<char> buffer(4096);
        struct pollfd fds[2] = {
            {STDIN_FILENO, POLLIN, 0},
            {master_fd, POLLIN, 0}
        };

        while (true) {
            if (poll(fds, 2, -1) > 0) {
                // Handle input from terminal
                if (fds[0].revents & POLLIN) {
                    ssize_t n = read(STDIN_FILENO, buffer.data(), buffer.size());
                    if (n <= 0) break;
                    write(master_fd, buffer.data(), n);
                }

                // Handle output from process
                if (fds[1].revents & POLLIN) {
                    ssize_t n = read(master_fd, buffer.data(), buffer.size());
                    if (n <= 0) break;
                    write(STDOUT_FILENO, buffer.data(), n);
                }

                if (fds[0].revents & (POLLHUP | POLLERR) ||
                    fds[1].revents & (POLLHUP | POLLERR)) {
                    break;
                }
            }
        }

        restore_terminal();
        return 0;
    }

    ~TerminalEmulator() {
        restore_terminal();
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <command>" << std::endl;
        return 1;
    }

    std::string command;
    for (int i = 1; i < argc; i++) {
        if (i > 1) command += " ";
        command += argv[i];
    }

    TerminalEmulator emulator;
    return emulator.run(command);
}