#pragma once

/*

Usages example:

int main() {
    Rotary rotary;

    // Clear default animations and add custom ones (optional)
    rotary.clearAnims();
    rotary.addAnim(RotaryFrames({ "Processing" }, 1)); // Prefix
    rotary.addAnim(RotaryFrames({ "⏳", "⌛" }, 1));    // Spinner
    rotary.addAnim(RotaryFrames({ "😊", "😎", "🤖" }, 3)); // Emojis

    // Simulate a long-running process
    for (int i = 0; i < 100; ++i) {
        rotary.tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    rotary.done("Finished!"); // Use custom done text
    return 0;
}

*/

#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <algorithm>
#include <mutex>
#include <stdexcept>

using namespace std;

namespace tools {

    class RotaryFrames {
    public:
        RotaryFrames(const vector<string>& frames, size_t speed_divider)
            : frames(frames), speed_divider(speed_divider), index(0) {
            if (frames.empty()) {
                throw invalid_argument("Frames cannot be empty.");
            }
            if (speed_divider == 0) {
                throw invalid_argument("Speed divider cannot be zero.");
            }
        }

        string getCurrentFrame() const {
            return frames[index];
        }

        void update(size_t ticks) {
            if (ticks % speed_divider == 0) {
                index = (index + 1) % frames.size();
            }
        }

        size_t getMaxFrameLength() const {
            size_t max_length = 0;
            for (const auto& frame : frames) {
                if (frame.length() > max_length) {
                    max_length = frame.length();
                }
            }
            return max_length;
        }

    private:
        vector<string> frames;
        size_t speed_divider;
        size_t index;
    };

    class Rotary {
    public:
        // Constructor with optional custom animations
        Rotary(const vector<RotaryFrames>& custom_animations = {}) {
            if (custom_animations.empty()) {
                // Use default animations if no custom animations are provided
                setDefaultAnimations();
            } else {
                // Use custom animations
                for (const auto& animation : custom_animations) {
                    addAnim(animation);
                }
            }
        }

        void addAnim(const RotaryFrames& animation) {
            lock_guard<::mutex> lock(mutex);
            animations.push_back(animation);
            updateLongestOutputLength(animation);
        }

        void clearAnims() {
            lock_guard<::mutex> lock(mutex);
            animations.clear();
            longest_output_length = 0;
        }

        void tick() {
            lock_guard<::mutex> lock(mutex);

            string status;
            for (const auto& animation : animations) {
                status += animation.getCurrentFrame() + " ";
            }

            cout << "\r" << status << flush;

            for (auto& animation : animations) {
                animation.update(ticks);
            }

            ticks++;
        }

        void clear(const string& done_text = "\r\033[2K") {
            lock_guard<::mutex> lock(mutex);
            cout << "\r\033[2K" << flush;
            longest_output_length = 0; // TODO: lonest output may not needed anymore...
        }

        static vector<RotaryFrames> getDefaultAnimations() {
            return {
                RotaryFrames({ "/", "-", "\\", "|" }, 1), // Sticks
                // RotaryFrames({ "🤔", "🧠", "💭" }, 2),     // Emojis
                // RotaryFrames({ ".", "..", "..." }, 3)     // Dots
            };
        }

    private:
        vector<RotaryFrames> animations;
        size_t ticks = 0;
        size_t longest_output_length = 0;
        ::mutex mutex;

        void setDefaultAnimations() {
            animations = getDefaultAnimations();
            for (const auto& animation : animations) {
                updateLongestOutputLength(animation);
            }
        }

        void updateLongestOutputLength(const RotaryFrames& animation) {
            size_t frame_length = animation.getMaxFrameLength() + 1; // +1 for the space
            longest_output_length += frame_length;
        }
    };

}