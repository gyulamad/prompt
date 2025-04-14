#pragma once

#include <string>
#include <functional>

using namespace std;

namespace tools::str {

    class FrameTokenParser {
    public:
        FrameTokenParser() {}
        virtual ~FrameTokenParser() {}

        string parse(
            const string& chunk,
            const string& start_token,
            const string& stop_token,
            function<void(const string&)> cb
        ) {
            string clean = "";
            for (size_t i = 0; i < chunk.size(); i++) {
                buffer += chunk[i];
                if (in_tokens) inner += chunk[i];

                bool in_start_token = false;
                for (size_t i = 1; i <= start_token.size(); i++)
                    if (buffer.ends_with(start_token.substr(0, i))) {
                        in_start_token = true;
                        break;
                    }
                bool in_stop_token = false;
                for (size_t i = 1; i <= stop_token.size(); i++)
                    if (buffer.ends_with(stop_token.substr(0, i))) {
                        in_stop_token = true;
                        break;
                    }
                if (!in_start_token && !in_stop_token && !in_tokens) clean += chunk[i];

                if (buffer.ends_with(start_token)) {
                    in_tokens = true;
                }
                if (buffer.ends_with(stop_token)) {
                    in_tokens = false;
                    cb(inner.substr(0, inner.size() - stop_token.size()));
                    clean += start_token + inner;
                    inner = "";
                }
            }
            return clean;
        }

        void reset() {
            buffer = "";
            in_tokens = false;
            inner = "";
        }

        string buffer;
        bool in_tokens;
        string inner;
    };
    
}
