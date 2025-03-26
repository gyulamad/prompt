#pragma once

#include "TTS.hpp"

namespace tools::voice {

    class ESpeakTTSAdapter: public TTS { // TODO: make TTS as abstract and implement espeak here
    public:
        using TTS::TTS;
    };

}