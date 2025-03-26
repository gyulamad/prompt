#pragma once

#include "../../str/trim.hpp"
#include "../../str/set_precision.hpp"
#include "../../regx/regx_match.hpp"
#include "../../cmd/Commander.hpp"
#include "../../voice/STT.hpp"
#include "../../voice/STTSwitch.hpp"
#include "../../voice/WhisperTranscriberSTTSwitch.hpp"
#include "../../utils/InputPipeInterceptor.hpp"
#include "../Agent.hpp"
#include "../Agency.hpp"

#include "UserAgentWhisperCommanderInterface.hpp"

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
            PackQueue<T>& queue,
            const string& name, 
            Agency<T>& agency, 
            UserAgentWhisperCommanderInterface<T>& interface,
            vector<string> recipients = { "echo" } // TODO: to config, and have to be able to change/see message package targets
        ): 
            Agent<T>(queue, name), 
            agency(agency),
            interface(interface),
            recipients(recipients)
        {
            interface.setUser(this);
        }

        virtual ~UserAgent() {}

        UserAgentWhisperCommanderInterface<T>& getInterfaceRef() { return interface; }

        void tick() override {
            if (agency.isClosing()) {
                sleep_ms(100);
                return;
            }
            T input;
            if (interface.readln(input)) this->exit();
            if (trim(input).empty()) return;
            else if (str_starts_with(input, "/")) { // TODO: add is_command(input) as a command matcher (regex or callback fn) instead just test for "/"
                interface.getCommanderRef().runCommand(&agency, input); 
            } else {
                if (text_input_echo) interface.println(input, true, false);
                onInput(input);
            }
        }

        void onInput(T input) {
            this->send(recipients, input);
        }

    private:
        Agency<T>& agency;
        UserAgentWhisperCommanderInterface<T>& interface;
        vector<string> recipients;
    };
    
}