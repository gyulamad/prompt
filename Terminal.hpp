#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>
#include <fcntl.h>
#include <signal.h>
#include <poll.h>
#include <errno.h>

using namespace std;

class Terminal {
public:
    virtual string read() = 0;         // read until a next line (or empty string)
    virtual char getc() = 0;           // read the next char (or '\0')
    virtual void write(string) = 0;    // writes to the output a string
    virtual void writeln(string) = 0;  // writes to the output a string and a new line
    virtual void putc(char) = 0;       // writes a character
    virtual ~Terminal() = default;
};

// New base class to handle common terminal functionality
class TerminalBase : public Terminal {
protected:
    termios original_termios;
    bool terminal_saved = false;
    
    void save_terminal_settings() {
        if (tcgetattr(STDIN_FILENO, &original_termios) == 0) {
            terminal_saved = true;
        }
    }

    void restore_terminal() {
        if (terminal_saved) {
            tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
            terminal_saved = false;
        }
    }

    void setup_terminal(bool non_blocking = false) {
        save_terminal_settings();
        
        // Set up raw mode
        termios raw = original_termios;
        raw.c_iflag &= ~(ICRNL | IXON);
        raw.c_lflag &= ~(ECHO | ICANON | ISIG);
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);

        if (non_blocking) {
            int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
            fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
        }
    }

    virtual ~TerminalBase() {
        restore_terminal();
    }
};

class TerminalEmulator: public TerminalBase {
private:
    static volatile sig_atomic_t g_running;
    pid_t child_pid;
    int master_fd;
    bool is_running;
    thread io_thread;
    mutex buffer_mutex;
    string accumulated_buffer;
    queue<char> char_buffer;
    chrono::microseconds poll_interval{10000};  // 10ms default
    
    static void signal_handler(int sig) {
        g_running = 0;
    }
    
    // void save_terminal_settings() {
    //     if (tcgetattr(STDIN_FILENO, &original_termios) == 0) {
    //         terminal_saved = true;
    //     }
    // }

    // void restore_terminal() {
    //     if (terminal_saved) {
    //         tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
    //         terminal_saved = false;
    //     }
    // }

    // void setup_terminal() {
    //     save_terminal_settings();
        
    //     termios raw = original_termios;
    //     raw.c_iflag &= ~(ICRNL | IXON);
    //     raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    //     tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    // }

    void io_loop() {
        vector<char> buffer(4096);
        struct pollfd fds[1] = {
            {master_fd, POLLIN, 0}
        };

        while (is_running && g_running) {
            int ret = poll(fds, 1, 100);
            if (ret < 0 && errno != EINTR) break;
            if (ret <= 0) continue;

            if (fds[0].revents & POLLIN) {
                ssize_t n = ::read(master_fd, buffer.data(), buffer.size());
                if (n <= 0) break;
                
                lock_guard<mutex> lock(buffer_mutex);
                accumulated_buffer += string(buffer.data(), n);
                for (size_t i = 0; i < n; i++) {
                    char_buffer.push(buffer[i]);
                }
            }

            if (fds[0].revents & (POLLHUP | POLLERR)) {
                break;
            }
        }
    }

public:
    TerminalEmulator() :
        child_pid(-1), 
        master_fd(-1), 
        is_running(false)//,
        // terminal_saved(false) 
    {
        struct sigaction sa;
        sa.sa_handler = signal_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGINT, &sa, nullptr);
        sigaction(SIGTERM, &sa, nullptr);
    }

    bool start(const string& command = "bash") {
        setup_terminal();

        master_fd = posix_openpt(O_RDWR);
        if (master_fd < 0) return false;

        if (grantpt(master_fd) < 0 || unlockpt(master_fd) < 0) {
            close(master_fd);
            return false;
        }

        int slave_fd = open(ptsname(master_fd), O_RDWR);
        if (slave_fd < 0) {
            close(master_fd);
            return false;
        }

        winsize ws;
        if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) != -1) {
            ioctl(slave_fd, TIOCSWINSZ, &ws);
        }

        child_pid = fork();
        if (child_pid == -1) {
            close(master_fd);
            close(slave_fd);
            return false;
        }

        if (child_pid == 0) {
            close(master_fd);
            
            setsid();
            ioctl(slave_fd, TIOCSCTTY, 0);

            dup2(slave_fd, STDIN_FILENO);
            dup2(slave_fd, STDOUT_FILENO);
            dup2(slave_fd, STDERR_FILENO);
            close(slave_fd);

            execl("/bin/sh", "sh", "-c", command.c_str(), nullptr);
            _exit(1);
        }

        close(slave_fd);
        is_running = true;
        io_thread = thread(&TerminalEmulator::io_loop, this);
        
        return true;
    }

    // Terminal interface implementation
    string read() override {
        lock_guard<mutex> lock(buffer_mutex);
        if (accumulated_buffer.empty()) {
            return "";
        }
        string result = accumulated_buffer;
        accumulated_buffer.clear();
        return result;
    }

    char getc() override {
        lock_guard<mutex> lock(buffer_mutex);
        if (char_buffer.empty()) {
            return '\0';
        }
        char c = char_buffer.front();
        char_buffer.pop();
        return c;
    }

    void write(string input) override {
        if (is_running) {
            ::write(master_fd, input.c_str(), input.length());
        }
    }

    void writeln(string input) override {
        if (is_running) {
            input += '\n';
            ::write(master_fd, input.c_str(), input.length());
        }
    }

    void putc(char c) override {
        if (is_running) {
            ::write(master_fd, &c, 1);
        }
    }

    bool update() {
        if (!is_alive() || !g_running) {
            return false;
        }
        this_thread::sleep_for(poll_interval);
        return true;
    }

    void set_poll_interval(chrono::microseconds interval) {
        poll_interval = interval;
    }

    bool is_alive() const {
        if (child_pid <= 0) return false;
        int status;
        pid_t result = waitpid(child_pid, &status, WNOHANG);
        return result == 0;
    }

    ~TerminalEmulator() override {
        is_running = false;
        g_running = 0;

        if (io_thread.joinable()) {
            io_thread.join();
        }
        
        if (child_pid > 0) {
            kill(child_pid, SIGTERM);
            int status;
            waitpid(child_pid, &status, 0);
        }
        
        if (master_fd >= 0) {
            close(master_fd);
        }

        restore_terminal();
    }
};

// Initialize static member
volatile sig_atomic_t TerminalEmulator::g_running = 1;

class TerminalIO: public TerminalBase {
private:
    // termios original_termios;
    // bool terminal_saved = false;
    queue<char> input_buffer;
    mutex buffer_mutex;

    // void save_terminal_settings() {
    //     if (tcgetattr(STDIN_FILENO, &original_termios) == 0) {
    //         terminal_saved = true;
    //     }
    // }

    // void restore_terminal() {
    //     if (terminal_saved) {
    //         // Restore terminal settings
    //         tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
    //         // Restore blocking mode
    //         int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    //         fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);
    //         terminal_saved = false;
    //     }
    // }

    // void setup_terminal() {
    //     save_terminal_settings();
        
    //     // Set up non-blocking input
    //     int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    //     fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
        
    //     // Set up raw mode
    //     termios raw = original_termios;
    //     raw.c_iflag &= ~(ICRNL | IXON);
    //     raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    //     tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    // }

public:
    TerminalIO() {
        setup_terminal(true);
    }

    string read() override {
        char buffer[256];
        string result;
        ssize_t nread = ::read(STDIN_FILENO, buffer, sizeof(buffer));
        if (nread > 0) {
            result = string(buffer, nread);
        }
        return result;
    }

    char getc() override {
        char c;
        ssize_t nread = ::read(STDIN_FILENO, &c, 1);
        return (nread == 1) ? c : '\0';
    }

    void write(string text) override {
        cout << text;
        cout.flush();
    }

    void writeln(string text) override {
        cout << text << endl;
    }

    void putc(char c) override {
        cout << c;
        cout.flush();
    }

    ~TerminalIO() override {
        restore_terminal();
    }
};
