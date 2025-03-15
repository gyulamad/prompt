#pragma once

#include "../../cmd/Commander.hpp"
#include "../../voice/STT.hpp"

using namespace tools::cmd;
using namespace tools::voice;

namespace tools::agency::agents {
    
    template<typename T, typename TranscriberT>
    class UserAgent: public Agent<T> {
    public:

        UserAgent(PackQueue<T>& queue, Agency<T>& agency, Commander* commander = nullptr, STT<TranscriberT>* stt = nullptr): 
            Agent<T>(queue, "user"), agency(agency), commander(commander), stt(stt) {}

        void setVoiceInput(bool state) { voice = state; }

        void tick() override {
            if (voice) {
                // TODO: voice input mode
                if (!stt) {
                    cout << "Speach to text adapter is missing, switching to text mode.." << endl;
                    setVoiceInput(false);
                    return;
                }

                stt->start();

            } else {

                // text input mode
                T input;
                if (commander) {
                    CommandLine& cline = commander->get_command_line_ref();
                    if (cline.is_exited()) return;
                    input = cline.readln();
                    if (cline.is_exited()) {
                        this->exit();
                        return;
                    }
                } else cin >> input;
                if (trim(input).empty()) return;
                else if (str_starts_with(input, "/")) commander->run_command(&agency, input); // TODO: add is_command(input) as a command matcher (regex or callback fn) instead just test for "/" 
                else this->send("echo", input); // TODO: forward to default agent (now it's echo agent but should be parametized)

            }
        }

        Commander* getCommanderPtr() { return commander; }
        
    private:
        bool voice = false;
        Agency<T>& agency;
        Commander* commander = nullptr;
        STT<TranscriberT>* stt = nullptr;
    };
    
}