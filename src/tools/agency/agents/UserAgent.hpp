#pragma once

#include "../../str/trim.hpp"
#include "../../str/set_precision.hpp"
#include "../../regx/regx_match.hpp"
#include "../../cmd/Commander.hpp"
#include "../../voice/STT.hpp"
#include "../../voice/STTSwitch.hpp"
#include "../../voice/WhisperTranscriberSTTSwitch.hpp"
#include "../../utils/InputPipeInterceptor.hpp"
#include "../../containers/array_shift.hpp"
#include "../Agent.hpp"
#include "../Agency.hpp"

#include "UserAgentInterface.hpp"

using namespace tools::str;
using namespace tools::regx;
using namespace tools::cmd;
using namespace tools::voice;
using namespace tools::utils;
using namespace tools::agency;

namespace tools::agency::agents {
    
    template<typename T>
    class UserAgent: public Agent<T> {
    public:

        // TODO: make configurable:
        atomic<bool> text_input_echo = true;

        UserAgent(
            Owns& owns,
            Worker<T>* agency,
            PackQueue<T>& queue,
            const string& name,
            UserAgentInterface<T>& interface
        ): 
            Agent<T>(owns, agency, queue, name), 
            interface(interface)
        {
            interface.setUser(this);
        }

        virtual ~UserAgent() {}

        UserAgentInterface<T>& getInterfaceRef() { return interface; }


        string type() const override { return "user"; }

        void tick() override {
            if (this->isClosing() || this->agency->isClosing()) return;
            T input;
            if (!inputs.empty()) input = array_shift(inputs);
            else {
                sleep_ms(100);
                if (interface.readln(input)) {
                    this->exit();
                    return;
                }
                inputs.push_back(input);
                return;
            }
            if (trim(input).empty()) return;
            if (text_input_echo) {
                interface.clearln();
                interface.println(input);
            }
            Commander& commander = interface.getCommanderRef();
            if (commander.isPrefixed(input)) {
                commander.runCommand(this, input); 
            } else {
                onInput(input);
            }
        }

        void onInput(T input) {
            // TODO: <-- !!! voice speach interrupted here?? (or just pause?) !!!
            interface.getTTSRef().speak_stop();
            // TTS& tts = interface.getTTSRef();
            // if (tts.is_speaking()) tts.speak_stop();

            // interface.getCommanderRef().getCommandLineRef().setPromptVisible(false);
            this->send(input);
        }

        void handle(const string& sender, const T& item) override {
            // DEBUG("Incoming message from '" + sender + "'");
            interface.println(sender + ": " + item);
            // interface.getCommanderRef().getCommandLineRef().setPromptVisible(true); // TODO: not here but here: [[[---STOP---]]]
        }


        // ------ startup & batch -----

        void batch(const vector<string>& inputs) {
            if (inputs.empty()) return;

        //     // Commander& commander = interface.getCommanderRef();
        //     // commander.getCommandLineRef().setPromptVisible(false);
            foreach (inputs, [&](const string& input) {
                this->inputs.push_back(input);
        //         if (echo) interface.println(input);
                
        //         // if (!commander.runCommand(this->agency, input)) return FE_BREAK;
        //         // return FE_CONTINUE;
            });
        //     // commander.getCommandLineRef().setPromptVisible(true);
            
        }

    private:
        UserAgentInterface<T>& interface;
        vector<string> inputs = {};
    };
    
}