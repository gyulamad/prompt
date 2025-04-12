#pragma once

#include "../chat/DecidorChatbot.hpp"

namespace tools::agency::agents {

    template<typename T>
    class DecidorChatbotAgent: public Agent<T> {
    public:
        DecidorChatbotAgent(
            Owns& owns,
            Worker<T>* agency,
            PackQueue<T>& queue,
            const string& name,
            void* decidorChatbot
        ): 
            Agent<T>(owns, agency, queue, name),
            decidorChatbot(owns.reserve<DecidorChatbot>(this, decidorChatbot, FILELN))
        {}

        virtual ~DecidorChatbotAgent() {
            this->owns.release(this, decidorChatbot);
        }

        string type() const override { return "decidor"; }

        void handle(const string& /*sender*/, const T& /*item*/) {
            STUB("Needs to be implemented"); // TODO
            throw ERROR("Unimplemented");
        }
    
    private:
        DecidorChatbot* decidorChatbot = nullptr;
    };

}
