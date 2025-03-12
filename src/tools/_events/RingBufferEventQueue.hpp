#pragma once

#include <memory>

#include "../utils/Logger.hpp"
#include "../utils/RingBuffer.hpp"

#include "EventQueue.hpp"

using namespace std;
using namespace tools::utils;

namespace tools::events {
    
    /**
     * Implementation of EventQueue using RingBuffer
     */
    class RingBufferEventQueue : public EventQueue {
    public:
        RingBufferEventQueue(
            size_t capacity, 
            Logger& logger
             // TODO: add expiry default NEVER, and remove expired evens periodically or when the buffer is full!!!
        ):
            ringBuffer(capacity, RingBuffer<Event>::WritePolicy::Rotate),
            logger(logger)
        {
            ringBuffer.set_drop_callback([this, &logger](size_t count) {
                logger.warn("Dropped " + to_string(count) + " event(s) due to full queue");
                if (dropCallback) dropCallback(count);
            });
        }

        bool write(Event& event) override {
            return ringBuffer.write(&event, 1);
        }

        size_t read(Event& event, bool blocking, int timeoutMs) override {
            return ringBuffer.read(&event, 1, blocking, timeoutMs);
        }

        size_t available() const override {
            return ringBuffer.available();
        }

        using DropCallback = function<void(size_t)>;

        void setDropCallback(DropCallback callback) {
            dropCallback = callback;
        }

        size_t getCapacity() const {
            return ringBuffer.getCapacity();
        }

    private:
        RingBuffer<Event> ringBuffer;
        Logger& logger;
        DropCallback dropCallback;
    };    

}

#ifdef TEST

#include "../utils/Test.hpp"
#include "../utils/tests/MockLogger.hpp"
#include "tests/TestEvent.hpp"

// Test basic write operation
void test_RingBufferEventQueue_write_basic() {
    MockLogger logger;
    RingBufferEventQueue queue(3, logger);
    TestEvent event(42);

    bool success = queue.write(event);
    assert(success == true && "Write should succeed with empty queue");
    size_t available = queue.available();
    assert(available == 1 && "Queue should have 1 event after write");
}

// Test reading from queue
void test_RingBufferEventQueue_read_basic() {
    MockLogger logger;
    RingBufferEventQueue queue(3, logger);
    TestEvent event(42);
    queue.write(event);

    TestEvent readEvent(0);
    size_t readCount = queue.read(readEvent, false, 0);
    assert(readCount == 1 && "Read should return 1 event");
    assert(readEvent.value == 42 && "Read event should match written event");
    size_t available = queue.available();
    assert(available == 0 && "Queue should be empty after read");
}

// Test reading from empty queue
void test_RingBufferEventQueue_read_empty() {
    MockLogger logger;
    RingBufferEventQueue queue(3, logger);

    TestEvent readEvent(0);
    size_t readCount = queue.read(readEvent, false, 0);
    assert(readCount == 0 && "Read from empty queue should return 0");
    // assert(readEvent == nullptr && "Read event should be null when queue is empty");
}

// Test write when queue is full (triggers drop callback)
void test_RingBufferEventQueue_write_full() {
    MockLogger logger;
    RingBufferEventQueue queue(2, logger);  // Capacity of 2
    TestEvent event1(1);
    TestEvent event2(2);
    queue.write(event1);
    queue.write(event2);
    
    TestEvent event3(3);
    bool success = queue.write(event3);  // Should overwrite oldest
    assert(success == true && "Write should succeed with Rotate policy");
    size_t available = queue.available();
    assert(available == 2 && "Queue should still have 2 events after overwrite");

    bool loggedDrop = logger.hasMessageContaining("Dropped 1 event(s) due to full queue");
    assert(loggedDrop == true && "Drop callback should log event drop");
}

// Test available method
void test_RingBufferEventQueue_available_multiple() {
    MockLogger logger;
    RingBufferEventQueue queue(3, logger);
    TestEvent event1(1);
    TestEvent event2(2);
    queue.write(event1);
    queue.write(event2);

    size_t available = queue.available();
    assert(available == 2 && "Available should return 2 after two writes");

    TestEvent readEvent(0);
    queue.read(readEvent, false, 0);
    available = queue.available();
    assert(available == 1 && "Available should return 1 after one read");
}

// Test blocking read with timeout (no data)
void test_RingBufferEventQueue_read_blocking_timeout() {
    MockLogger logger;
    RingBufferEventQueue queue(3, logger);

    auto start = chrono::steady_clock::now();
    TestEvent readEvent(0);
    size_t readCount = queue.read(readEvent, true, 500);  // 500ms timeout
    auto end = chrono::steady_clock::now();
    auto durationMs = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    assert(readCount == 0 && "Blocking read should return 0 with no data");
    assert(durationMs >= 500 && "Blocking read should wait at least 500ms");
}

// Test blocking read with data available
void test_RingBufferEventQueue_read_blocking_success() {
    MockLogger logger;
    RingBufferEventQueue queue(3, logger);
    TestEvent event(99);
    queue.write(event);

    TestEvent readEvent(0);
    size_t readCount = queue.read(readEvent, true, 500);
    assert(readCount == 1 && "Blocking read should return 1 when data is available");
    assert(readEvent.value == 99 && "Read event should match written event");
}

// Test constructor with invalid capacity
void test_RingBufferEventQueue_constructor_invalid_capacity() {
    MockLogger logger;
    bool thrown = false;

    try {
        RingBufferEventQueue queue(0, logger);  // Capacity < 1 should throw
    } catch (const invalid_argument& e) {
        thrown = true;
        string what = e.what();
        assert(str_contains(what, "Capacity must be at least 1") && "Exception message should indicate invalid capacity");
    }
    assert(thrown == true && "Constructor should throw for capacity < 1");
}

// Test concurrent writes
void test_RingBufferEventQueue_write_concurrent() {
    MockLogger logger;
    RingBufferEventQueue queue(100, logger);  // Large capacity to avoid drops

    vector<thread> writers;
    const int numThreads = 4;
    const int writesPerThread = 10;

    for (int i = 0; i < numThreads; ++i) {
        writers.emplace_back([i, &queue]() {
            for (int j = 0; j < writesPerThread; ++j) {
                TestEvent event(i * writesPerThread + j);
                while (!queue.write(event)) {
                    this_thread::yield();  // Retry if full (unlikely with large capacity)
                }
            }
        });
    }

    for (thread& writer : writers) {
        writer.join();
    }

    size_t available = queue.available();
    assert(available == numThreads * writesPerThread && "All concurrent writes should succeed");
}

// Test concurrent reads and writes
void test_RingBufferEventQueue_read_write_concurrent() {
    MockLogger logger;
    RingBufferEventQueue queue(50, logger);
    const int numWrites = 30;
    const int numReads = 20;
    atomic<int> writeCount{0};
    atomic<int> readCount{0};

    // Writer thread
    thread writer([&]() {
        for (int i = 0; i < numWrites; ++i) {
            TestEvent event(i);
            while (!queue.write(event)) {
                this_thread::yield();
            }
            writeCount++;
        }
    });

    // Reader thread
    thread reader([&]() {
        for (int i = 0; i < numReads; ++i) {
            Event event;
            size_t count = queue.read(event, true, 1000);
            if (count == 1) {
                readCount++;
            }
            this_thread::yield();
        }
    });

    writer.join();
    reader.join();

    assert(writeCount == numWrites && "All writes should complete");
    assert(readCount <= writeCount && "Reads should not exceed writes");
    assert(readCount <= numReads && "Reads should not exceed requested number");
    size_t finalAvailable = queue.available();
    assert(finalAvailable == (writeCount - readCount) && "Remaining events should match writes minus reads");
}

// Register tests
TEST(test_RingBufferEventQueue_write_basic);
TEST(test_RingBufferEventQueue_read_basic);
TEST(test_RingBufferEventQueue_read_empty);
TEST(test_RingBufferEventQueue_write_full);
TEST(test_RingBufferEventQueue_available_multiple);
TEST(test_RingBufferEventQueue_read_blocking_timeout);
TEST(test_RingBufferEventQueue_read_blocking_success);
TEST(test_RingBufferEventQueue_constructor_invalid_capacity);
TEST(test_RingBufferEventQueue_write_concurrent);
TEST(test_RingBufferEventQueue_read_write_concurrent);
#endif

/* TODO:
Not Covered: Edge cases like maximum capacity limits or specific contention patterns (e.g., all threads writing simultaneously to a tiny queue), which could be added but are less critical given RingBuffer’s internal synchronization.
*/