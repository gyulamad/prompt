#pragma once

#include <string>

#include "../utils/ANSI_FMT.hpp"
#include "../str/set_precision.hpp"

using namespace std;
using namespace tools::utils;
using namespace tools::str;

namespace tools::voice {

    class MicView {
    public:
        MicView() {}
        virtual ~MicView() {}
        void incRecs() { recs++; }
        void decRecs() { recs--; }
        string getView(bool muted, bool loud, float threshold_pc, float vol_pc, float rmax, float rms, bool in_progress) {

            string out = "";

                    
            // REC status
            out += (
                muted 
                    ? ANSI_FMT(ANSI_FMT_C_BLACK, "MUTE") 
                    : loud 
                        ? ANSI_FMT(ANSI_FMT_C_RED, "●") + "REC" 
                        : "?MIC"
            );


            // RMS volume
            //out += "V:" + set_precision(threshold_pc * 100, 2) + "/" + set_precision(rms * 100, 2) + " \t[";
            out += "[" + string(muted ? ANSI_FMT_C_BLACK : ANSI_FMT_C_GREEN);
            double step = 0.2;
            double i = threshold_pc;
            for (; i < 1; i += step) {
                if (muted) out += "◦";
                else if (vol_pc > i) out += "•";
                else out += "◦"; //"·•◦";
            }
            out += string(ANSI_FMT_RESET) + "] VOL:" + set_precision(threshold_pc * 100, 2) + "/" + set_precision(vol_pc * 100, 2) + "%";
            out += " RMS:" + set_precision(rmax, 2) + "/" + set_precision(rms, 2);


            // progress roller
            rollnxt++;
            int at = rollnxt%4;
            char c = roller[at];
            string roll = "";
            roll += c;
            out += (in_progress
                ? " " + roll + " " + to_string(recs)
                : "");

            return out;
        }
    private:

        // ---- roller ----

        int rollnxt = 6000;
        const char* roller = "|/-\\";
        int recs = 0;

        // -----------------
    };    

}