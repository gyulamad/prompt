#pragma once

#include <mutex>
#include <condition_variable>
#include <vector>
#include <atomic>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <thread>

using namespace std;

namespace tools::utils {

    template<typename T>
    class RingBuffer {
    public:
        enum class WritePolicy {
            Reject,    // Reject new writes if there isn't enough space
            Rotate,    // Overwrite oldest items as needed (preserving a rolling window)
            Reset      // Discard all unread data before writing new data
        };

        // Caller must select a policy
        RingBuffer(size_t capacity, WritePolicy policy) 
            : buffer(capacity), capacity(capacity), policy(policy) {
            if (capacity < 1) 
                throw ERROR("Capacity must be at least 1");
        }
        
        // Original methods
        bool write(const T* data, size_t count) {
            {
                lock_guard<mutex> lock(mtx);
                size_t current_size = size.load(memory_order_acquire);
                if (current_size + count > capacity) {
                    if (dropCallback) dropCallback((current_size + count) - capacity);
                    if (policy == WritePolicy::Reject) {
                        return false;
                    } else if (policy == WritePolicy::Rotate) {
                        // Overwrite only as many items as necessary
                        size_t excess = (current_size + count) - capacity;
                        readPos = (readPos + excess) % capacity;
                        // Adjust current_size to reflect that we have discarded 'excess' items
                        current_size = current_size > excess ? current_size - excess : 0;
                    } else if (policy == WritePolicy::Reset) {
                        // Discard all existing unread data
                        current_size = 0;
                        readPos = writePos;
                    }
                }
                
                for (size_t i = 0; i < count; ++i)
                    buffer[(writePos + i) % capacity] = data[i];
                
                writePos = (writePos + count) % capacity;
                size.store(min(capacity, current_size + count), memory_order_release);
            }
            cv.notify_one();
            return true;
        }
        
        // Enhanced: Add a convenience method for writing a single item
        bool write_one(const T& item) {
            return write(&item, 1);
        }
        
        // Enhanced: Return number of items that couldn't be written (0 if all were written)
        size_t try_write_all(const T* data, size_t count) {
            {
                lock_guard<mutex> lock(mtx);
                size_t current_size = size.load(memory_order_acquire);
                size_t available_space = capacity - current_size;
                
                if (count > available_space) {
                    if (dropCallback) dropCallback(count - available_space);
                    if (policy == WritePolicy::Reject) {
                        return count; // Couldn't write any
                    } else if (policy == WritePolicy::Rotate) {
                        // We'll write everything, but need to discard oldest items
                        size_t excess = count - available_space;
                        readPos = (readPos + excess) % capacity;
                        // Adjust current_size to reflect that we have discarded 'excess' items
                        current_size = current_size > excess ? current_size - excess : 0;
                    } else if (policy == WritePolicy::Reset) {
                        // Discard all existing unread data
                        current_size = 0;
                        readPos = writePos;
                    }
                }
                
                for (size_t i = 0; i < count; ++i)
                    buffer[(writePos + i) % capacity] = data[i];
                
                writePos = (writePos + count) % capacity;
                size.store(min(capacity, current_size + count), memory_order_release);
            }
            cv.notify_one();
            return 0; // All items written
        }
        
        // Original read method
        size_t read(T* dest, size_t maxCount, bool blocking = false, int timeoutMs = 1000) {
            unique_lock<mutex> lock(mtx);
            
            if (blocking) {
                if (!cv.wait_for(lock, chrono::milliseconds(timeoutMs),
                                [this] { return size.load(memory_order_acquire) > 0; })) {
                    return 0; // Timeout reached, no data
                }
            }
            
            size_t current_size = size.load(memory_order_acquire);
            if (current_size == 0) return 0;
            
            size_t toRead = min(current_size, maxCount);
            
            for (size_t i = 0; i < toRead; ++i)
                dest[i] = buffer[(readPos + i) % capacity];
            
            readPos = (readPos + toRead) % capacity;
            size.store(current_size - toRead, memory_order_release);
            return toRead;
        }
        
        // Enhanced: Add a convenience method for reading a single item
        bool read_one(T& item) {
            return read(&item, 1) == 1;
        }
        
        // Enhanced: Add a method to peek at the next item without removing it
        bool peek_one(T& item) {
            lock_guard<mutex> lock(mtx);
            size_t current_size = size.load(memory_order_acquire);
            if (current_size == 0) return false;
            
            item = buffer[readPos];
            return true;
        }
        
        // Original method
        size_t available() const {
            return size.load(memory_order_acquire);
        }
        
        // Enhanced: Add method to get remaining capacity
        size_t remaining_capacity() const {
            return capacity - available();
        }
        
        // Enhanced: Clear the buffer
        void clear() {
            lock_guard<mutex> lock(mtx);
            readPos = writePos;
            size.store(0, memory_order_release);
        }
        
        // Enhanced: Register a callback for dropped items (when using Rotate policy)
        using DropCallback = function<void(size_t count)>;
        void set_drop_callback(DropCallback callback) {
            lock_guard<mutex> lock(mtx);
            dropCallback = callback;
        }

        size_t getCapacity() const {
            return capacity;
        }
        
    private:
        vector<T> buffer;
        size_t capacity;
        size_t readPos{0};
        size_t writePos{0};
        atomic<size_t> size{0};
        WritePolicy policy;
        DropCallback dropCallback;
        
        mutable mutex mtx;
        condition_variable cv;
    };

} // namespace tools::utils


#ifdef TEST

#include "Test.hpp"

using namespace tools::utils;

// Test basic initialization 
void test_RingBuffer_constructor_normal() {
    RingBuffer<int> rb(10, RingBuffer<int>::WritePolicy::Reject);
    assert(rb.available() == 0);
}

// Test constructor with invalid capacity
void test_RingBuffer_constructor_invalid() {
    bool exception_thrown = false;
    try {
        RingBuffer<int> rb(0, RingBuffer<int>::WritePolicy::Reject); // Less than required minimum of 1
    } catch (const exception& e) {
        exception_thrown = true;
    }
    assert(exception_thrown && "Invalid capacity should throw exception");
}

// Test writing data to buffer
void test_RingBuffer_write_basic() {
    RingBuffer<int> rb(10, RingBuffer<int>::WritePolicy::Reject);
    int data[] = {1, 2, 3, 4, 5};
    bool result = rb.write(data, 5);
    assert(result == true);
    assert(rb.available() == 5);
}

// Test reading data from buffer
void test_RingBuffer_read_basic() {
    RingBuffer<int> rb(10, RingBuffer<int>::WritePolicy::Reject);
    int data[] = {1, 2, 3, 4, 5};
    rb.write(data, 5);
    
    int readData[5] = {0};
    size_t bytesRead = rb.read(readData, 5);
    
    assert(bytesRead == 5);
    assert(rb.available() == 0);
    for (int i = 0; i < 5; i++) {
        assert(readData[i] == data[i]);
    }
}

// Test partial read
void test_RingBuffer_read_partial() {
    RingBuffer<int> rb(10, RingBuffer<int>::WritePolicy::Reject);
    int data[] = {1, 2, 3, 4, 5};
    rb.write(data, 5);
    
    int readData[3] = {0};
    size_t bytesRead = rb.read(readData, 3);
    
    assert(bytesRead == 3);
    assert(rb.available() == 2);
    for (int i = 0; i < 3; i++) {
        assert(readData[i] == data[i]);
    }
}

// Test reading from an empty buffer
void test_RingBuffer_read_empty() {
    RingBuffer<int> rb(10, RingBuffer<int>::WritePolicy::Reject);
    int readData[5] = {0};
    size_t bytesRead = rb.read(readData, 5);
    
    assert(bytesRead == 0);
    assert(rb.available() == 0);
}

// Test writing to a full buffer
void test_RingBuffer_write_full() {
    RingBuffer<int> rb(5, RingBuffer<int>::WritePolicy::Reject); // Usable capacity is 5 (full capacity)
    int data[] = {1, 2, 3, 4, 5};
    bool result = rb.write(data, 5);
    assert(result == true);

    int moreData[] = {6};
    result = rb.write(moreData, 1);
    assert(result == false); // No space for more elements
}

// Test writing wrap-around behavior
void test_RingBuffer_write_wraparound() {
    RingBuffer<int> rb(5, RingBuffer<int>::WritePolicy::Reject);
    int data1[] = {1, 2};
    rb.write(data1, 2);
    
    int readData[2] = {0};
    rb.read(readData, 2); // Read everything out
    
    int data2[] = {3, 4, 5};
    bool result = rb.write(data2, 3);
    assert(result == true);
    assert(rb.available() == 3);
    
    int readResult[3] = {0};
    size_t bytesRead = rb.read(readResult, 3);
    assert(bytesRead == 3);
    for (int i = 0; i < 3; i++) {
        assert(readResult[i] == data2[i]);
    }
}

// Test reading wrap-around behavior
void test_RingBuffer_read_wraparound() {
    RingBuffer<int> rb(6, RingBuffer<int>::WritePolicy::Reject); // Actual capacity is 5
    
    // Fill buffer partially and read it out to advance readPos
    int data1[] = {1, 2, 3};
    rb.write(data1, 3);
    int temp[3];
    rb.read(temp, 3);
    
    // Now write data that will wrap around
    int data2[] = {4, 5, 6, 7, 8};
    bool result = rb.write(data2, 5);
    assert(result == true);
    
    // Read it back and verify
    int readResult[5] = {0};
    size_t bytesRead = rb.read(readResult, 5);
    assert(bytesRead == 5);
    for (int i = 0; i < 5; i++) {
        assert(readResult[i] == data2[i]);
    }
}

void test_RingBuffer_thread_safety() {
    RingBuffer<int> rb(1000, RingBuffer<int>::WritePolicy::Reject);
    
    thread writer([&rb]() {
        for (int i = 0; i < 100; i++) {
            int data = i;
            while (!rb.write(&data, 1)) {
                this_thread::yield();
            }
        }
    });
    
    thread reader([&rb]() {
        int sum = 0;
        int expected_sum = 0;
        for (int i = 0; i < 100; i++) {
            expected_sum += i;
            int data = 0;
            while (rb.read(&data, 1) == 0) {
                this_thread::yield();
            }
            sum += data;
        }
        assert(sum == expected_sum && "Sum mismatch");
    });
    
    writer.join();
    reader.join();
}

// Test with different data types (string)
void test_RingBuffer_different_types() {
    RingBuffer<string> rb(5, RingBuffer<string>::WritePolicy::Reject);
    string data[] = {"hello", "world"};
    bool result = rb.write(data, 2);
    assert(result == true);
    
    string readData[2];
    size_t count = rb.read(readData, 2);
    assert(count == 2);
    assert(readData[0] == "hello");
    assert(readData[1] == "world");
}

// ---!@#

void test_RingBuffer_write_overwrite() {
    RingBuffer<int> rb(5, RingBuffer<int>::WritePolicy::Rotate); // Enable overwrite mode
    int data1[] = {1, 2, 3, 4, 5};
    rb.write(data1, 5); // Fill buffer

    int data2[] = {6, 7, 8}; 
    bool result = rb.write(data2, 3); // Should overwrite {1,2,3}

    assert(result == true);
    assert(rb.available() == 5);

    int readData[5] = {0};
    rb.read(readData, 5);

    // Expect overwritten values: {4,5,6,7,8}
    int expected[] = {4, 5, 6, 7, 8};
    for (int i = 0; i < 5; i++) assert(readData[i] == expected[i]);
}

void test_RingBuffer_read_after_overwrite() {
    RingBuffer<int> rb(4, RingBuffer<int>::WritePolicy::Reset); // Overwrite mode enabled
    int data1[] = {10, 20};
    rb.write(data1, 2);

    int data2[] = {30, 40, 50, 60}; // Overwrites {10,20}
    rb.write(data2, 4);

    int readData[4] = {0};
    rb.read(readData, 4);

    int expected[] = {30, 40, 50, 60};
    for (int i = 0; i < 4; i++) assert(readData[i] == expected[i]);
}

void test_RingBuffer_blocking_read() {
    RingBuffer<int> rb(5, RingBuffer<int>::WritePolicy::Reject);

    thread writer([&]() {
        this_thread::sleep_for(chrono::milliseconds(100));
        int data = 42;
        rb.write(&data, 1);
    });

    int readData = 0;
    size_t bytesRead = rb.read(&readData, 1, true); // Blocking read

    assert(bytesRead == 1);
    assert(readData == 42);

    writer.join();
}

void test_RingBuffer_blocking_read_timeout() {
    RingBuffer<int> rb(5, RingBuffer<int>::WritePolicy::Reject);

    auto start = chrono::steady_clock::now();
    int readData = 0;
    size_t bytesRead = rb.read(&readData, 1, true); // Should block

    auto end = chrono::steady_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    assert(bytesRead == 0); // No data was written
    assert(duration >= 100); // Ensure it waited
}

void test_RingBuffer_overwrite_full_write() {
    RingBuffer<int> rb(3, RingBuffer<int>::WritePolicy::Reset);
    int data1[] = {1, 2, 3};
    rb.write(data1, 3);

    int data2[] = {4, 5, 6};
    rb.write(data2, 3); // Completely overwrites

    int readData[3] = {0};
    rb.read(readData, 3);

    int expected[] = {4, 5, 6};
    for (int i = 0; i < 3; i++) assert(readData[i] == expected[i]);
}

void test_RingBuffer_rotate_mode() {
    // In rotate mode, new writes overwrite only the number of items necessary.
    RingBuffer<int> rb(4, RingBuffer<int>::WritePolicy::Rotate);
    
    int data1[] = {1, 2, 3};
    rb.write(data1, 3);  // Buffer now holds: {1,2,3}
    
    int readData[2] = {0};
    rb.read(readData, 2); // Read two items -> leftover: {3}
    
    int data2[] = {4, 5, 6}; 
    rb.write(data2, 3);   // Should overwrite two oldest items.
                          // Expected final buffer content:
                          // Old unread item 3 is kept if not overwritten,
                          // then new items are added.
                          // Calculation:
                          // current_size was 1, count=3, so excess = (1+3)-4 = 0, 
                          // actually no excess in this scenario. 
                          // Therefore final order: starting at readPos, we get {3,4,5,6}.
    
    int finalRead[4] = {0};
    size_t bytesRead = rb.read(finalRead, 4);
    // Expect the full 4 items.
    int expected[] = {3, 4, 5, 6};
    assert(bytesRead == 4);
    for (int i = 0; i < 4; i++)
        assert(finalRead[i] == expected[i]);
}

void test_RingBuffer_reset_mode() {
    RingBuffer<int> rb(3, RingBuffer<int>::WritePolicy::Rotate);
    
    int data1[] = {10, 20, 30};
    rb.write(data1, 3);  // Buffer holds: {10,20,30}
    
    int readData[2] = {0};
    rb.read(readData, 2); // Read two -> leftover: {30}
    
    int data2[] = {40, 50, 60}; 
    rb.write(data2, 3);   // Since new write would overflow (1+3>3),
                          // Reset mode discards the old data.
                          // Final buffer should only have {40,50,60}.
    
    int finalRead[3] = {0};
    size_t bytesRead = rb.read(finalRead, 3);
    int expected[] = {40, 50, 60};
    assert(bytesRead == 3);
    for (int i = 0; i < 3; i++)
        assert(finalRead[i] == expected[i]);
}

void test_RingBuffer_drop_mode() {
    RingBuffer<int> rb(4, RingBuffer<int>::WritePolicy::Reject);
    
    int data1[] = {100, 200};
    bool result = rb.write(data1, 2); // Should succeed.
    assert(result == true);
    
    int data2[] = {300, 400, 500}; // 2 + 3 = 5, which exceeds capacity.
    result = rb.write(data2, 3);
    assert(result == false); // Write is rejected.
    
    // Ensure buffer still holds only the first two elements.
    int finalRead[2] = {0};
    size_t bytesRead = rb.read(finalRead, 2);
    int expected[] = {100, 200};
    assert(bytesRead == 2);
    for (int i = 0; i < 2; i++)
        assert(finalRead[i] == expected[i]);
}

void test_RingBuffer_blocking_read_rotate() {
    RingBuffer<int> rb(4, RingBuffer<int>::WritePolicy::Rotate);

    thread writer([&]() {
        this_thread::sleep_for(chrono::milliseconds(100));
        int data = 42;
        rb.write(&data, 1);
    });

    int readData = 0;
    size_t bytesRead = rb.read(&readData, 1, true, 1000); // blocking read with 1s timeout
    assert(bytesRead == 1);
    assert(readData == 42);
    writer.join();
}

void test_RingBuffer_interleaved_overwrite() {
    RingBuffer<int> rb(4, RingBuffer<int>::WritePolicy::Rotate);
    int data1[] = {1, 2, 3};
    rb.write(data1, 3);
    
    int readData[2] = {0};
    rb.read(readData, 2); // Read first two values
    
    int data2[] = {4, 5, 6};
    rb.write(data2, 3); // Overwrites
    
    int finalRead[4] = {0};
    rb.read(finalRead, 4);

    int expected[] = {3, 4, 5, 6};
    assert(finalRead[0] == expected[0]);
    assert(finalRead[1] == expected[1]);
    assert(finalRead[2] == expected[2]);
    assert(finalRead[3] == expected[3]); // Only reads available data
}

// Test write_one method
void test_RingBuffer_write_one_basic() {
    RingBuffer<int> rb(5, RingBuffer<int>::WritePolicy::Reject);
    bool result = rb.write_one(42);
    
    assert(result == true && "write_one should succeed with enough space");
    assert(rb.available() == 1 && "Buffer should contain one item after write_one");
    
    int readData = 0;
    rb.read_one(readData);
    assert(readData == 42 && "Read value should match written value");
}

// Test write_one when buffer is full
void test_RingBuffer_write_one_full() {
    RingBuffer<int> rb(3, RingBuffer<int>::WritePolicy::Reject);
    
    // Fill the buffer
    for (int i = 0; i < 3; i++) {
        rb.write_one(i);
    }
    
    // Try to write one more item
    bool result = rb.write_one(99);
    assert(result == false && "write_one should fail when buffer is full with Reject policy");
    
    // Verify buffer content remains unchanged
    int readData[3] = {0};
    size_t bytesRead = rb.read(readData, 3);
    assert(bytesRead == 3 && "Should read 3 items");
    for (int i = 0; i < 3; i++) {
        assert(readData[i] == i && "Buffer content should be unchanged");
    }
}

// Test try_write_all with enough space
void test_RingBuffer_try_write_all_enough_space() {
    RingBuffer<int> rb(5, RingBuffer<int>::WritePolicy::Reject);
    int data[] = {1, 2, 3};
    
    size_t unwritten = rb.try_write_all(data, 3);
    assert(unwritten == 0 && "All items should be written with enough space");
    assert(rb.available() == 3 && "Buffer should contain all written items");
    
    int readData[3] = {0};
    size_t bytesRead = rb.read(readData, 3);
    assert(bytesRead == 3 && "Should read all 3 items");
    for (int i = 0; i < 3; i++) {
        assert(readData[i] == data[i] && "Read values should match written values");
    }
}

// Test try_write_all with Reject policy when not enough space
void test_RingBuffer_try_write_all_reject_policy() {
    RingBuffer<int> rb(3, RingBuffer<int>::WritePolicy::Reject);
    int data[] = {1, 2, 3, 4, 5};
    
    size_t unwritten = rb.try_write_all(data, 5);
    assert(unwritten == 5 && "None should be written with Reject policy when not enough space");
    assert(rb.available() == 0 && "Buffer should remain empty");
}

// Test try_write_all with Rotate policy when not enough space
void test_RingBuffer_try_write_all_rotate_policy() {
    RingBuffer<int> rb(3, RingBuffer<int>::WritePolicy::Rotate);
    // First write some data that will be partially overwritten
    rb.write_one(10);
    rb.write_one(20);
    
    int data[] = {30, 40, 50};
    size_t unwritten = rb.try_write_all(data, 3);
    
    assert(unwritten == 0 && "All items should be written with Rotate policy");
    assert(rb.available() == 3 && "Buffer should be full");
    
    int readData[3] = {0};
    size_t bytesRead = rb.read(readData, 3);
    
    // With Rotate policy, the buffer should have discarded the oldest items
    int expected[] = {30, 40, 50}; // The first item (10) should be discarded
    for (int i = 0; i < 3; i++) {
        assert(readData[i] == expected[i] && "Buffer content should reflect rotation policy");
    }
}

// Test try_write_all with Reset policy
void test_RingBuffer_try_write_all_reset_policy() {
    RingBuffer<int> rb(3, RingBuffer<int>::WritePolicy::Reset);
    
    // First fill buffer with some data
    rb.write_one(10);
    rb.write_one(20);
    
    // Try to write more data than remaining space
    int data[] = {30, 40, 50};
    size_t unwritten = rb.try_write_all(data, 3);
    
    assert(unwritten == 0 && "All items should be written with Reset policy");
    assert(rb.available() == 3 && "Buffer should contain only new data");
    
    int readData[3] = {0};
    size_t bytesRead = rb.read(readData, 3);
    
    int expected[] = {30, 40, 50};
    for (int i = 0; i < 3; i++) {
        assert(readData[i] == expected[i] && "Buffer content should be reset and contain only new data");
    }
}

// Test read_one basic functionality
void test_RingBuffer_read_one_basic() {
    RingBuffer<int> rb(5, RingBuffer<int>::WritePolicy::Reject);
    rb.write_one(42);
    
    int readData = 0;
    bool result = rb.read_one(readData);
    
    assert(result == true && "read_one should succeed when data is available");
    assert(readData == 42 && "Read value should match written value");
    assert(rb.available() == 0 && "Buffer should be empty after reading");
}

// Test read_one from empty buffer
void test_RingBuffer_read_one_empty() {
    RingBuffer<int> rb(5, RingBuffer<int>::WritePolicy::Reject);
    
    int readData = 0;
    bool result = rb.read_one(readData);
    
    assert(result == false && "read_one should fail when buffer is empty");
    assert(rb.available() == 0 && "Buffer should remain empty");
}

// Test peek_one basic functionality
void test_RingBuffer_peek_one_basic() {
    RingBuffer<int> rb(5, RingBuffer<int>::WritePolicy::Reject);
    rb.write_one(42);
    
    int peekData = 0;
    bool result = rb.peek_one(peekData);
    
    assert(result == true && "peek_one should succeed when data is available");
    assert(peekData == 42 && "Peeked value should match written value");
    assert(rb.available() == 1 && "Buffer should still contain data after peeking");
    
    // Verify we can still read the same value
    int readData = 0;
    rb.read_one(readData);
    assert(readData == 42 && "Read value should match peeked value");
}

// Test peek_one from empty buffer
void test_RingBuffer_peek_one_empty() {
    RingBuffer<int> rb(5, RingBuffer<int>::WritePolicy::Reject);
    
    int peekData = 0;
    bool result = rb.peek_one(peekData);
    
    assert(result == false && "peek_one should fail when buffer is empty");
}

// Test remaining_capacity
void test_RingBuffer_remaining_capacity_basic() {
    RingBuffer<int> rb(5, RingBuffer<int>::WritePolicy::Reject);
    
    size_t initial_capacity = rb.remaining_capacity();
    assert(initial_capacity == 5 && "Initial remaining capacity should be total capacity");
    
    rb.write_one(1);
    size_t after_one_write = rb.remaining_capacity();
    assert(after_one_write == 4 && "Remaining capacity should decrease after write");
    
    rb.write_one(2);
    size_t after_two_writes = rb.remaining_capacity();
    assert(after_two_writes == 3 && "Remaining capacity should decrease further after another write");
    
    // Read one item
    int temp;
    rb.read_one(temp);
    size_t after_read = rb.remaining_capacity();
    assert(after_read == 4 && "Remaining capacity should increase after read");
}

// Test clear method
void test_RingBuffer_clear_basic() {
    RingBuffer<int> rb(5, RingBuffer<int>::WritePolicy::Reject);
    
    int data[] = {1, 2, 3};
    rb.write(data, 3);
    assert(rb.available() == 3 && "Buffer should contain written data");
    
    rb.clear();
    assert(rb.available() == 0 && "Buffer should be empty after clear");
    assert(rb.remaining_capacity() == 5 && "Remaining capacity should be total capacity after clear");
    
    // Verify we can't read any data
    int readData;
    bool result = rb.read_one(readData);
    assert(result == false && "Should not be able to read after clear");
}

// Test drop callback with Rotate policy
void test_RingBuffer_drop_callback() {
    RingBuffer<int> rb(3, RingBuffer<int>::WritePolicy::Rotate);
    
    size_t dropped_count = 0;
    rb.set_drop_callback([&dropped_count](size_t count) {
        dropped_count = count;
    });
    
    // Fill the buffer
    int data1[] = {1, 2, 3};
    rb.write(data1, 3);
    
    // Write more data to cause rotation
    int data2[] = {4, 5};
    rb.write(data2, 2);
    
    // Check if callback was called with correct count
    assert(dropped_count == 2 && "Drop callback should report 2 items dropped");
    
    // Verify buffer content
    int readData[3] = {0};
    size_t bytesRead = rb.read(readData, 3);
    
    int expected[] = {3, 4, 5};
    for (int i = 0; i < 3; i++) {
        assert(readData[i] == expected[i] && "Buffer content should reflect items after rotation");
    }
}

// Register all tests
TEST(test_RingBuffer_constructor_normal);
TEST(test_RingBuffer_constructor_invalid);
TEST(test_RingBuffer_write_basic);
TEST(test_RingBuffer_read_basic);
TEST(test_RingBuffer_read_partial);
TEST(test_RingBuffer_read_empty);
TEST(test_RingBuffer_write_full);
TEST(test_RingBuffer_write_wraparound);
TEST(test_RingBuffer_read_wraparound);
TEST(test_RingBuffer_thread_safety);
TEST(test_RingBuffer_different_types);
TEST(test_RingBuffer_write_overwrite);
TEST(test_RingBuffer_read_after_overwrite);
TEST(test_RingBuffer_blocking_read);
TEST(test_RingBuffer_blocking_read_timeout);
TEST(test_RingBuffer_overwrite_full_write);
TEST(test_RingBuffer_rotate_mode);
TEST(test_RingBuffer_reset_mode);
TEST(test_RingBuffer_drop_mode);
TEST(test_RingBuffer_blocking_read_rotate);
TEST(test_RingBuffer_interleaved_overwrite);
TEST(test_RingBuffer_write_one_basic);
TEST(test_RingBuffer_write_one_full);
TEST(test_RingBuffer_try_write_all_enough_space);
TEST(test_RingBuffer_try_write_all_reject_policy);
TEST(test_RingBuffer_try_write_all_rotate_policy);
TEST(test_RingBuffer_try_write_all_reset_policy);
TEST(test_RingBuffer_read_one_basic);
TEST(test_RingBuffer_read_one_empty);
TEST(test_RingBuffer_peek_one_basic);
TEST(test_RingBuffer_peek_one_empty);
TEST(test_RingBuffer_remaining_capacity_basic);
TEST(test_RingBuffer_clear_basic);
TEST(test_RingBuffer_drop_callback);

#endif