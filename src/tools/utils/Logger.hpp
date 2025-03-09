#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>
#include <functional>

#include "files.hpp"

/*
Key Features:
DEBUG Level:

Added DEBUG as the highest log level. It can be used for detailed debugging information.

Thread Safety:

Uses a mutex to ensure thread-safe logging.

Asynchronous Logging:

Logs are processed in a background thread to avoid blocking the main application.

File Logging:

Supports logging to a file in addition to the console.

Custom Log Format:

Allows users to define their own log format using a LogFormatter function.

Log Level Filtering:

Users can set a minimum log level (e.g., only log messages with severity >= WARNING).

Convenience Macros:

Macros like LOG_DEBUG, LOG_INFO, etc., simplify logging and automatically include file name and line number.

How to Use:

int main() {
    Logger logger("MyLogger", "logfile.txt");

    logger.setMinLogLevel(Logger::Level::DEBUG); // Set minimum log level to DEBUG

    LOG_DEBUG(logger, "This is a debug message.");
    LOG_INFO(logger, "This is an info message.");
    LOG_WARNING(logger, "This is a warning message.");
    LOG_ERROR(logger, "This is an error message.");

    return 0;
}
*/

using namespace std;

namespace tools::utils {

    class Logger {
    public:
        enum class Level {
            DEBUG,  // Highest level, rules them all
            INFO,
            WARNING,
            ERROR,
            NONE = 100000,
        };

        using LogFormatter = function<string(Level, const string&, const string&)>;

        // string noFormatter(Level level, const string& name, const string& message) {
        //     return ""; //getCurrentTime() + " [" + name + "] " + levelToString(level) + ": " + message;
        // }

    protected:
        LogFormatter formatter;
        string name;

    private:
        ofstream logFile;
        Level minLogLevel = Level::INFO;
        mutex logMutex;
        queue<string> logQueue;
        mutex queueMutex;
        condition_variable queueCondition;
        thread logThread;
        bool stopLogging = false;

        // Default log formatter
        string defaultFormatter(Level level, const string& name, const string& message) {
            return getCurrentTime() + " [" + name + "] " + levelToString(level) + ": " + message;
        }

        void processLogs() {
            while (true) {
                string logMessage;
                {
                    unique_lock<mutex> lock(queueMutex);
                    queueCondition.wait(lock, [this] { return !logQueue.empty() || stopLogging; });
                    if (stopLogging && logQueue.empty()) break;
                    logMessage = logQueue.front();
                    logQueue.pop();
                }

                if (logFile.is_open()) {
                    // Write to file only (if filename is provided)
                    lock_guard<mutex> fileLock(logMutex); // Ensure thread-safe file writing
                    logFile << logMessage << endl;
                    if (logFile.fail()) {
                        cerr << "Failed to write to log file." << endl;
                    }
                    logFile.flush(); // Flush to ensure the message is written to the file
                } else {
                    // Write to console only (if no filename is provided)
                    cout << logMessage << endl;
                }
            }
        }

        string getCurrentTime() const {
            auto now = chrono::system_clock::now();
            auto in_time_t = chrono::system_clock::to_time_t(now);

            tm bt;
            localtime_r(&in_time_t, &bt);

            ostringstream timeStream;
            timeStream << put_time(&bt, "%Y-%m-%d %H:%M:%S");
            return timeStream.str();
        }

        string levelToString(Level level) const {
            switch (level) {
                case Level::DEBUG: return "DEBUG";
                case Level::INFO: return "INFO";
                case Level::WARNING: return "WARNING";
                case Level::ERROR: return "ERROR";
                case Level::NONE: return "NONE";
                default: return "UNKNOWN";
            }
        }

    public:
        // Delete copy constructor and copy assignment operator
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;

        Logger(
            const string& name, 
            const string& filename = "", 
            LogFormatter customFormatter = nullptr
        ): 
            name(name), 
            formatter(customFormatter ? customFormatter : [this](Level level, const string& name, const string& message) {
                return defaultFormatter(level, name, message);
            }) 
        {
            if (!filename.empty()) {

                // Extract the directory path                
                // string directory = filename.substr(0, filename.find_last_of('/'));
                // if (!directory.empty() && !is_dir(directory)) {
                //     mkdir(directory); // Create the directory if it doesn't exist
                // }

                logFile.open(filename, ios::app);
                if (!logFile.is_open()) {
                    cerr << "Failed to create log file: " << filename << endl;
                }
            }
            logThread = thread(&Logger::processLogs, this);
        }

        ~Logger() {
            {
                lock_guard<mutex> lock(queueMutex);
                stopLogging = true;
            }
            queueCondition.notify_all();
            logThread.join();
            if (logFile.is_open()) {
                logFile.close();
            }
        }

        // Allow move constructor and move assignment operator
        Logger(Logger&& other) noexcept
            : name(move(other.name)),
              logFile(move(other.logFile)),
              minLogLevel(other.minLogLevel),
              logQueue(move(other.logQueue)),
              logThread(move(other.logThread)),
              stopLogging(other.stopLogging),
              formatter(move(other.formatter)) {
            other.stopLogging = true; // Ensure the moved-from object is in a valid state
        }
    
        Logger& operator=(Logger&& other) noexcept {
            if (this != &other) {
                {
                    lock_guard<mutex> lock(queueMutex);
                    stopLogging = true;
                }
                queueCondition.notify_all();
                if (logThread.joinable()) logThread.join();
                if (logFile.is_open()) logFile.close();
    
                name = move(other.name);
                logFile = move(other.logFile);
                minLogLevel = other.minLogLevel;
                logQueue = move(other.logQueue);
                logThread = move(other.logThread);
                stopLogging = other.stopLogging;
                formatter = move(other.formatter);
    
                other.stopLogging = true; // Ensure the moved-from object is in a valid state
            }
            return *this;
        }

        void setMinLogLevel(Level level) {
            minLogLevel = level;
        }

        virtual void log(Level level, const string& message) {
            if (level < minLogLevel) return; // Skip if below minimum level
            if (message.empty()) return; // Ignore empty log notes
            lock_guard<mutex> lock(queueMutex);
            // string logNote = formatter(level, name, message);
            // if (!logNote.empty()) logQueue.push(logNote);
            logQueue.push(formatter(level, name, message));
            queueCondition.notify_one();
        }

        virtual void debug(const string& message) {
            log(Level::DEBUG, message);
        }

        virtual void dbg(const string& message) {
            log(Level::DEBUG, message);
        }

        virtual void info(const string& message) {
            log(Level::INFO, message);
        }

        virtual void nfo(const string& message) {
            log(Level::INFO, message);
        }

        virtual void warning(const string& message) {
            log(Level::WARNING, message);
        }

        virtual void warn(const string& message) {
            log(Level::WARNING, message);
        }

        virtual void error(const string& message) {
            log(Level::ERROR, message);
        }

        virtual void err(const string& message) {
            log(Level::ERROR, message);
        }
    };

    // Macros for convenience
    #define LOG_DEBUG(logger, message) logger.debug(string(__FILE__) + ":" + to_string(__LINE__) + " " + message)
    #define LOG_INFO(logger, message) logger.info(string(__FILE__) + ":" + to_string(__LINE__) + " " + message)
    #define LOG_WARNING(logger, message) logger.warning(string(__FILE__) + ":" + to_string(__LINE__) + " " + message)
    #define LOG_ERROR(logger, message) logger.error(string(__FILE__) + ":" + to_string(__LINE__) + " " + message)

}


#ifdef TEST
using namespace tools::utils;

void test_Logger_log_console_output() {
    string output = capture_cout([&]() {
        Logger logger("TestLogger");
        logger.info("This is a test info message.");
        logger.warning("This is a test warning message.");
        logger.error("This is a test error message.");
        // Sleep to allow the log thread to process messages
        this_thread::sleep_for(chrono::milliseconds(100));
    });

    assert(output.find("[TestLogger] INFO: This is a test info message.") != string::npos);
    assert(output.find("[TestLogger] WARNING: This is a test warning message.") != string::npos);
    assert(output.find("[TestLogger] ERROR: This is a test error message.") != string::npos);
}

void test_Logger_log_file_output() {
    string filename = "test_log.txt";
    string output = capture_cout([&]() {
        Logger logger("TestLogger", filename);
        logger.info("This is a test info message.");
        logger.warning("This is a test warning message.");
        logger.error("This is a test error message.");
        // Sleep to allow the log thread to process messages
        this_thread::sleep_for(chrono::milliseconds(100));

        // Verify file contents
        ifstream file(filename);
        assert(file.is_open() && "Log file should be created.");
        string line;
        int count = 0;
        while (getline(file, line)) {
            assert(!line.empty() && "Log file should contain valid log messages.");
            count++;
        }
        assert(count >= 3 && "Log file should contain at least 3 log messages.");
        file.close();

        // Clean up
        filesystem::remove(filename);
    });
}

void test_Logger_setMinLogLevel_filter_logs() {
    string output = capture_cout([&]() {
        Logger logger("TestLogger");
        logger.setMinLogLevel(Logger::Level::WARNING);

        logger.info("This info message should not appear.");
        logger.warning("This warning message should appear.");
        logger.error("This error message should appear.");
        // Sleep to allow the log thread to process messages
        this_thread::sleep_for(chrono::milliseconds(100));
    });

    assert(output.find("[TestLogger] INFO: This info message should not appear.") == string::npos);
    assert(output.find("[TestLogger] WARNING: This warning message should appear.") != string::npos);
    assert(output.find("[TestLogger] ERROR: This error message should appear.") != string::npos);
}

void test_Logger_custom_formatter() {
    string output = capture_cout([&]() {
        auto customFormatter = [](Logger::Level level, const string& name, const string& message) -> string {
            return "[" + name + "] Custom: " + message;
        };
        Logger logger("TestLogger", "", customFormatter);

        logger.info("This is a test info message with a custom formatter.");
        // Sleep to allow the log thread to process messages
        this_thread::sleep_for(chrono::milliseconds(100));
    });

    assert(output.find("[TestLogger] Custom: This is a test info message with a custom formatter.") != string::npos);
}

void test_Logger_empty_message() {
    string output = capture_cout([&]() {
        Logger logger("TestLogger");

        logger.info(""); // Empty message should be ignored
        logger.warning(""); // Empty message should be ignored
        logger.error(""); // Empty message should be ignored
        // Sleep to allow the log thread to process messages
        this_thread::sleep_for(chrono::milliseconds(100));
    });

    assert(output.empty() && "Empty messages should not produce any output.");
}

void test_Logger_thread_safety() {
    string output;
    {
        // Capture all output generated within this scope (including background thread activity)
        stringstream buffer;
        streambuf* old = cout.rdbuf(buffer.rdbuf()); // Redirect cout to buffer

        {
            Logger logger("TestLogger");
            thread t1([&logger]() {
                for (int i = 0; i < 100; ++i) {
                    logger.info("Thread 1 message " + to_string(i));
                }
            });
            thread t2([&logger]() {
                for (int i = 0; i < 100; ++i) {
                    logger.info("Thread 2 message " + to_string(i));
                }
            });
            t1.join();
            t2.join();
            this_thread::sleep_for(chrono::milliseconds(500)); // Wait for background thread to process logs
        } // Logger is destroyed here, ensuring all logs are flushed

        cout.rdbuf(old); // Restore original cout buffer
        output = buffer.str();
    }

    // Count messages
    int count1 = 0, count2 = 0;
    size_t pos = 0;
    while ((pos = output.find("Thread 1 message", pos)) != string::npos) {
        count1++;
        pos += 16; // Length of "Thread 1 message"
    }
    pos = 0;
    while ((pos = output.find("Thread 2 message", pos)) != string::npos) {
        count2++;
        pos += 16; // Length of "Thread 2 message"
    }
    
    // Assertions
    assert(count1 == 100 && "All Thread 1 messages should be logged.");
    assert(count2 == 100 && "All Thread 2 messages should be logged.");
}


TEST(test_Logger_log_console_output);
TEST(test_Logger_log_file_output);
TEST(test_Logger_setMinLogLevel_filter_logs);
TEST(test_Logger_custom_formatter);
TEST(test_Logger_empty_message);
TEST(test_Logger_thread_safety);
#endif