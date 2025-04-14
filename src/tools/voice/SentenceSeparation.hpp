#pragma once

#include <string>
#include <vector>

using namespace std;

namespace tools::voice {

    class SentenceSeparation {
    public:
        SentenceSeparation(const vector<string>& separators): separators(separators) {}
        virtual ~SentenceSeparation() {}

        // Returns the position of the sentence end, or string::npos if not found
        virtual size_t findSentenceEnd(const string& text, size_t start_pos) const = 0;

    protected:
        vector<string> separators;
    };
    
}