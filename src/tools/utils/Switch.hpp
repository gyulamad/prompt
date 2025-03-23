#pragma once

// #include <functional>

// #include "foreach.hpp"

// using namespace std;

namespace tools::utils {
    
    enum SwitchState { ON = 0, OFF };

    template<typename DataT>
    class Switch {
    public:
        // using cb = function<void(Switch& swtch, DataT* data)>;
        Switch(SwitchState state = OFF): state(state) {}
        virtual ~Switch() { off(); }
        virtual void on() { 
            state = ON;
            // foreach (cb_ons, [this](pair<cb, DataT*>& p) { p.first(*this, p.second); });
        };
        virtual void off() { 
            state = OFF;
            // foreach (cb_offs, [this](pair<cb, DataT*>& p) { p.first(*this, p.second); });
        };
        virtual bool is_on() const { return state == ON; };
        virtual bool is_off() const { return state == OFF; };
        // virtual void cb_on(cb cb, DataT* data = nullptr) { cb_ons.push_back(pair(cb, data)); }
        // virtual void cb_off(cb cb, DataT* data = nullptr) { cb_offs.push_back(pair(cb, data)); }
    protected:
        SwitchState state;
        // vector<pair<cb, DataT*>> cb_ons;
        // vector<pair<cb, DataT*>> cb_offs;
    };

}
