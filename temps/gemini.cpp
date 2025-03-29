#include "../src/tools/utils/Test.hpp"
#include "../src/tools/abstracts/Stream.hpp"
#include "../src/tools/str/escape.hpp"
#include "../src/tools/str/trim.hpp"
#include "../src/tools/str/str_starts_with.hpp"
#include "../src/tools/str/json_escape.hpp"
#include "../src/tools/str/str_replace.hpp"
#include "../src/tools/utils/Curl.hpp"
#include "../src/tools/utils/JSON.hpp"
#include "../src/tools/voice/SentenceStream.hpp"

using namespace tools::abstracts;
using namespace tools::utils;
using namespace tools::str;
using namespace tools::voice;

class Gemini {
public:
    void request(const string& input) {
        string secret = "AIzaSyDabZvXQNSyDYAcivoaKhSWhRmu9Q6hMh4";
        // string variant = "gemini-1.5-pro-latest";
        string variant = "gemini-1.5-flash-8b";
        long timeout = 30000;

        Curl curl;
        curl.AddHeader("Content-Type: application/json");
        curl.AddHeader("Accept: application/json");
        curl.SetTimeout(timeout);
        curl.SetVerifySSL(true);
        
        string url = "https://generativelanguage.googleapis.com/v1beta/models/" 
            + variant + ":streamGenerateContent?alt=sse&key=" + escape(secret);            

        context += input + "\n";
        string data = str_replace("{{context}}", json_escape(context), R"({
            "contents": [{
                "parts": [{
                    "text": "{{context}}"
                }]
            }]
        })");
        
        string response;
        curl.POST(url, [&](const string& chunk) {
            // Process SSE events
            vector<string> splits = explode("\r\n\r\n", chunk);
            for (const string& split : splits) {
                string trm = trim(split);
                if (trm.empty()) continue;
                if (!str_starts_with(trm, "data:")) throw ERROR("Invalid SSE response: " + trm);
                vector<string> parts = explode("data: ", trm);
                if (parts.size() < 2) throw ERROR("Invalid SSE data: " + trm);
                JSON json(parts[1]);
                if (json.isDefined("error")) throw ERROR("Gemini error: " + json.dump());
                if (!json.isDefined("candidates[0].content.parts[0].text")) throw ERROR("Gemini error: text is not defined: " + json.dump());
                string text = json.get<string>("candidates[0].content.parts[0].text");

                context += text;
                cout << text << flush;
            }
        }, data);
    }
private:
    string context;
};




int main() {
    run_tests();

    BasicSentenceSeparation separator({".", "!", "?"});
    SentenceStream ssentence(separator, 1024 * 1024);

    string essey = R"(The Unexpected Power of the Personal Essay

The seemingly simple act of writing a personal essay can reveal profound truths about ourselves and the world around us.  It's a unique form of self-discovery and connection with others, even though it often feels straightforward.  This essay will explore why the personal essay, despite its seemingly straightforward nature, packs a powerful punch.

One of the core elements of a compelling personal essay is vulnerability.  Sharing personal experiences and feelings can be incredibly challenging.  It takes courage to lay bare your struggles, joys, and even your imperfections in front of a reader.  But when done well, this vulnerability is what connects with the reader.  It humanizes the writer and allows the reader to understand and relate on a deeper level.  Think about the impact of essays where the author doesn't shy away from their flaws or insecurities; they're often the essays that stick with us long after we finish reading.

The process of writing a personal essay can be a journey of self-discovery.  When you confront your experiences on paper, you often uncover truths about yourself you weren't aware of before.  It's in the act of writing, of expressing your thoughts and feelings, that you can achieve a new level of self-awareness.  The act of putting your experiences into words gives you a different perspective.  Through this process, you may even see patterns and connections you never noticed before.  It's not about perfection; it's about honesty.

Beyond self-discovery, personal essays can foster empathy and understanding in the reader.  Sharing personal experiences can create a sense of connection and community.  We often feel less alone when we realize that others have faced similar struggles or celebrated similar joys.  When we can see ourselves reflected in the experiences of others, the world feels smaller and yet bigger at the same time.  Well-written personal essays can offer this sense of connection, allowing readers to see humanity through a fresh lens.  There are many personal essays that demonstrate this ability to build bridges between people.

The personal essay, as a genre, has evolved throughout history.  From ancient reflections to contemporary narratives, the form has always served as a tool for social change.  Sometimes, personal essays can serve as powerful commentaries on social issues or movements.  Sharing personal experiences that challenge norms can open up dialogue and inspire action in readers.


Ultimately, the personal essay is more than just a collection of words; it's a powerful tool for personal and communal growth.  By embracing vulnerability, we can discover truths about ourselves and connect with others in profound ways.  So, the next time you pick up an essay, consider the potential for personal growth that lies within its pages.  Why not try writing one yourself? You might be surprised at what you discover.
)";

    // TODO: write a split


    exit(0);

    Gemini gemini;

    string input;
    while (input != "exit") {
        cout << "> " << flush;
        getline(cin, input);
        gemini.request(input);
    }


    return 0;
}