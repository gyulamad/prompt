#pragma once

#include <string>

#include "../utils/Printer.hpp"
#include "ChatHistory.hpp"

using namespace std;

namespace tools::chat {

    class Chatbot {
    public:
        Chatbot(
            const string& name,
            Factory<ChatHistory>& histories, const string& history_type, //ChatHistory& history, 
            Printer& printer
        ): 
            name(name),
            histories(histories),
            history(histories.hold(this, histories.create(history_type)) /*history*/),
            printer(printer)
        {}
        virtual ~Chatbot() {
            cout << "Chatbot (" + this->name + ") destruction..." << endl; 
            histories.release(this, history);
        }
        virtual string chat(const string& sender, const string& text) = 0;
        virtual string chunk(const string& chunk) { 
            printer.print(chunk);
            return chunk;
        }
        virtual string response(const string& response) { return response; }

        ChatHistory& getHistoryRef() { return *safe(history); }

        const string name;
    protected:
        Factory<ChatHistory>& histories;
        ChatHistory* history = nullptr;
        Printer& printer;
    };

}
