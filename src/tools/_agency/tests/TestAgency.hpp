#pragma once

// Custom TestAgency to override setup and loop
template<typename T>
class TestAgency : public Agency<T> {
public:
    TestAgency(PackQueue<T>& queue, long ms = 100) : Agency<T>(queue, ms) {}
    
    void setup() override {
        setup_called = true;
    }

    void loop() override {
        loop_count++;
    }


    long getMs() const { return this->ms; }

    vector<Agent<T>*>& getAgentsRef() { return this->agents; }

    bool setup_called = false;
    int loop_count = 0;
};
