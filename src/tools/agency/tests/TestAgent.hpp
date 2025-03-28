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

    void send(const string& recipient, const T& item) {
        Agent<T>::send(recipient, item);
    }

    void send(const vector<string>& recipients, const T& item) {
        Agent<T>::send(recipients, item);
    }


    bool isClosing() const { return this->closing; }

};