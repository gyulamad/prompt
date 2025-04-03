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
            const string& name,
            vector<string> recipients,
            void* talkbot,
            UserAgentInterface<T>& interface
        ): 
            Agent<T>(owns, agency, queue, name, recipients),
            talkbot(owns.reserve<Talkbot>(this, talkbot, FILELN)),
            interface(interface)
        {}

        virtual ~TalkbotAgent() {
            this->owns.release(this, talkbot);
        }

        void* getTalkbotPtr() { return talkbot; }


        string type() const override { return "talk"; }

        void handle(const string& sender, const T& item) override {
            
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

    private:
        Talkbot* talkbot = nullptr;
        UserAgentInterface<T>& interface;
    };
    
}

#ifdef TEST

#include "../../voice/SentenceSeparation.hpp"
#include "../PackQueue.hpp"

using namespace tools::voice;
using namespace tools::agency;
using namespace tools::agency::agents;

class DummyTalkbot: public Talkbot {
public:
    using Talkbot::Talkbot;
    string chat(const string& sender, const string& text, bool& interrupted) override { return ""; }
};

class DummySentenceSeparation: public SentenceSeparation {
public:
    using SentenceSeparation::SentenceSeparation;
    size_t findSentenceEnd(const string& text, size_t start_pos) const override { return 0; }
};

void test_TalkbotAgent_reserve() {
    Owns owns;
    string name = "dummy";
    string prompt = "";
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
    } editor;
    CommandLine commandLine(
        editor
    );
    vector<Command*> commands;
    STTSwitch sttSwitch;
    MicView micView;
    Commander commander(
        commandLine, 
        commands
    );
    InputPipeInterceptor interceptor;
    UserAgentInterface<string> interface(
        tts,
        sttSwitch,
        micView,
        commander,
        interceptor
    );
    TalkbotAgent<string> agent(owns, nullptr, queue, "talk", {"user"}, talkbot, interface);
}

TEST(test_TalkbotAgent_reserve);

#endif