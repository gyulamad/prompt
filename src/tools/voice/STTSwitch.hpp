#pragma once

#include "../abstracts/Switch.hpp"
#include "STT.hpp"

using namespace tools::abstracts;

namespace tools::voice {

    class STTSwitch: public Switch {
    public:
        using Switch::Switch;
        virtual STT* getSttPtr() { return Switch::is_on() ? stt : nullptr; }
    protected:
        STT* stt = nullptr;
    };

}
