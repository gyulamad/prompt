#pragma once

#include <vector>

#include "../ERROR.hpp"

using namespace std;
using namespace tools;

namespace tools::voice {

    class Transcriber {
    private:
        bool inProgress = false;
        
    protected:

        void preTranscribe() {
            inProgress = true;
        }

        void postTranscribe() {
            inProgress = false;
        }

    public:
        Transcriber(const string& conf, const char* lang = nullptr) {}

        virtual ~Transcriber() {}

        bool isInProgress() const {
            return inProgress;
        }    

        virtual string transcribe(const vector<float>& audio_data) UNIMP
    };

}