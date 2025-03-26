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

}
