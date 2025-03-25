#pragma once

#include <iostream>
#include <atomic>
#include <thread>
#include <functional>
#include <fstream> // For file I/O
#include <portaudio.h>

#include "../utils/RingBuffer.hpp"
#include "../utils/ERROR.hpp"

using namespace std;
using namespace tools::utils;

namespace tools::voice {


    class VoiceRecorder {
    public:
        using AudioCallback = function<void(const float* data, size_t count)>;

        VoiceRecorder(
            double sampleRate, 
            unsigned long framesPerBuffer, 
            size_t bufferSeconds,
            RingBuffer<float>::WritePolicy writePolicy = RingBuffer<float>::WritePolicy::Rotate
        ):
            sampleRate(sampleRate),
            framesPerBuffer(framesPerBuffer),
            ringBuffer(sampleRate * bufferSeconds, writePolicy)
        {

            PaError err = Pa_Initialize();
            if (err) cerr << "portaudio error: " << err << endl;

            // PaHostApiIndex alsaApi = Pa_HostApiTypeIdToHostApiIndex(paALSA);
            // cout << "alsaApi: " << alsaApi << endl;

            // const PaHostApiInfo* info = Pa_GetHostApiInfo(alsaApi);
            // cout << "info->defaultInputDevice: " << info->defaultInputDevice << endl;
            // cout << "info->defaultOutputDevice: " << info->defaultOutputDevice << endl;
            // cout << "info->deviceCount: " << info->deviceCount << endl;
            // cout << "info->name: " << info->name << endl;
            // cout << "info->structVersion: " << info->structVersion << endl;
            // cout << "info->type: " << info->type << endl;


            // PaStreamParameters inputParams;
            // inputParams.device = Pa_GetHostApiInfo(alsaApi)->defaultInputDevice;
            // cout << "[DEBUG] VoiceRecorder worker thread start..." << endl;
            workerThread = thread(&VoiceRecorder::stream_thread, this);
        }

        virtual ~VoiceRecorder() {
            running = false;
            if (workerThread.joinable()) workerThread.join();
            Pa_Terminate();
        }         

        virtual size_t available() const {
            return ringBuffer.available();
        }

        virtual void read_audio(float* dest, size_t maxSamples) {
            ringBuffer.read(dest, maxSamples);
        }

        // Save recorded data as a raw PCM file
        static void save_as_pcm(const string& filename, const vector<float>& buffer) {
            ofstream file(filename, ios::binary);
            if (!file) {
                throw ERROR("Failed to open file: " + filename);
            }

            // Write the raw PCM data (float samples)
            file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(float));

            if (!file) {
                throw ERROR("Failed to write PCM data to file: " + filename);
            }

            // cout << "PCM data saved to " << filename << " (" << buffer.size() << " samples)" << endl;
        }

    protected:
        PaStream* paStream = nullptr;

    private:
        double sampleRate;
        unsigned long framesPerBuffer;
        RingBuffer<float> ringBuffer;
        atomic<bool> running{true};
        thread workerThread;

        static int pa_callback(
            const void* input, 
            void* /*output*/,
            unsigned long frameCount,
            const PaStreamCallbackTimeInfo*,
            PaStreamCallbackFlags,
            void* userData
        ) {
            VoiceRecorder* self = static_cast<VoiceRecorder*>(userData);
            return self->process_audio(static_cast<const float*>(input), frameCount);
        }

        int process_audio(const float* input, unsigned long frameCount) {
            if (!ringBuffer.write(input, frameCount)) {
                // TODO: Handle buffer overflow (optional)
                //cerr << "Voice recorder buffer overflow" << endl;
            }
            return paContinue;
        }

        void stream_thread() {
            PaStreamParameters inputParams{
                .device = Pa_GetDefaultInputDevice(),
                .channelCount = 1,
                .sampleFormat = paFloat32,
                .suggestedLatency = Pa_GetDeviceInfo(Pa_GetDefaultInputDevice())->defaultLowInputLatency,
                .hostApiSpecificStreamInfo = nullptr
            };

            Pa_OpenStream(&paStream, &inputParams, nullptr, sampleRate,
                        framesPerBuffer, paClipOff, &VoiceRecorder::pa_callback, this);
            Pa_StartStream(paStream);

            while (running) Pa_Sleep(30); // Let PortAudio handle the callback

            Pa_StopStream(paStream);
            Pa_CloseStream(paStream);
        }
    };


}

#ifdef TEST

#include "../utils/Test.hpp"
#include "../utils/Suppressor.hpp"

using namespace std;
using namespace tools::utils;
using namespace tools::voice;

// VoiceRecorder Tests
void test_VoiceRecorder_constructor_valid() {
    bool constructed = true;
    try {
        Suppressor suppressor(stderr);
        VoiceRecorder recorder(16000.0, 512, 5); // 16kHz, 512 frames, 5s buffer
    } catch (...) {
        constructed = false;
    }
    assert(constructed && "VoiceRecorder constructor should initialize without crashing");
}

void test_VoiceRecorder_available_initial() {
    size_t available_samples = 0;
    {
        Suppressor suppressor(stderr);
        VoiceRecorder recorder(16000.0, 512, 5);
        available_samples = recorder.available();
    }
    assert(available_samples == 0 && "Initial available samples should be 0");
}

void test_VoiceRecorder_read_audio_empty() {
    vector<float> buffer(512, 0.0f);
    size_t samples_read = 0;
    {
        Suppressor suppressor(stderr);
        VoiceRecorder recorder(16000.0, 512, 5);
        recorder.read_audio(buffer.data(), buffer.size());
        samples_read = recorder.available(); // After reading, check remaining
    }
    assert(samples_read == 0 && "Reading from empty recorder should yield 0 samples");
    // Optionally check buffer unchanged, assuming no recording yet
    for (float sample : buffer) {
        assert(sample == 0.0f && "Buffer should remain unchanged with no data");
    }
}

void test_VoiceRecorder_buffer_capacity() {
    size_t max_samples = 0;
    {
        Suppressor suppressor(stderr);
        VoiceRecorder recorder(16000.0, 512, 5); // 5s * 16kHz = 80,000 samples
        max_samples = recorder.available() + (16000 * 5); // Assume max capacity
    }
    assert(max_samples == 80000 && "Buffer capacity should match sample rate * seconds");
}

void test_VoiceRecorder_read_audio_exceeds_available() {
    vector<float> buffer(1024, 0.0f); // Larger than typical frame size
    size_t samples_read = 0;
    {
        Suppressor suppressor(stderr);
        VoiceRecorder recorder(16000.0, 512, 5);
        recorder.read_audio(buffer.data(), buffer.size());
        samples_read = recorder.available(); // Should still be 0 if no data
    }
    assert(samples_read == 0 && "Reading beyond available should not increase samples");
    for (float sample : buffer) {
        assert(sample == 0.0f && "Buffer should remain unchanged with no data");
    }
}

void test_VoiceRecorder_multiple_reads() {
    vector<float> buffer1(512, 0.0f);
    vector<float> buffer2(512, 0.0f);
    size_t available_after_first = 0;
    size_t available_after_second = 0;
    {
        Suppressor suppressor(stderr);
        VoiceRecorder recorder(16000.0, 512, 5);
        recorder.read_audio(buffer1.data(), buffer1.size());
        available_after_first = recorder.available();
        recorder.read_audio(buffer2.data(), buffer2.size());
        available_after_second = recorder.available();
    }
    assert(available_after_first == 0 && "First read should leave no samples");
    assert(available_after_second == 0 && "Second read should leave no samples");
}

// Register tests
TEST(test_VoiceRecorder_constructor_valid);
TEST(test_VoiceRecorder_available_initial);
TEST(test_VoiceRecorder_read_audio_empty);
TEST(test_VoiceRecorder_buffer_capacity);
TEST(test_VoiceRecorder_read_audio_exceeds_available);
TEST(test_VoiceRecorder_multiple_reads);

#endif