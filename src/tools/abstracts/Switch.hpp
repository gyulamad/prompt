#pragma once

namespace tools::abstracts {
    
    enum SwitchState { ON = 0, OFF };

    class Switch {
    public:
        Switch(SwitchState state = OFF): state(state) {}
        virtual ~Switch() { off(); }
        virtual void on() { 
            state = ON;
        };
        virtual void off() { 
            state = OFF;
        };
        virtual bool is_on() const { return state == ON; };
        virtual bool is_off() const { return state == OFF; };
    protected:
        SwitchState state;
    };

#ifdef TEST

// #include "../utils/Test.hpp"

using namespace std;
using namespace tools::abstracts;

void test_Switch_constructor_default() {
    Switch sw;
    assert(sw.is_off() && "Default constructor should initialize to OFF");
}

void test_Switch_constructor_with_state() {
    Switch sw(SwitchState::ON);
    assert(sw.is_on() && "Constructor with state ON should initialize to ON");
}

void test_Switch_on() {
    Switch sw;
    sw.on();
    assert(sw.is_on() && "on() should set the state to ON");
}

void test_Switch_off() {
    Switch sw(SwitchState::ON);
    sw.off();
    assert(sw.is_off() && "off() should set the state to OFF");
}

void test_Switch_is_on() {
    Switch sw;
    assert(!sw.is_on() && "is_on() should return false when the state is OFF");
    sw.on();
    assert(sw.is_on() && "is_on() should return true when the state is ON");
}

void test_Switch_is_off() {
    Switch sw(SwitchState::ON);
    sw.off();
    assert(sw.is_off() && "off() should set the state to OFF");
}

TEST(test_Switch_constructor_default);
TEST(test_Switch_constructor_with_state);
TEST(test_Switch_on);
TEST(test_Switch_off);
TEST(test_Switch_is_on);
TEST(test_Switch_is_off);

#endif

}
