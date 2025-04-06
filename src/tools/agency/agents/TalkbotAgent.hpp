#pragma once

#include <string>

#include "../Agent.hpp"
#include "../../chat/Talkbot.hpp"
// #include "../../abstracts/UserInterface.hpp"
#include "UserAgentInterface.hpp"

using namespace std;
using namespace tools::chat;
using namespace tools::abstracts;

namespace tools::agency::agents {
    
    template<typename T>
    class TalkbotAgent: public Agent<T> {
    public:

        TalkbotAgent(
            Owns& owns,
            Worker<T>* agency,
            PackQueue<T>& queue,
            const JSON& json,
            void* talkbot,
            UserAgentInterface<T>& interface
        ): 
            Agent<T>(owns, agency, queue, json),
            talkbot(owns.reserve<Talkbot>(this, talkbot, FILELN)),
            interface(interface)
        {}

        virtual ~TalkbotAgent() {
            this->owns.release(this, talkbot);
        }

        void* getTalkbotPtr() { return talkbot; }


        string type() const override { return "talk"; }

        void handle(const string& sender, const T& item) override {
            DEBUG("TalkbotAgent::handle called");
            DEBUG("Command: " + item);
            
            // // TODO: hide/disable input interface prompt (voice input only to interrupt - or keypress also??)
            // interface.clearln();
            // // interface.hide_prompt();
            // interface.println("[[[---START--->>>");

            interface.getCommanderRef().getCommandLineRef().setPromptVisible(false);
            
            bool interrupted;
            string response = safe(talkbot)->chat(sender, item, interrupted);

            if (interrupted) {
                DEBUG("[[[---TALK INTERRUPTED BY USER---]]] (DBG)"); // TODO interrupt next, now
            }
            
            // this->removeRecipients({ sender });
            // this->send(response);

            // TODO: show/enable input
            // DEBUG("[[[---STOP---]]]"); // TODO interrupt next, now
            // interface.show_prompt();

            interface.getCommanderRef().getCommandLineRef().setPromptVisible(true);
        }

        // void fromJSON(const JSON& json) override {
        //     DEBUG("TalkbotAgent::fromJSON called");
        //     safe(talkbot);
        //     DEBUG("talkbot is safe");
        //     DEBUG("json is: " + json.dump());
        //     talkbot->fromJSON(json); // TODO !@# segfault here!!!
        //     DEBUG("talkbot->fromJSON(json) is called");
        // }

    private:
        Talkbot* talkbot = nullptr;
        UserAgentInterface<T>& interface;
    };
    
}

#ifdef TEST

#include "../../voice/SentenceSeparation.hpp"
#include "../PackQueue.hpp"
#include "../tests/DummyTalkbot.hpp"
#include "../tests/DummySentenceSeparation.hpp"

using namespace tools::voice;
using namespace tools::agency;
using namespace tools::agency::agents;

void test_TalkbotAgent_reserve() {
    Owns owns;
    string name = "dummy";
    string prompt = "";
    string cprefix = "/";
    bool use_start_token = false;
    ChatHistory* history = owns.allocate<ChatHistory>(prompt, use_start_token);
    Printer printer;
    vector<string> separators = {};
    DummySentenceSeparation separator(separators);
    SentenceStream sentences(separator, 1024);
    map<string, string> speak_replacements = {};
    Process proc;
    TTS tts("en", 300, 0, "", "", speak_replacements, &proc);
    DummyTalkbot* talkbot = owns.allocate<DummyTalkbot>(
        owns, name, history, printer, sentences, tts
    ); // Dummy Talkbot
    PackQueue<string> queue;

    class MockLineEditor: public LineEditor {
        void setCompletionCallback(CompletionCallback) override {}
        void setMultiLine(bool /*enable*/) override {}
        void setHistoryMaxLen(size_t /*max_len*/) override {}
        void loadHistory(const char* /*path*/) override {}
        void saveHistory(const char* /*path*/) override {}
        void addHistory(const char* /*line*/) override {}
        bool readLine(string& /*line*/) override { return false; }
        void refreshLine() override {}
        void wipeLine() override {}
        void setPrompt(const char* /*prompt*/) override {}
        void setPrompt(string& /*prompt*/) override {}
    } editor;
    CommandLine commandLine(
        editor
    );
    vector<Command*> commands;
    STTSwitch sttSwitch;
    MicView micView;
    Commander commander(
        commandLine, 
        commands,
        cprefix
    );
    InputPipeInterceptor interceptor;
    UserAgentInterface<string> interface(
        tts,
        sttSwitch,
        micView,
        commander,
        interceptor
    );
    JSON json;
    json.set("name", "name");
    json.set("recipients", vector<string>{});
    TalkbotAgent<string> agent(owns, nullptr, queue, json, talkbot, interface);
}

TEST(test_TalkbotAgent_reserve);

#endif
