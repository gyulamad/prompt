#pragma once

#include <functional>
#include <thread>
#include <atomic>
#include <vector>
#include <chrono>
#include <cassert>
#include <condition_variable>
#include <mutex>

using namespace std;

namespace tools {

    class Task {
    private:
        std::atomic<bool> paused{false};
        std::atomic<bool> finished{false};
        std::thread worker;
        std::function<void()> callback;
        int tickms;
        std::mutex mtx;
        std::condition_variable cv;
    
        void run() {
            std::unique_lock<std::mutex> lock(mtx);
            while (!finished) {
                // Wait for `tickms` or until `finished` is set
                cv.wait_for(lock, std::chrono::milliseconds(tickms), [this] {
                    return finished.load();
                });
    
                if (finished) break;
                if (paused) continue;
                callback();
            }
        }
    
    public:
        template<typename... Args>
        Task(int tickms, Args&&... args) 
            : callback(std::bind(std::forward<Args>(args)...)), tickms(tickms) {
            worker = std::thread(&Task::run, this);
        }
    
        ~Task() { stop(); }
    
        void pause()  { paused = true; }
        void resume() { paused = false; }
        void stop() {
            {
                std::lock_guard<std::mutex> lock(mtx);
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
        Tasks() = default;
        ~Tasks() { cleanup(); }

        // Add move semantics to allow transferring ownership (optional)
        Tasks(Tasks&& other) noexcept : tasks(move(other.tasks)) {}
        Tasks& operator=(Tasks&& other) noexcept {
            if (this != &other) {
                cleanup();
                tasks = move(other.tasks);
            }
            return *this;
        }

        template<typename... Args>
        Task& start(int tickms, Args&&... args) {
            Task* task = new Task(tickms, forward<Args>(args)...);
            tasks.push_back(task);
            return *task;
        }
    };

} // namespace tools

#ifdef TEST
// Test 1: Basic task creation and execution
void test_Tasks_basic() {
    Tasks tasks;
    bool executed = false;

    Task& task = tasks.start(100, [&]() {
        executed = true;
    });

    this_thread::sleep_for(chrono::milliseconds(300));
    assert(executed && "Task did not execute!");
    task.stop();
}

// Test 2: Pause and resume functionality
void test_Tasks_pause_resume() {
    Tasks tasks;
    int counter = 0;

    Task& task = tasks.start(100, [&]() {
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
    Tasks tasks;
    bool executed = false;

    Task& task = tasks.start(100, [&]() {
        executed = true;
    });

    task.stop();
    this_thread::sleep_for(chrono::milliseconds(300));
    assert(!executed && "Task did not stop!");
}

// Test 4: Multiple tasks
void test_Tasks_multiple_tasks() {
    Tasks tasks;
    int counter1 = 0, counter2 = 0;

    Task& task1 = tasks.start(100, [&]() {
        counter1++;
    });

    Task& task2 = tasks.start(200, [&]() {
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
    Tasks tasks;
    bool executed = false;

    Task& task = tasks.start(100, [&]() {
        executed = true;
    });

    this_thread::sleep_for(chrono::milliseconds(300));
    assert(executed && "Task did not execute!");

    // Destructor of Tasks will clean up the task
}

// Test 6: Move semantics
void test_Tasks_move_semantics() {
    Tasks tasks1;
    int counter = 0;

    Task& task = tasks1.start(100, [&]() {
        counter++;
    });

    Tasks tasks2 = move(tasks1); // Move tasks1 to tasks2
    this_thread::sleep_for(chrono::milliseconds(300));
    assert(counter > 0 && "Task did not execute after move!");

    task.stop();
}


TEST(test_Tasks_basic);
TEST(test_Tasks_pause_resume);
TEST(test_Tasks_stop);
TEST(test_Tasks_multiple_tasks);
TEST(test_Tasks_cleanup);
TEST(test_Tasks_move_semantics);
#endif
