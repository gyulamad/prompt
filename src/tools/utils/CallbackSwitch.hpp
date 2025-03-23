#pragma once

#include <functional>

#include "foreach.hpp"

#include "CallbackSwitch.hpp"

using namespace std;

namespace tools::utils {

    template<typename DataT>
    class CallbackSwitch: public Switch<DataT> {
    public:
        using cb = function<void(CallbackSwitch& swtch, DataT* data)>;
        using Switch<DataT>::Switch;
        virtual void on() { 
            Switch<DataT>::on();
            foreach (cb_ons, [this](pair<cb, DataT*>& p) { p.first(*this, p.second); });
        };
        virtual void off() { 
            Switch<DataT>::off();
            foreach (cb_offs, [this](pair<cb, DataT*>& p) { p.first(*this, p.second); });
        };
        virtual void cb_on(cb cb, DataT* data = nullptr) { cb_ons.push_back(pair(cb, data)); }
        virtual void cb_off(cb cb, DataT* data = nullptr) { cb_offs.push_back(pair(cb, data)); }
    protected:
        vector<pair<cb, DataT*>> cb_ons;
        vector<pair<cb, DataT*>> cb_offs;
    };

}
