#pragma once

#include <functional>
#include <thread>
#include <atomic>
#include <vector>
#include <chrono>
#include <condition_variable>
#include <mutex>

#include "Logger.hpp"

using namespace std;

namespace tools::utils {

    class TaskStop: public exception {};

    class Task {
    private:
        atomic<bool> paused{false};
        atomic<bool> finished{false};
        thread worker;
        function<void()> callback;
        int tickms;
        bool immediate_start; // Flag for immediate execution
        int run_n_times; // Number of times to run the task (0 for infinite)
        mutex mtx;
        condition_variable cv;
        Logger& logger; // Reference to a Logger instance

        void run() {
            unique_lock<mutex> lock(mtx);
            bool first_run = true; // Track the first run
            int run_count = 0; // Track the number of executions

            while (!finished && (run_n_times == 0 || run_count < run_n_times)) {
                if (first_run && immediate_start) {
                    // Skip the initial wait if immediate_start is true
                    first_run = false;
                } else {
                    // Wait for `tickms` or until `finished` is set
                    cv.wait_for(lock, chrono::milliseconds(tickms), [this] {
                        return finished.load();
                    });
                }

                if (finished) break;
                if (paused) continue;
                
                try {
                    // Allow callback to signal completion by throwing TaskStop
                    callback(); 
                } catch (const TaskStop& e) {
                    string what = e.what();
                    if (!what.empty()) logger.error("Task stop reason:: " + string(e.what()));
                    finished = true;
                } catch (const exception& e) {
                    // Log the exception using the Logger
                    logger.error("Exception in task callback: " + string(e.what()));
                }

                run_count++; // Increment the run counter
                if (run_n_times != 0 && run_count >= run_n_times) {
                    finished = true;
                }
            }
        }

    public:
        // Constructor now takes a Logger reference, immediate_start flag, and run_n_times
        template<typename... Args>
        Task(
            Logger& logger,
            int tickms, 
            bool immediate_start, 
            int run_n_times, 
            Args&&... args
        ): 
            logger(logger),
            callback(bind(forward<Args>(args)...)),
            tickms(tickms),
            immediate_start(immediate_start),
            run_n_times(run_n_times)
        {
            worker = thread(&Task::run, this);
        }
    
        ~Task() { stop(); }

        void pause()  { paused = true; }
        void resume() { paused = false; }
        void stop() {
            {
                lock_guard<mutex> lock(mtx);
                finished = true;
            }
            cv.notify_all(); // Interrupt the sleep
            if (worker.joinable()) worker.join();
        }

        bool is_paused() const  { return paused; }
        bool is_finished() const { return finished; }
    };

    class Tasks {
    private:
        vector<Task*> tasks;
        Logger& logger; // Reference to a Logger instance

        // Forbidden copy operations
        Tasks(const Tasks&) = delete;
        Tasks& operator=(const Tasks&) = delete;

        // Helper to clean up tasks
        void cleanup() {
            for (Task* task : tasks) {
                task->stop();
                delete task;
            }
            tasks.clear();
        }

    public:
        // Constructor now takes a Logger reference
        Tasks(Logger& logger) : logger(logger) {}
        ~Tasks() { cleanup(); }

        // Add move semantics to allow transferring ownership (optional)
        Tasks(Tasks&& other) noexcept : tasks(move(other.tasks)), logger(other.logger) {}
        Tasks& operator=(Tasks&& other) noexcept {
            if (this != &other) {
                cleanup();
                tasks = move(other.tasks);
                logger = move(other.logger);
            }
            return *this;
        }

        template<typename... Args>
        Task& start(int tickms, bool immediate_start, int run_n_times, Args&&... args) {
            Task* task = new Task(logger, tickms, immediate_start, run_n_times, forward<Args>(args)...);
            tasks.push_back(task);
            return *task;
        }

        // Start a task immediately and run indefinitely (until stopped).
        template<typename... Args>
        Task& timer(int tickms, Args&&... args) {
            return start(tickms, true, 0, forward<Args>(args)...);
        }

        // Start a task after the first tick and run indefinitely (until stopped).
        template<typename... Args>
        Task& delay(int tickms, Args&&... args) {
            return start(tickms, false, 0, forward<Args>(args)...);
        }

        // Start a task immediately and run exactly once.
        template<typename... Args>
        Task& fork(Args&&... args) {
            return start(0, true, 1, forward<Args>(args)...);
        }

        // Start a task after the first tick and run exactly once.
        template<typename... Args>
        Task& fuze(int tickms, Args&&... args) {
            return start(tickms, false, 1, forward<Args>(args)...);
        }

        // Start a task immediately and run a specific number of times.
        template<typename... Args>
        Task& repeat(int tickms, int n, Args&&... args) {
            return start(tickms, true, n, forward<Args>(args)...);
        }

        // Start a task after the first tick and run a specific number of times.
        template<typename... Args>
        Task& schedule(int tickms, int n, Args&&... args) {
            return start(tickms, false, n, forward<Args>(args)...);
        }

        // Start a task immediately and run indefinitely (until stopped), with no tick delay.
        template<typename... Args>
        Task& loop(Args&&... args) {
            return start(0, true, 0, forward<Args>(args)...);
        }

        // Start a task after the first tick and run indefinitely (until stopped), with no tick delay.
        template<typename... Args>
        Task& defer(Args&&... args) {
            return start(0, false, 0, forward<Args>(args)...);
        }
    };

} // namespace tools::utils

#ifdef TEST

#include "Test.hpp"

using namespace tools::utils;

// Test 1: Basic task creation and execution
void test_Tasks_basic() {
    Logger logger("TestLogger");
    logger.setMinLogLevel(Logger::Level::NONE);
    Tasks tasks(logger);
    bool executed = false;

    Task& task = tasks.start(100, false, 0, [&]() {
        executed = true;
    });

    this_thread::sleep_for(chrono::milliseconds(300));
    assert(executed && "Task did not execute!");
    task.stop();
}

// Test 2: Pause and resume functionality
void test_Tasks_pause_resume() {
    Logger logger("TestLogger");
    logger.setMinLogLevel(Logger::Level::NONE);
    Tasks tasks(logger);
    int counter = 0;

    Task& task = tasks.start(100, false, 0, [&]() {
        counter++;
    });

    this_thread::sleep_for(chrono::milliseconds(300));
    task.pause();
    int pausedValue = counter;
    this_thread::sleep_for(chrono::milliseconds(300));
    assert(counter == pausedValue && "Task did not pause!");

    task.resume();
    this_thread::sleep_for(chrono::milliseconds(300));
    assert(counter > pausedValue && "Task did not resume!");
    task.stop();
}

// Test 3: Stop functionality
void test_Tasks_stop() {
    Logger logger("TestLogger");
    logger.setMinLogLevel(Logger::Level::NONE);
    Tasks tasks(logger);
    bool executed = false;

    Task& task = tasks.start(100, false, 0, [&]() {
        executed = true;
    });

    task.stop();
    this_thread::sleep_for(chrono::milliseconds(300));
    assert(!executed && "Task did not stop!");
}

// Test 4: Multiple tasks
void test_Tasks_multiple_tasks() {
    Logger logger("TestLogger");
    logger.setMinLogLevel(Logger::Level::NONE);
    Tasks tasks(logger);
    int counter1 = 0, counter2 = 0;

    Task& task1 = tasks.start(100, false, 0, [&]() {
        counter1++;
    });

    Task& task2 = tasks.start(200, false, 0, [&]() {
        counter2++;
    });

    this_thread::sleep_for(chrono::milliseconds(500));
    assert(counter1 > 0 && "Task 1 did not execute!");
    assert(counter2 > 0 && "Task 2 did not execute!");

    task1.stop();
    task2.stop();
}

// Test 5: Task destruction and cleanup
void test_Tasks_cleanup() {
    Logger logger("TestLogger");
    logger.setMinLogLevel(Logger::Level::NONE);
    Tasks tasks(logger);
    bool executed = false;

    Task& task = tasks.start(100, false, 0, [&]() {
        executed = true;
    });

    this_thread::sleep_for(chrono::milliseconds(300));
    assert(executed && "Task did not execute!");

    // Destructor of Tasks will clean up the task
}

// Test 6: Move semantics
void test_Tasks_move_semantics() {
    Logger logger("TestLogger");
    logger.setMinLogLevel(Logger::Level::NONE);
    Tasks tasks(logger);
    int counter = 0;

    Task& task = tasks.start(100, false, 0, [&]() {
        counter++;
    });

    Tasks tasks2 = move(tasks); // Move tasks1 to tasks2
    this_thread::sleep_for(chrono::milliseconds(300));
    assert(counter > 0 && "Task did not execute after move!");

    task.stop();
}

void test_Tasks_callback_exactly_5_times() {
    Logger logger("TestLogger");
    logger.setMinLogLevel(Logger::Level::NONE);
    Tasks tasks(logger);

    int counter = 0;
    Task& task = tasks.start(100, true, 5, [&]() {
        counter++;
    });
    this_thread::sleep_for(chrono::milliseconds(1000));
    assert(counter == 5 && "Task callback only once!");
}

void test_Tasks_callback_throws_stop_at_3th() {
    Logger logger("TestLogger");
    logger.setMinLogLevel(Logger::Level::NONE);
    Tasks tasks(logger);

    int counter = 0;
    Task& task = tasks.start(100, true, 5, [&]() {
        counter++;
        if (counter == 3) throw TaskStop();
    });
    this_thread::sleep_for(chrono::milliseconds(1000));
    assert(counter == 3 && "Task callback only once!");
}


TEST(test_Tasks_basic);
TEST(test_Tasks_pause_resume);
TEST(test_Tasks_stop);
TEST(test_Tasks_multiple_tasks);
TEST(test_Tasks_cleanup);
TEST(test_Tasks_move_semantics);
TEST(test_Tasks_callback_exactly_5_times);
TEST(test_Tasks_callback_throws_stop_at_3th);
#endif
