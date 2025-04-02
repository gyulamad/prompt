#pragma once

using namespace tools::voice;

// Mock SpeechListener
class MockSpeechListenerWithPublic_noise_cb : public SpeechListener {
public:
    using SpeechListener::SpeechListener;
    virtual ~MockSpeechListenerWithPublic_noise_cb() {}
    static void public_noise_cb(
        void* listener, 
        float vol_pc, 
        float threshold_pc, 
        float rmax, 
        float rms, 
        bool is_noisy, 
        vector<float>& buffer,
        bool muted
    ) {
        SpeechListener::noise_cb(
            listener, 
            vol_pc, 
            threshold_pc, 
            rmax, 
            rms, 
            is_noisy, 
            buffer,
            muted
        );
    }
};