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
        // void set_visible(bool visible) { this->visible = visible; }
        void incRecs() { recs++; }
        void decRecs() { recs--; }
        int getRecs() const { return recs; }
        string getView(bool muted, bool loud, float threshold_pc, float vol_pc, float rmax, float rms, bool in_progress) {
            // if (!visible) return "";

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
        // bool visible = false;

        // ---- roller ----

        int rollnxt = 6000;
        const char* roller = "|/-\\";
        int recs = 0;

        // -----------------
    };    

}

#ifdef TEST

using namespace tools::voice;

void test_MicView_incDecRecs() {
    MicView mv;
    mv.incRecs();
    assert(mv.getRecs() == 1 && "Increment test failed");
    mv.decRecs();
    assert(mv.getRecs() == 0 && "Decrement test failed");
    mv.incRecs(); mv.incRecs();
    assert(mv.getRecs() == 2 && "Two increments should reach 2");
}

void test_MicView_getView_muted() {
    MicView mv;
    string result = mv.getView(true, false, 0.5f, 0.3f, 0.0f, 0.0f, false);
    assert(result.find("MUTE") != string::npos && "Muted view should contain 'MUTE'");
    assert(result.find("VOL:50.00/30.00%") != string::npos && "Volume values incorrect");
    assert(result.find(" | ") == string::npos && "Progress should not be present");
}

void test_MicView_getView_loud() {
    MicView mv;
    string result = mv.getView(false, true, 0.5f, 0.3f, 0.0f, 0.0f, false);
    assert(result.find(ANSI_FMT(ANSI_FMT_C_RED, "●") + "REC" ) != string::npos && "Loud view should contain ●REC");
    assert(result.find("VOL:50.00/30.00%") != string::npos && "Volume values incorrect");
}

void test_MicView_getView_progress() {
    MicView mv;
    mv.incRecs();
    mv.incRecs();
    string result = mv.getView(false, false, 0.5f, 0.3f, 0.0f, 0.0f, true);
    assert(
        (
            result.find(" | 2") != string::npos || 
            result.find(" / 2") != string::npos || 
            result.find(" - 2") != string::npos || 
            result.find(" \\ 2") != string::npos 
        )
        && "Progress indicator not found"
    );
}

void test_MicView_getView_rms() {
    MicView mv;
    string result = mv.getView(false, false, 0.5f, 0.3f, 1.2f, 0.45f, false);
    assert(result.find("RMS:1.20/0.45") != string::npos && "RMS values should be displayed");
}

TEST(test_MicView_incDecRecs);
TEST(test_MicView_getView_muted);
TEST(test_MicView_getView_loud);
TEST(test_MicView_getView_progress);
TEST(test_MicView_getView_rms);
#endif
