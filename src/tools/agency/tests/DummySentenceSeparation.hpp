#pragma once

class DummySentenceSeparation: public SentenceSeparation {
public:
    using SentenceSeparation::SentenceSeparation;
    size_t findSentenceEnd(const string& /*text*/, size_t /*start_pos*/) const override { return 0; }
};