#pragma once

#include "../../../libs/K-Adam/SafeQueue/SafeQueue.hpp"

#include "Pack.hpp"

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
                if (pack.recipient != recipient) {
                    temp.push(move(pack)); // Keep packs that don't match
                }
            }
            this->q = move(temp); // Replace the original queue
        }

    };

}

#ifdef TEST

#include "../utils/Test.hpp"
#include "tests/helpers.hpp"

using namespace tools::agency;

// Test drop on empty queue
void test_PackQueue_drop_empty() {
    PackQueue<string> pq;
    pq.drop("bob");
    auto actual_contents = queue_to_vector(pq);
    assert(actual_contents.empty() && "Drop on empty queue should leave it empty");
}

// Test drop with single non-matching item
void test_PackQueue_drop_single_no_match() {
    PackQueue<string> pq;
    Pack<string> pack("alice", "bob", "hello");
    pq.Produce(move(pack));
    pq.drop("charlie"); // Shouldn't match "bob"
    auto actual_contents = queue_to_vector(pq);
    assert(actual_contents.size() == 1 && "Drop should keep non-matching item");
    assert(actual_contents[0].sender == "alice" && "Sender should be 'alice'");
    assert(actual_contents[0].recipient == "bob" && "Recipient should be 'bob'");
    assert(actual_contents[0].item == "hello" && "Item should be 'hello'");
}

// Test drop with single matching item
void test_PackQueue_drop_single_match() {
    PackQueue<string> pq;
    Pack<string> pack("alice", "bob", "hello");
    pq.Produce(move(pack));
    pq.drop("bob"); // Matches "bob"
    auto actual_contents = queue_to_vector(pq);
    assert(actual_contents.empty() && "Drop should remove matching item");
}

// Test drop with multiple items, mixed matches
void test_PackQueue_drop_multiple_mixed() {
    PackQueue<string> pq;
    pq.Produce(Pack<string>("alice", "bob", "hello"));
    pq.Produce(Pack<string>("bob", "charlie", "hi"));
    pq.Produce(Pack<string>("charlie", "bob", "hey"));
    pq.drop("bob"); // Should remove packs with recipient "bob"
    
    auto actual_contents = queue_to_vector(pq);
    vector<Pack<string>> expected_contents = {Pack<string>("bob", "charlie", "hi")};
    
    assert(actual_contents.size() == 1 && "Drop should remove only matching recipients");
    assert(actual_contents[0].sender == expected_contents[0].sender && "Sender mismatch");
    assert(actual_contents[0].recipient == expected_contents[0].recipient && "Recipient mismatch");
    assert(actual_contents[0].item == expected_contents[0].item && "Item mismatch");
    // Optionally use vector_equal if available: assert(vector_equal(actual_contents, expected_contents) && "Contents should match expected");
}

// Register tests
TEST(test_PackQueue_drop_empty);
TEST(test_PackQueue_drop_single_no_match);
TEST(test_PackQueue_drop_single_match);
TEST(test_PackQueue_drop_multiple_mixed);

#endif