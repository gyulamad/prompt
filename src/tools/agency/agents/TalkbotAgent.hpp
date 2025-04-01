#pragma once

#include <string>

#include "../../chat/Talkbot.hpp"

using namespace tools::chat;

using namespace std;

namespace tools::agency::agents {

    template<typename T>
    class TalkbotAgentConfig: public AgentConfig<T> { // TODO extends Agent::Config ???
    public:
        TalkbotAgentConfig(
            Owns& owns,
            void* agency,
            PackQueue<T>& queue,
            const string& name,
            vector<string> recipients,
            void* talkbot
        ):
            talkbot(talkbot),
            AgentConfig<T>(owns, agency, queue, name, recipients)
        {}

        virtual ~TalkbotAgentConfig() {}

        void* getTalkbotPtr() { return talkbot; }

    private:
        void* talkbot = nullptr;
    };

    template<typename T>
    class TalkbotAgent: public Agent<T> {
    public:

        TalkbotAgent(TalkbotAgentConfig<T>& config
            // Owns& owns,
            // PackQueue<T>& queue,
            // const string& name,
            // vector<string> recipients,
            // Talkbot* talkbot
        ): 
            owns(config.getOwnsRef()),
            Agent<T>(config),
            talkbot(owns.reserve(this, config.getTalkbotPtr(), FILELN))
        {
            // owns.reserve(this, talkbot, FILELN);
        }

        virtual ~TalkbotAgent() {
            owns.release(this, talkbot);
        }

        string type() const override { return "talk"; }

        void handle(const string& sender, const T& item) override {
            this->addRecipients({ sender });
            this->send(((Talkbot*)talkbot)->chat(sender, item));
        }

    private:
        Owns& owns;
        void* talkbot = nullptr;
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
    string chat(const string& sender, const string& text) override { return ""; }
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
    TalkbotAgentConfig<string> config(owns, nullptr, queue, "talk", {"user"}, talkbot);
    TalkbotAgent<string> agent(config);
}

TEST(test_TalkbotAgent_reserve);

#endif