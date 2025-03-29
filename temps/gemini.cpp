#include "../src/tools/utils/Test.hpp"
// #include "../src/tools/abstracts/Stream.hpp"
// #include "../src/tools/str/escape.hpp"
// #include "../src/tools/str/trim.hpp"
// #include "../src/tools/str/str_starts_with.hpp"
// #include "../src/tools/str/json_escape.hpp"
// #include "../src/tools/str/str_replace.hpp"
// #include "../src/tools/str/tpl_replace.hpp"
// #include "../src/tools/utils/foreach.hpp"
// #include "../src/tools/utils/Curl.hpp"
// #include "../src/tools/utils/JSON.hpp"
#include "../src/tools/chat/ChatHistory.hpp"
#include "../src/tools/voice/BasicSentenceSeparation.hpp"
#include "../src/tools/voice/SentenceStream.hpp"
#include "../src/tools/voice/ESpeakTTSAdapter.hpp"
#include "../src/tools/utils/Printer.hpp"
#include "../src/tools/utils/Process.hpp"
#include "../src/tools/ai/Gemini.hpp"

// using namespace tools::abstracts;
// using namespace tools::str;
using namespace tools::chat;
using namespace tools::utils;
using namespace tools::voice;
using namespace tools::ai;

// TODO: Implement a full GeminiAPI interface
// TODO: Add OpenAIAPI base implementation: https://ai.google.dev/gemini-api/docs/openai














int main() {
    run_tests();

    
    string secret = "AIzaSyDabZvXQNSyDYAcivoaKhSWhRmu9Q6hMh4";
    // string variant = "gemini-1.5-pro-latest";
    string variant = "gemini-1.5-flash-8b";
    long timeout = 30000;

    string prompt = "> ";

    long sentences_max_buffer_size = 1024*1024;

    vector<string> separators = {".", "!", "?"};

    ChatHistory history(prompt, false);

    Printer printer;
    BasicSentenceSeparation separator(separators);
    SentenceStream sentences(separator, sentences_max_buffer_size);

    string lang = "en";
    
    Process process;
    ESpeakTTSAdapter tts(
        lang,
        200, //int speed, 
        0, //int gap,
        "sox -v 0.03 beep.wav -t wav - | aplay -q -N", //const string& beep_cmd,
        "find sounds/r2d2/ -name \"*.wav\" | shuf -n 1 | xargs -I {} bash -c 'sox -v 0.01 \"{}\" -t wav - | aplay -q -N'", //const string& think_cmd,
        {
            { "...", "\n\n\n\n" },
            { "***", "\n\n\n\n" },
            { "**", "\n\n\n" },
            { "*", "\n\n" },
            { "'", "" },
        }, //const map<string, string>& speak_replacements,
        &process //Process* process = nullptr
    );

    Gemini gemini("gemini", history, printer, sentences, tts, secret, variant, timeout);

    string input;
    while (true) {
        cout << prompt << flush;
        getline(cin, input);
        if (input == "exit") break;
        gemini.chat("user", input);
    }


    return 0;
}