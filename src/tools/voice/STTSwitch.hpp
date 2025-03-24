#pragma once

#include "../utils/Switch.hpp"
#include "STT.hpp"

using namespace tools::utils;

namespace tools::voice {

    template<typename DataT>
    class STTSwitch: public Switch<DataT> {
    public:
        using Switch<DataT>::Switch;
        virtual STT* getSttPtr() { return Switch<DataT>::is_on() ? stt : nullptr; }
    protected:
        STT* stt = nullptr;
    };

}