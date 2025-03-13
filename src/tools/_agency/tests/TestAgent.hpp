#pragma once

template<typename T>
class TestAgent : public Agent<T> {
public:
    TestAgent(PackQueueHolder<T>& agency, const string& name = "", long ms = 0)
        : Agent<T>(agency, name, ms) {}

    bool handle(const string& sender, const T& item) override {
        lock_guard<mutex> lock(data_mtx); // Protect shared test data
        last_sender = sender;
        last_item = item;
        handled_count++;
        return continue_running;
    }

    bool tick() override {
        return tick_continue;
    }


    thread& getThreadRef() { return this->t; }

    template<typename... Args>
    void publicSend(Args&&... args) { this->send(forward<Args>(args)...); }

    void publicSendMultiple(T item, const vector<string>& recipients) { this->send(item, recipients); }

    template<typename... Args>
    void publicExit(Args&&... args) { this->exit(forward<Args>(args)...); }

    bool isDying() const { return this->dying; }

    // Test helpers
    string last_sender;
    T last_item;
    int handled_count = 0;
    bool continue_running = true;
    bool tick_continue = true;
    mutex data_mtx; // For thread-safe test data access
};
