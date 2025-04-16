#pragma once

#include "../../utils/ERROR.hpp"

class MockWorker : public Worker<string> {
public:
    MockWorker(
        Owns& owns,
        Worker<string>* agency,
        PackQueue<string>& queue,
        const string& name
    ) : Worker<string>(owns, agency, queue, name), throwInTick(false) {}

    void handle(const string& /*sender*/, const string& /*item*/) override {}
    string type() const override { return "MockWorker"; }

    void tick() override {
        if (throwInTick) throw ERROR("Tick failed!");
        Worker<string>::tick();
    }

    virtual void hoops(const string& errmsg = "") {
        hoopsCalled = true;
        hoopsErrorMessage = errmsg;
        closing = true;
    }

    bool throwInTick = false;
    bool hoopsCalled = false;
    string hoopsErrorMessage = "<unset>";
};
