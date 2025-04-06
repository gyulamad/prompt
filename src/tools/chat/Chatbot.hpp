#pragma once

#include <string>

#include "../utils/Printer.hpp"
#include "ChatHistory.hpp"

using namespace std;

namespace tools::chat {

    class Chatbot { // : public JSONSerializable { //  TODO: refact: build up a correct abstraction hierarhy
    public:

        class cancel: public exception {};

        Chatbot(
            Owns& owns,
            const string& name,
            void* history, 
            Printer& printer
        ):
            // JSONSerializable(),
            owns(owns),
            name(name),
            history(owns.reserve<ChatHistory>(this, history, FILELN)),
            printer(printer)
        {}

        virtual ~Chatbot() {
            // cout << "Chatbot (" + this->name + ") destruction..." << endl; 
            // histories.release(this, history);
            owns.release(this, history);
        }

        string getName() const { return name; }

        void* getHistoryPtr() { return history; } // TODO: remove this

        // ----- stream chat -----

        virtual string chat(const string& sender, const string& text, bool& interrupted) = 0;

        virtual string chunk(const string& chunk) {
            STUB_VIRTUAL // TODO...
            printer.print(chunk);
            return chunk;
        }

        virtual string response(const string& response) {
            STUB_VIRTUAL // TODO...
            return response; 
        }

        // ----- completion -----

        virtual string respond(const string& sender, const string& text) = 0;


        // ----- JSON serialization -----

        // void fromJSON(const JSON& json) override {
        //     DEBUG("Chatbot::fromJSON called"); 
        //     // Convert pointer to integer type for printing
        //     uintptr_t history_addr = reinterpret_cast<uintptr_t>(history);
        //     DEBUG("Checking history pointer address: " + to_string(history_addr)); 
        //     safe(history); // Check history pointer before use
        //     DEBUG("History pointer is safe"); 
        //     history->fromJSON(json.get<JSON>("history")); // Call history's fromJSON
        //     DEBUG("Called history->fromJSON"); 

        //     name = json.get<string>("name");
        // }

        // JSON toJSON() const override {
        //     JSON json;
        //     json.set("history", safe(history)->toJSON());
        //     json.set("name", name);
        //     return json;
        // }
        

    protected:
        Owns& owns;
        string name; //  TODO: remove this!
        ChatHistory* history = nullptr;
        Printer& printer;
    };

    

}
