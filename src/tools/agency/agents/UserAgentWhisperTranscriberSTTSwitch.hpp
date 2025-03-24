#pragma once

#include "../../voice/WhisperTranscriberSTTSwitch.hpp"

using namespace tools::voice;

namespace tools::agency::agents {

    template<typename T>
    class UserAgent;

    template<typename T>
    class UserAgentWhisperTranscriberSTTSwitch: public WhisperTranscriberSTTSwitch<UserAgent<T>> {
    public:
        using WhisperTranscriberSTTSwitch<UserAgent<T>>::WhisperTranscriberSTTSwitch;
    };

}
