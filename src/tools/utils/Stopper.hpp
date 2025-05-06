#pragma once

#include <thread>
#include <chrono>
#include <stdexcept>

using namespace std;

namespace tools::utils {

    class Stopper {
    public:
        Stopper(): running(false), elapsed(0), startTime() {}

        // Start the stopwatch (throws if already running)
        void start() {
            if (running)
                throw runtime_error("Stopper is already running");
            running = true;
            startTime = chrono::high_resolution_clock::now();
        }

        // Pause the stopwatch (throws if not running)
        void pause() {
            if (!running)
                throw runtime_error("Stopper is not running");
            running = false;
            auto endTime = chrono::high_resolution_clock::now();
            elapsed += chrono::duration_cast<chrono::nanoseconds>(endTime - startTime).count();
        }

        // Resume the stopwatch (throws if already running)
        void resume() {
            if (running)
                throw runtime_error("Stopper is already running");
            running = true;
            startTime = chrono::high_resolution_clock::now();
        }

        // Stop the stopwatch and return elapsed time in milliseconds (throws if not running)
        double stop() {
            if (!running)
                throw runtime_error("Stopper is not running");
            running = false;
            auto endTime = chrono::high_resolution_clock::now();
            elapsed += chrono::duration_cast<chrono::nanoseconds>(endTime - startTime).count();
            double elapsedMs = elapsed / 1'000'000.0; // Convert nanoseconds to milliseconds
            reset(); // Reset for next use
            return elapsedMs;
        }

        // Get elapsed time so far in milliseconds (works whether running or paused)
        double getElapsedMs() const {
            if (running) {
                auto currentTime = chrono::high_resolution_clock::now();
                auto currentElapsed = elapsed + 
                    chrono::duration_cast<chrono::nanoseconds>(currentTime - startTime).count();
                return currentElapsed / 1'000'000.0; // Convert to milliseconds
            }
            return elapsed / 1'000'000.0; // Convert to milliseconds
        }

        // Reset the stopwatch to initial state
        void reset() {
            running = false;
            elapsed = 0;
            startTime = chrono::high_resolution_clock::now();
        }

        // Check if the stopwatch is running
        bool isRunning() const {
            return running;
        }

    private:
        bool running;
        long long elapsed; // Accumulated time in nanoseconds
        chrono::time_point<chrono::high_resolution_clock> startTime;
    };
}


#ifdef TEST

// #include "Test.hpp"

#include "../str/str_contains.h"

using namespace tools::str;
using namespace tools::utils;

void test_Stopper_start_stop_basic() {
    Stopper stopper;
    stopper.start();
    this_thread::sleep_for(chrono::milliseconds(100));
    double elapsed = stopper.stop();
    assert(elapsed >= 90 && elapsed <= 150 && "Elapsed time should be approximately 100ms");
    assert(!stopper.isRunning() && "Stopper should not be running after stop");
}

void test_Stopper_pause_resume() {
    Stopper stopper;
    stopper.start();
    this_thread::sleep_for(chrono::milliseconds(50));
    stopper.pause();
    double elapsedPaused = stopper.getElapsedMs();
    assert(elapsedPaused >= 45 && elapsedPaused <= 75 && "Paused elapsed time should be approximately 50ms");
    stopper.resume();
    this_thread::sleep_for(chrono::milliseconds(50));
    double elapsed = stopper.stop();
    assert(elapsed >= 90 && elapsed <= 150 && "Total elapsed time should be approximately 100ms");
}

void test_Stopper_getElapsedMs_running() {
    Stopper stopper;
    stopper.start();
    this_thread::sleep_for(chrono::milliseconds(100));
    double elapsed = stopper.getElapsedMs();
    assert(elapsed >= 90 && elapsed <= 150 && "Running elapsed time should be approximately 100ms");
    stopper.stop();
}

void test_Stopper_getElapsedMs_paused() {
    Stopper stopper;
    stopper.start();
    this_thread::sleep_for(chrono::milliseconds(50));
    stopper.pause();
    double elapsed = stopper.getElapsedMs();
    assert(elapsed >= 45 && elapsed <= 75 && "Paused elapsed time should be approximately 50ms");
    stopper.resume();
    stopper.stop();
}

void test_Stopper_reset() {
    Stopper stopper;
    stopper.start();
    this_thread::sleep_for(chrono::milliseconds(50));
    stopper.pause();
    stopper.reset();
    double elapsed = stopper.getElapsedMs();
    assert(elapsed == 0 && "Elapsed time should be 0 after reset");
    assert(!stopper.isRunning() && "Stopper should not be running after reset");
}

void test_Stopper_start_already_running() {
    Stopper stopper;
    stopper.start();
    bool thrown = false;
    try {
        stopper.start();
    } catch (exception &e) {
        thrown = true;
        string what = e.what();
        assert(str_contains(what, "Stopper is already running") && "Exception message should indicate already running");
    }
    assert(thrown && "Start while running should throw");
    stopper.stop();
}

void test_Stopper_pause_not_running() {
    Stopper stopper;
    bool thrown = false;
    try {
        stopper.pause();
    } catch (exception &e) {
        thrown = true;
        string what = e.what();
        assert(str_contains(what, "Stopper is not running") && "Exception message should indicate not running");
    }
    assert(thrown && "Pause while not running should throw");
}

void test_Stopper_stop_not_running() {
    Stopper stopper;
    bool thrown = false;
    try {
        stopper.stop();
    } catch (exception &e) {
        thrown = true;
        string what = e.what();
        assert(str_contains(what, "Stopper is not running") && "Exception message should indicate not running");
    }
    assert(thrown && "Stop while not running should throw");
}

void test_Stopper_resume_already_running() {
    Stopper stopper;
    stopper.start();
    bool thrown = false;
    try {
        stopper.resume();
    } catch (exception &e) {
        thrown = true;
        string what = e.what();
        assert(str_contains(what, "Stopper is already running") && "Exception message should indicate already running");
    }
    assert(thrown && "Resume while running should throw");
    stopper.stop();
}

void test_Stopper_immediate_stop() {
    Stopper stopper;
    stopper.start();
    double elapsed = stopper.stop();
    assert(elapsed >= 0 && elapsed <= 10 && "Immediate stop should have near-zero elapsed time");
    assert(!stopper.isRunning() && "Stopper should not be running after stop");
}

// Register tests
TEST(test_Stopper_start_stop_basic);
TEST(test_Stopper_pause_resume);
TEST(test_Stopper_getElapsedMs_running);
TEST(test_Stopper_getElapsedMs_paused);
TEST(test_Stopper_reset);
TEST(test_Stopper_start_already_running);
TEST(test_Stopper_pause_not_running);
TEST(test_Stopper_stop_not_running);
TEST(test_Stopper_resume_already_running);
TEST(test_Stopper_immediate_stop);

#endif