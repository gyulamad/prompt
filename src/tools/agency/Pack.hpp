#pragma once

#include "../utils/Streamable.hpp"

using namespace tools::utils;

namespace tools::agency {

    template<typename T>
    class Pack {
    public:
        Pack(const string& sender = "", const string& recipient = "", T item = T()): sender(sender), recipient(recipient), item(item) {}
        string sender;
        string recipient;
        T item;

        void dump(ostream& os = cout) const {
            os << "Pack[sender: " << sender << ", recipient: " << recipient << ", item: ";
            if constexpr (has_ostream<T>::value) os << item;
            else os << "(unprintable)";
            os << "]";
        }
    };

}

#ifdef TEST

// #include "../utils/Test.hpp"

using namespace tools::agency;

// Dummy non-streamable type for testing
struct NonStreamable {
    int x;
    NonStreamable(int val = 0) : x(val) {}
};

// Test default constructor
void test_Pack_constructor_default() {
    Pack<string> pack;
    auto actual_sender = pack.sender;
    auto actual_recipient = pack.recipient;
    auto actual_item = pack.item;
    assert(actual_sender == "" && "Default sender should be empty");
    assert(actual_recipient == "" && "Default recipient should be empty");
    assert(actual_item == "" && "Default item should be empty string");
}

// Test constructor with arguments
void test_Pack_constructor_with_args() {
    Pack<string> pack("alice", "bob", "hello");
    auto actual_sender = pack.sender;
    auto actual_recipient = pack.recipient;
    auto actual_item = pack.item;
    assert(actual_sender == "alice" && "Sender should be 'alice'");
    assert(actual_recipient == "bob" && "Recipient should be 'bob'");
    assert(actual_item == "hello" && "Item should be 'hello'");
}

// Test dump with streamable type (string)
void test_Pack_dump_streamable() {
    Pack<string> pack("alice", "bob", "hello");
    auto actual_output = capture_cout([&]() { pack.dump(); });
    auto expected_output = "Pack[sender: alice, recipient: bob, item: hello]";
    assert(actual_output == expected_output && "Dump output should match expected format for streamable type");
}

// Test dump with non-streamable type
void test_Pack_dump_non_streamable() {
    Pack<NonStreamable> pack("alice", "bob", NonStreamable(42));
    auto actual_output = capture_cout([&]() { pack.dump(); });
    auto expected_output = "Pack[sender: alice, recipient: bob, item: (unprintable)]";
    assert(actual_output == expected_output && "Dump output should match expected format for non-streamable type");
}

// Register tests
TEST(test_Pack_constructor_default);
TEST(test_Pack_constructor_with_args);
TEST(test_Pack_dump_streamable);
TEST(test_Pack_dump_non_streamable);

#endif