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

#include "ERROR.hpp"

using namespace std;
// TODO: this file in YAGNI and can be removed
class Terminal {
public:
    virtual string read() = 0;         // read until a next line (or empty string)
    virtual char getc() = 0;           // read the next char (or '\0')
    virtual void write(string) = 0;    // writes to the output a string
    virtual void writeln(string) = 0;  // writes to the output a string and a new line
    virtual void putc(char) = 0;       // writes a character
    virtual ~Terminal() = default;
};

// Base class to handle common terminal functionality
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

class SignalHandler {
private:
    struct sigaction old_int;
    struct sigaction old_term;
    volatile sig_atomic_t* running_flag;  // Pointer instead of reference

    static void handler(int sig) {
        // We'll set this up in the constructor
        if (instance) {
            *(instance->running_flag) = 0;
        }
    }

    static SignalHandler* instance;  // For handler access

public:
    SignalHandler(volatile sig_atomic_t* flag) : running_flag(flag) {
        instance = this;  // Set instance for static handler
        
        struct sigaction sa;
        sa.sa_handler = &SignalHandler::handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        
        // Save old handlers and set new ones
        sigaction(SIGINT, &sa, &old_int);
        sigaction(SIGTERM, &sa, &old_term);
    }

    ~SignalHandler() {
        // Restore original signal handlers
        sigaction(SIGINT, &old_int, nullptr);
        sigaction(SIGTERM, &old_term, nullptr);
        if (instance == this) {
            instance = nullptr;
        }
    }

    // Prevent copying
    SignalHandler(const SignalHandler&) = delete;
    SignalHandler& operator=(const SignalHandler&) = delete;
};

// Define the static member
SignalHandler* SignalHandler::instance = nullptr;

class TerminalEmulator: public TerminalBase {
private:
    static volatile sig_atomic_t g_running;
    SignalHandler signal_handler;
    static constexpr chrono::microseconds POLL_INTERVAL{10000};
    pid_t child_pid;
    int master_fd;
    bool is_running;
    thread io_thread;
    mutex buffer_mutex;
    string buffer;

    void io_loop() {
        vector<char> temp_buffer(4096);
        struct pollfd fds[1] = {
            {master_fd, POLLIN, 0}
        };

        while (is_running && g_running) {
            int ret = poll(fds, 1, 100);
            if (ret < 0 && errno != EINTR) break;
            if (ret <= 0) continue;

            if (fds[0].revents & POLLIN) {
                ssize_t n = ::read(master_fd, temp_buffer.data(), temp_buffer.size());
                if (n <= 0) break;
                
                lock_guard<mutex> lock(buffer_mutex);
                buffer.append(temp_buffer.data(), n);
            }

            if (fds[0].revents & (POLLHUP | POLLERR)) {
                break;
            }
        }
    }

public:
    TerminalEmulator(const string& command = "bash") : 
        signal_handler(&g_running),
        child_pid(-1), 
        master_fd(-1), 
        is_running(false) 
    {
        setup_terminal();

        master_fd = posix_openpt(O_RDWR);
        if (master_fd < 0) 
            throw ERROR("//TODO add proper error message1");

        if (grantpt(master_fd) < 0 || unlockpt(master_fd) < 0) {
            close(master_fd);
            throw ERROR("//TODO add proper error message2");
        }

        int slave_fd = open(ptsname(master_fd), O_RDWR);
        if (slave_fd < 0) {
            close(master_fd);
            throw ERROR("//TODO add proper error message3");
        }

        winsize ws;
        if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) != -1) {
            ioctl(slave_fd, TIOCSWINSZ, &ws);
        }

        child_pid = fork();
        if (child_pid == -1) {
            close(master_fd);
            close(slave_fd);
            throw ERROR("//TODO add proper error message4");
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
    }

    // Terminal interface implementation
    string read() override { // TODO: implement reading with blocking mode for e.g. readln(string token = "\n") ...
        lock_guard<mutex> lock(buffer_mutex);
        if (buffer.empty()) {
            return "";
        }
        string result = buffer;
        buffer.clear();
        return result;
    }

    char getc() override {
        lock_guard<mutex> lock(buffer_mutex);
        if (buffer.empty()) {
            return '\0';
        }
        char c = buffer[0];
        buffer.erase(0, 1);  // Remove first character
        return c;
    }

    void write(string input) override {
        if (is_running) {
            ::write(master_fd, input.c_str(), input.length());
        }
    }

    void writeln(string input) override {
        write(input + "\n");
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
        this_thread::sleep_for(POLL_INTERVAL);  // Use the constant instead
        return true;
    }

    void set_poll_interval(chrono::microseconds interval) {
        // This method should probably be removed since POLL_INTERVAL is now const
        throw runtime_error("Cannot modify constant poll interval");
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

// Initialize static members
volatile sig_atomic_t TerminalEmulator::g_running = 1;
constexpr chrono::microseconds TerminalEmulator::POLL_INTERVAL;  // Definition

class TerminalIO: public TerminalBase {
private:
    static constexpr size_t INITIAL_BUFFER_SIZE = 4096;
    mutex buffer_mutex;

public:
    TerminalIO() {
        setup_terminal(true);
    }
    
    string read() {
        vector<char> buffer(INITIAL_BUFFER_SIZE);
        string result;
        
        while (true) {
            ssize_t nread = ::read(STDIN_FILENO, buffer.data(), buffer.size());
            if (nread <= 0) break;  // Error or no more data
            
            result.append(buffer.data(), nread);
            
            // Check if there might be more data
            if (nread == buffer.size()) {
                continue;  // Try reading more
            }
            break;
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
