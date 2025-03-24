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
            Agency<T>& agency, 
            UserAgentWhisperCommanderInterface<T>& interface
        ): 
            Agent<T>(queue, "user"), 
            agency(agency),
            interface(interface) 
        {}

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
            else if (str_starts_with(input, "/")) {
                interface.getCommanderRef().run_command(&agency, input); // TODO: add is_command(input) as a command matcher (regex or callback fn) instead just test for "/" 
            }
            else if (text_input_echo) interface.println(input, true, false);
        }

    private:
        Agency<T>& agency;
        UserAgentWhisperCommanderInterface<T>& interface;
    };
    
}