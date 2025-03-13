#pragma once

// MinimalAgent for testing unimplemented handle
template<typename T>
class MinimalAgent : public Agent<T> {
public:
    MinimalAgent(PackQueueHolder<T>& agency, const string& name = "", long ms = 0)
        : Agent<T>(agency, name, ms) {}

    // Expose thread for testing
    thread& getThreadRef() { return this->t; }
    // No handle override, so base class UNIMP_THROWS applies
};
