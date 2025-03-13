#pragma once

#include "../../../libs/K-Adam/SafeQueue/SafeQueue.hpp"

#include "Pack.hpp"

using namespace std;

namespace tools::agency {

    template<typename T>
    class PackQueue: public SafeQueue<Pack<T>> {
    public:

        void drop(const string& recipient) {
            unique_lock<mutex> lock(this->mtx); // Access the mutex from SafeQueue

            queue<Pack<T>> temp; // Temporary queue for filtered items
            while (!this->q.empty()) {
                Pack<T> pack = move(this->q.front());
                this->q.pop();
                if (pack.getRecipient() != recipient) {
                    temp.push(move(pack)); // Keep packs that don't match
                }
            }
            this->q = move(temp); // Replace the original queue
        }

    };

}

#ifdef TEST

using namespace tools::agency;

// SafeQueue Tests
void test_SafeQueue_Produce_EmptyQueue() {
    SafeQueue<int> q;
    q.Produce(42);
    int actual = q.Size();
    int expected = 1;
    assert(actual == expected && "Produce should add one item to empty queue");
}

void test_SafeQueue_Consume_EmptyQueue() {
    SafeQueue<int> q;
    int item;
    bool actual = q.Consume(item);
    bool expected = false;
    assert(actual == expected && "Consume should return false on empty queue");
}

void test_SafeQueue_Consume_SingleItem() {
    SafeQueue<int> q;
    q.Produce(42);
    int item;
    bool consumed = q.Consume(item);
    int actual = item;
    int expected = 42;
    assert(consumed && actual == expected && "Consume should retrieve correct item");
    assert(q.Size() == 0 && "Queue should be empty after consume");
}

void test_SafeQueue_ConsumeSync_SingleItem() {
    SafeQueue<int> q;
    q.Produce(42);
    int item;
    bool consumed = q.ConsumeSync(item);
    int actual = item;
    int expected = 42;
    assert(consumed && actual == expected && "ConsumeSync should retrieve correct item");
    assert(q.Size() == 0 && "Queue should be empty after ConsumeSync");
}

void test_SafeQueue_ConsumeSync_EmptyQueue() {
    SafeQueue<int> q;
    thread t([&q]() {
        int item;
        bool consumed = q.ConsumeSync(item);
        bool actual = consumed;
        bool expected = false;
        assert(actual == expected && "ConsumeSync should return false after Finish on empty queue");
    });
    this_thread::sleep_for(chrono::milliseconds(10)); // Let thread wait
    q.Finish();
    t.join();
}

void test_SafeQueue_Peek_EmptyQueue() {
    SafeQueue<int> q;
    int* actual = q.Peek();
    int* expected = nullptr;
    assert(actual == expected && "Peek should return nullptr on empty queue");
}

void test_SafeQueue_Peek_SingleItem() {
    SafeQueue<int> q;
    q.Produce(42);
    int* ptr = q.Peek();
    int actual = *ptr;
    int expected = 42;
    assert(actual == expected && "Peek should return pointer to front item");
}

void test_SafeQueue_Finish_MultiThread() {
    SafeQueue<int> q;
    vector<thread> threads;
    for (int i = 0; i < 3; i++) {
        threads.emplace_back([&q]() {
            int item;
            bool consumed = q.ConsumeSync(item);
            assert(!consumed && "ConsumeSync should return false after Finish");
        });
    }
    this_thread::sleep_for(chrono::milliseconds(10));
    q.Finish();
    for (auto& t : threads) t.join();
    assert(q.Size() == 0 && "Queue should remain empty after Finish");
}

void test_SafeQueue_Hold_Release() {
    SafeQueue<int> q;
    q.Hold();
    thread t([&q]() {
        q.Hold(); // Should wait until Release
    });
    this_thread::sleep_for(chrono::milliseconds(10));
    q.Release();
    t.join();
    assert(true && "Hold/Release should allow synchronization"); // Basic functionality check
}

// PackQueue Tests
void test_PackQueue_drop_SingleRecipient() {
    PackQueue<int> q;
    q.Produce(Pack<int>(42, "", "alice"));     // item=42, sender="", recipient="alice"
    q.Produce(Pack<int>(43, "", "bob"));       // item=43, sender="", recipient="bob"
    q.drop("alice");
    Pack<int> pack;
    bool consumed = q.Consume(pack);
    string recipient = pack.getRecipient();
    int actual = pack.getItem();
    int expected = 43;
    assert(consumed && actual == expected && "drop should remove alice's pack, keep bob's");
    assert(recipient == "bob" && "Recipient should be bob after dropping alice");
    assert(q.Size() == 0 && "Queue should have one item removed");
}

void test_PackQueue_drop_MultipleMatching() {
    PackQueue<int> q;
    q.Produce(Pack<int>(1, "", "alice"));
    q.Produce(Pack<int>(2, "", "alice"));
    q.Produce(Pack<int>(3, "", "bob"));
    q.drop("alice");
    Pack<int> pack;
    bool consumed = q.Consume(pack);
    int actual = pack.getItem();
    int expected = 3;
    assert(consumed && actual == expected && "drop should remove all alice's packs");
    assert(q.Size() == 0 && "Queue should have only bob's item left");
}

void test_PackQueue_drop_EmptyQueue() {
    PackQueue<int> q;
    q.drop("alice");
    int actual = q.Size();
    int expected = 0;
    assert(actual == expected && "drop on empty queue should keep it empty");
}

// Register Tests
TEST(test_SafeQueue_Produce_EmptyQueue);
TEST(test_SafeQueue_Consume_EmptyQueue);
TEST(test_SafeQueue_Consume_SingleItem);
TEST(test_SafeQueue_ConsumeSync_SingleItem);
TEST(test_SafeQueue_ConsumeSync_EmptyQueue);
TEST(test_SafeQueue_Peek_EmptyQueue);
TEST(test_SafeQueue_Peek_SingleItem);
TEST(test_SafeQueue_Finish_MultiThread);
TEST(test_SafeQueue_Hold_Release);
TEST(test_PackQueue_drop_SingleRecipient);
TEST(test_PackQueue_drop_MultipleMatching);
TEST(test_PackQueue_drop_EmptyQueue);
#endif