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

#include "UserAgentInterface.hpp"

using namespace tools::str;
using namespace tools::regx;
using namespace tools::cmd;
using namespace tools::voice;
using namespace tools::utils;
using namespace tools::agency;

namespace tools::agency::agents {

    template<typename T>
    class UserAgentConfig: public AgentConfig<T> {
    public:
        UserAgentConfig(
            PackQueue<T>& queue,
            const string& name, 
            vector<string> recipients, // TODO: to config, and have to be able to change/see message package targets
            Agency<T>& agency, 
            UserAgentInterface<T>& interface
        ):
            AgentConfig<T>(queue, name, recipients),
            agency(agency),
            interface(interface)
        {}

        virtual ~UserAgentConfig() {}

        Agency<T>& getAgencyRef() { return agency; } 
        UserAgentInterface<T>& getInterfaceRef() { return interface; }

    private:
        Agency<T>& agency;
        UserAgentInterface<T>& interface;
    };

    template<typename T>
    class UserAgent: public Agent<T> {
    public:

        // TODO: make configurable:
        atomic<bool> text_input_echo = true;

        UserAgent(UserAgentConfig<T> config
            // PackQueue<T>& queue,
            // const string& name, 
            // vector<string> recipients, // TODO: to config, and have to be able to change/see message package targets
            // Agency<T>& agency, 
            // UserAgentInterface<T>& interface
        ): 
            Agent<T>(config), 
            agency(config.getAgencyRef()),
            interface(config.getInterfaceRef())
        {
            interface.setUser(this);
        }

        virtual ~UserAgent() {}

        string type() const override { return "user"; }

        UserAgentInterface<T>& getInterfaceRef() { return interface; }

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
                if (text_input_echo) {
                    interface.clearln();
                    interface.println(input);
                }
                onInput(input);
            }
        }

        void onInput(T input) {
            this->send(input);
        }

        void handle(const string& sender, const T& item) override {
            interface.println("Incoming message from '" + sender + "': " + item);
        }

    private:
        Agency<T>& agency;
        UserAgentInterface<T>& interface;
    };
    
}