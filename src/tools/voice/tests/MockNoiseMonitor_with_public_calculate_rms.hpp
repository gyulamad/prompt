#pragma once

using namespace tools::voice;

class MockNoiseMonitor_with_public_calculate_rms: public NoiseMonitor {
public:
    using NoiseMonitor::NoiseMonitor;
    virtual ~MockNoiseMonitor_with_public_calculate_rms() {}
    float public_calculate_rms(const vector<float>& buffer) {
        return calculate_rms(buffer);
    }
};