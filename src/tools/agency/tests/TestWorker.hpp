#pragma once

// #include "../Worker.hpp"

// using namespace tools::agency;

template<typename T>
class TestWorker: public Worker<T> {
public:
    using Worker<T>::Worker;

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
    //     Worker<T>::send(recipient, item);
    // }

    void testSend(const vector<string>& recipients, const T& item) {
        this->setRecipients(recipients);
        this->send(item);
    }
    // void send(const vector<string>& recipients, const T& item) {
    //     Worker<T>::send(recipients, item);
    // }


    bool isClosing() const { return this->closing; }

};