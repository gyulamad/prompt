#pragma once

#include <iostream>
#include <atomic>
#include <thread>
#include <functional>
#include <fstream> // For file I/O
#include <portaudio.h>

#include "../RingBuffer.hpp"

using namespace std;
using namespace tools;

namespace tools::voice {


    class VoiceRecorder {
    public:
        using AudioCallback = function<void(const float* data, size_t count)>;

        VoiceRecorder(
            double sampleRate, 
            unsigned long framesPerBuffer, 
            size_t bufferSeconds
        ):
            sampleRate(sampleRate),
            framesPerBuffer(framesPerBuffer),
            ringBuffer(sampleRate * bufferSeconds)
        {

            PaError err = Pa_Initialize();
            // cout << "err: " << err << endl;

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
            cout << "DEBUG: VoiceRecorder worker thread start..." << endl;
            workerThread = thread(&VoiceRecorder::stream_thread, this);
        }

        ~VoiceRecorder() {
            running = false;
            //if (workerThread.joinable()) 
                workerThread.join();
            Pa_Terminate();
        }         

        size_t available() const {
            return ringBuffer.available();
        }

        void read_audio(float* dest, size_t maxSamples) {
            ringBuffer.read(dest, maxSamples);
        }

        // Save recorded data as a raw PCM file
        static void save_as_pcm(const string& filename, const vector<float>& buffer) {
            ofstream file(filename, ios::binary);
            if (!file) {
                throw runtime_error("Failed to open file: " + filename);
            }

            // Write the raw PCM data (float samples)
            file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(float));

            if (!file) {
                throw runtime_error("Failed to write PCM data to file: " + filename);
            }

            // cout << "PCM data saved to " << filename << " (" << buffer.size() << " samples)" << endl;
        }

    private:
        PaStream* paStream = nullptr;
        RingBuffer<float> ringBuffer;
        double sampleRate;
        unsigned long framesPerBuffer;
        atomic<bool> running{true};
        thread workerThread;

        static int pa_callback(
            const void* input, 
            void* output,
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
                cerr << "Voice recorder buffer overflow" << endl;
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