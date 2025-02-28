#pragma once

#include <mutex>

using namespace std;

namespace tools {

    template<typename T>
    class RingBuffer {
    public:
        RingBuffer(size_t capacity): buffer(capacity), capacity(capacity) {
            if (capacity < 2) 
                throw std::invalid_argument("Capacity must be at least 2");
        }
    
        bool write(const T* data, size_t count) {
            std::lock_guard<std::mutex> lock(mtx);
            size_t current_size = size.load(std::memory_order_acquire);
            if (current_size + count > capacity) return false;
    
            size_t start = writePos;
            for (size_t i = 0; i < count; ++i) {
                buffer[(writePos + i) % capacity] = data[i];
            }
            writePos = (writePos + count) % capacity;
            size.store(current_size + count, std::memory_order_release);
            return true;
        }
    
        size_t read(T* dest, size_t maxCount) {
            std::lock_guard<std::mutex> lock(mtx);
            size_t current_size = size.load(std::memory_order_acquire);
            size_t toRead = std::min(current_size, maxCount);
    
            size_t start = readPos;
            for (size_t i = 0; i < toRead; ++i) {
                dest[i] = buffer[(readPos + i) % capacity];
            }
            readPos = (readPos + toRead) % capacity;
            size.store(current_size - toRead, std::memory_order_release);
            return toRead;
        }
    
        size_t available() const {
            return size.load(std::memory_order_acquire);
        }
    
    private:
        std::vector<T> buffer;
        size_t capacity;
        size_t readPos{0};
        size_t writePos{0};
        std::atomic<size_t> size{0}; // Atomic size for thread safety
        mutable std::mutex mtx; // Mutex for critical sections
    };

}

#ifdef TEST

using namespace tools;

// Test basic initialization 
void test_RingBuffer_constructor_normal() {
    RingBuffer<int> rb(10);
    assert(rb.available() == 0);
}

// Test constructor with invalid capacity
void test_RingBuffer_constructor_invalid() {
    bool exception_thrown = false;
    try {
        RingBuffer<int> rb(1); // Less than required minimum of 2
    } catch (const invalid_argument& e) {
        exception_thrown = true;
    }
    assert(exception_thrown && "Invalid capacity should throw exception");
}

// Test writing data to buffer
void test_RingBuffer_write_basic() {
    RingBuffer<int> rb(10);
    int data[] = {1, 2, 3, 4, 5};
    bool result = rb.write(data, 5);
    assert(result == true);
    assert(rb.available() == 5);
}

// Test reading data from buffer
void test_RingBuffer_read_basic() {
    RingBuffer<int> rb(10);
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
    RingBuffer<int> rb(10);
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
    RingBuffer<int> rb(10);
    int readData[5] = {0};
    size_t bytesRead = rb.read(readData, 5);
    
    assert(bytesRead == 0);
    assert(rb.available() == 0);
}

// Test writing to a full buffer
void test_RingBuffer_write_full() {
    RingBuffer<int> rb(5); // Usable capacity is 5 (full capacity)
    int data[] = {1, 2, 3, 4, 5};
    bool result = rb.write(data, 5);
    assert(result == true);

    int moreData[] = {6};
    result = rb.write(moreData, 1);
    assert(result == false); // No space for more elements
}

// Test writing wrap-around behavior
void test_RingBuffer_write_wraparound() {
    RingBuffer<int> rb(5);
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
    RingBuffer<int> rb(6); // Actual capacity is 5
    
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
    RingBuffer<int> rb(1000);
    
    std::thread writer([&rb]() {
        for (int i = 0; i < 100; i++) {
            int data = i;
            while (!rb.write(&data, 1)) {
                std::this_thread::yield();
            }
        }
    });
    
    std::thread reader([&rb]() {
        int sum = 0;
        int expected_sum = 0;
        for (int i = 0; i < 100; i++) {
            expected_sum += i;
            int data = 0;
            while (rb.read(&data, 1) == 0) {
                std::this_thread::yield();
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
    RingBuffer<string> rb(5);
    string data[] = {"hello", "world"};
    bool result = rb.write(data, 2);
    assert(result == true);
    
    string readData[2];
    size_t count = rb.read(readData, 2);
    assert(count == 2);
    assert(readData[0] == "hello");
    assert(readData[1] == "world");
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
#endif