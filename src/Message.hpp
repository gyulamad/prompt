#pragma once

#include <string>
#include <unordered_map>

#include "tools/strings.hpp"
#include "tools/JSON.hpp"

using namespace std;
using namespace tools;

namespace prompt {

    #define ANSI_FMT_MODEL_THINKS ANSI_FMT_C_BLACK // ANSI_FMT_T_DIM

    typedef enum { ROLE_NONE = 0, ROLE_INPUT, ROLE_OUTPUT } role_t;
    typedef unordered_map<role_t, const string> role_name_map_t;
    const role_name_map_t default_role_name_map = {
        { ROLE_NONE, "" },
        { ROLE_INPUT, "input" },
        { ROLE_OUTPUT, "output" },
    };

    string to_string(const role_t& role, const role_name_map_t& role_name_map = default_role_name_map) {
        return role_name_map.at(role);
    }
    
    class Message {
    private:
        string text;
        role_t role;
    public:

        Message(
            const string& text, 
            const role_t& role
        ):
            text(text), 
            role(role)
        {}

        virtual ~Message() {}

        JSON toJSON() const {
            JSON json;
            json.set("text", text);
            json.set("role", role);
            return json;
        }

        void fromJSON(JSON json) {
            text = json.get<string>("text");
            role = json.get<role_t>("role");
        }

        string get_text() const {
            return text;
        }

        void set_text(const string& text) {
            this->text = text;
        }

        role_t get_role() const {
            return role;
        }

        void set_role(const role_t& role) {
            this->role = role;
        }

        size_t length() const {
            return dump().size();
        }

        string dump(bool show = false) const {
            string dump = ((role == ROLE_NONE) ? "" : escape(to_string(role)) + ": ") + escape(text);
            return dump;
        }
    };
}