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

namespace tools {

    class Logger {
    public:
        enum class Level {
            DEBUG,  // Highest level, rules them all
            INFO,
            WARNING,
            ERROR
        };

        using LogFormatter = function<string(Level, const string&, const string&)>;

    private:
        string name;
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

        LogFormatter formatter;

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
                default: return "UNKNOWN";
            }
        }

    public:
        Logger(const string& name, const string& filename = "", LogFormatter customFormatter = nullptr)
            : name(name), formatter(customFormatter ? customFormatter : [this](Level level, const string& name, const string& message) {
                return defaultFormatter(level, name, message);
            }) {
            if (!filename.empty()) {

                // Extract the directory path                
                // string directory = filename.substr(0, filename.find_last_of('/'));
                // if (!directory.empty() && !is_dir(directory)) {
                //     mkdir(directory); // Create the directory if it doesn't exist
                // }

                logFile.open(filename, ios::app);
                if (!logFile.is_open())
                {
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

        void setMinLogLevel(Level level) {
            minLogLevel = level;
        }

        void log(Level level, const string& message) {
            if (level < minLogLevel) return; // Skip if below minimum level
            if (message.empty()) return; // Ignore empty log notes
            lock_guard<mutex> lock(queueMutex);
            logQueue.push(formatter(level, name, message));
            queueCondition.notify_one();
        }

        void debug(const string& message) {
            log(Level::DEBUG, message);
        }

        void info(const string& message) {
            log(Level::INFO, message);
        }

        void warning(const string& message) {
            log(Level::WARNING, message);
        }

        void error(const string& message) {
            log(Level::ERROR, message);
        }
    };

    // Macros for convenience
    #define LOG_DEBUG(logger, message) logger.debug(string(__FILE__) + ":" + to_string(__LINE__) + " " + message)
    #define LOG_INFO(logger, message) logger.info(string(__FILE__) + ":" + to_string(__LINE__) + " " + message)
    #define LOG_WARNING(logger, message) logger.warning(string(__FILE__) + ":" + to_string(__LINE__) + " " + message)
    #define LOG_ERROR(logger, message) logger.error(string(__FILE__) + ":" + to_string(__LINE__) + " " + message)

}
