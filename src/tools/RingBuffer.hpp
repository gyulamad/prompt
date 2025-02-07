#pragma once

using namespace std;

namespace tools {

    template<typename T>
    class RingBuffer {
    public:
        RingBuffer(size_t capacity): buffer(capacity), capacity(capacity) {
            if (capacity < 2) 
                throw invalid_argument("Capacity must be at least 2");
        }

        bool write(const T* data, size_t count) {
            size_t writePos = this->writePos.load(memory_order_relaxed);
            size_t readPos = this->readPos.load(memory_order_acquire);

            size_t avail = (readPos > writePos) 
                ? (readPos - writePos - 1) 
                : (capacity - writePos + readPos - 1);
            if (avail < count) return false; // Not enough space

            for (size_t i = 0; i < count; ++i) buffer[(writePos + i) % capacity] = data[i];

            this->writePos.store((writePos + count) % capacity, memory_order_release);
            return true;
        }

        size_t read(T* dest, size_t maxCount) {
            size_t readPos = this->readPos.load(memory_order_relaxed);
            size_t writePos = this->writePos.load(memory_order_acquire);

            size_t avail = (writePos >= readPos) 
                ? (writePos - readPos) 
                : (capacity - readPos + writePos);
            size_t toRead = min(avail, maxCount);

            for (size_t i = 0; i < toRead; ++i) dest[i] = buffer[(readPos + i) % capacity];

            this->readPos.store((readPos + toRead) % capacity, memory_order_release);
            return toRead;
        }

        size_t available() const {
            size_t writePos = this->writePos.load(memory_order_acquire);
            size_t readPos = this->readPos.load(memory_order_acquire);
            return (writePos >= readPos) 
                ? (writePos - readPos) 
                : (capacity - readPos + writePos);
        }

    private:
        vector<T> buffer;
        size_t capacity;
        atomic<size_t> readPos{0};
        atomic<size_t> writePos{0};
    };

}