#pragma once

// #include "../Agent.hpp"

// using namespace tools::agency;

template<typename T>
class TestAgent: public Agent<T> {
public:
    using Agent<T>::Agent;

    string type() const override { return "test"; }

    bool handled = false;
    void handle(const string&, const T&) override {
        handled = true;
    }

    void testSend(const string& recipient, const T& item) {
        this->setRecipients({recipient});
        this->send(item);
    }
    // void send(const string& recipient, const T& item) {
    //     Agent<T>::send(recipient, item);
    // }

    void testSend(const vector<string>& recipients, const T& item) {
        this->setRecipients(recipients);
        this->send(item);
    }
    // void send(const vector<string>& recipients, const T& item) {
    //     Agent<T>::send(recipients, item);
    // }


    bool isClosing() const { return this->closing; }

};