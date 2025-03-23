#pragma once

#include <unistd.h>
#include <termios.h>
#include <poll.h>
#include <thread>
#include <vector>
#include <iostream>
#include <functional>
#include <cstring>

#include "Closable.hpp"
#include "ERROR.hpp"
#include "foreach.hpp"

using namespace std;

namespace tools::utils {

    class InputPipeInterceptor: public Closable {
    public:

        using InputSequenceDetectionCallback = function<void(vector<char> /*sequence*/)>;

        InputPipeInterceptor() {
            if (pipe(pipe_fds) == -1) throw ERROR("Pipe creation failed");

            atexit(cleanup_terminal);

            // Start input handler in a separate thread
            t = thread([this]() {
                try {
                    input_handler(*this, pipe_fds[1]); // TODO: it hooks the input (1) but we may want to parameterize it for output also?
                } catch (exception& e) {
                    cerr << "Input interceptor error: " << e.what() << endl;
                }
            });
        }

        virtual ~InputPipeInterceptor() {// Cleanup
            close();
            if (t.joinable()) t.join(); // In a real app, ensure proper thread termination
            ::close(pipe_fds[0]);
            ::close(pipe_fds[1]);
        }

        void subsrcibe(void* subscriber, InputSequenceDetectionCallback callback) {
            lock_guard<mutex> lock(callbacks_mutex);
            callbacks[subscriber] = callback;
        }

        size_t unsubscribe(void* subsciber) {
            lock_guard<mutex> lock(callbacks_mutex);
            return callbacks.erase(subsciber);
        }

        int getPipeFileDescriptorAt(size_t n) {
            return pipe_fds[n];
        }
    
    private:


        void onSequenceDetected(vector<char> sequence) {
            //lock_guard<mutex> lock(callbacks_mutex);
            foreach(callbacks, [sequence](InputSequenceDetectionCallback callback, void* subscriber) {
                if (subscriber && callback) callback(sequence);
            });

            // for (InputSequenceDetectionCallback callback: callbacks) if (callback) callback(sequence);
            
            // string s = "";
            // for (const char& c: seq) s += "[" + to_string((int)c) + "]"; 
            // cout << "Sequence detected: " << s << endl;
        }

        static void input_handler(InputPipeInterceptor& that, int write_fd) {
            char c;
            struct termios original = set_raw_mode(); // Set raw mode, save original
            vector<char> seq;
            while (!that.closing) {
                // // Read one character from stdin
                // ssize_t n = read(STDIN_FILENO, &c, 1);
                // if (n <= 0) {
                //     if (n == -1) cerr << "Read error\n";
                //     continue;
                // }

                // Set up pollfd structure for STDIN_FILENO
                struct pollfd fds = {STDIN_FILENO, POLLIN, 0};
                int timeout = 100; // TODO make configurable: 100ms timeout (adjust as needed)

                // Wait for input or timeout
                int ret = poll(&fds, 1, timeout);
                if (ret < 0) {
                    std::cerr << "Poll error: " << strerror(errno) << "\n";
                    break;
                }
                if (ret == 0) {
                    // Timeout, no input, check closing and loop
                    continue;
                }

                // Input is available, read it
                ssize_t n = read(STDIN_FILENO, &c, 1);
                if (n <= 0) {
                    if (n == -1) std::cerr << "Read error: " << strerror(errno) << "\n";
                    continue;
                }
        
                seq = {c}; // Start sequence

                if (c == 27) { // ESC key detected
                    // Check for additional characters (part of an escape sequence)
                    struct pollfd fds = {STDIN_FILENO, POLLIN, 0};
                    while (poll(&fds, 1, 10) > 0) { // 10ms timeout
                        char next;
                        if (read(STDIN_FILENO, &next, 1) == 1) {
                            seq.push_back(next);
                        } else {
                            break;
                        }
                    }
        
                    // Handle standalone ESC (if no additional characters)
                    // if (seq.size() == 1) {
                    //     cout << "Standalone ESC detected\n";
                    //     // Example action: toggle mute or custom logic here
                    // }
        
                }

                that.onSequenceDetected(seq);

                if (seq.size()) {
                    // Forward the entire sequence to the pipe
                    if (::write(write_fd, seq.data(), seq.size()) == -1) {
                        cerr << "Write to pipe failed\n";
                        that.close();
                    }
                }

            }
        
            // Restore terminal settings (unreachable in this loop, see main)
            restore_terminal(original);
        }

        // Function to set terminal to raw mode and return original settings
        static struct termios set_raw_mode() {
            struct termios original, raw;
            tcgetattr(STDIN_FILENO, &original); // Save original settings
            raw = original;
            raw.c_lflag &= ~(ICANON | ECHO);    // Disable canonical mode and echo
            raw.c_cc[VMIN] = 1;                 // Read at least 1 character
            raw.c_cc[VTIME] = 0;                // No timeout
            tcsetattr(STDIN_FILENO, TCSANOW, &raw);
            return original;                    // Return original for restoration
        }

        // Restore terminal settings
        static void restore_terminal(const struct termios& original) {
            tcsetattr(STDIN_FILENO, TCSANOW, &original);
        }

        static void cleanup_terminal() {
            struct termios original;
            tcgetattr(STDIN_FILENO, &original);
            tcsetattr(STDIN_FILENO, TCSANOW, &original);
        }

        int pipe_fds[2];
        thread t;
        unordered_map<void*, InputSequenceDetectionCallback> callbacks;
        mutex callbacks_mutex;
    };
        

}
