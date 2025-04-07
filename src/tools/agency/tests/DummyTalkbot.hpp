#pragma once

class DummyTalkbot: public Talkbot {
public:
    using Talkbot::Talkbot;
    virtual ~DummyTalkbot() {}
    string chat(const string& /*sender*/, const string& /*text*/, bool& /*interrupted*/) override { return ""; }
    string respond(const string& /*sender*/, const string& /*text*/) override { return ""; }
};