#pragma once

#include "../../../../utils/JSON.h"
#include "../../../../utils/Process.hpp"
#include "../../../../str/escape.hpp"
#include "../../UserAgentInterface.hpp"
#include "../Tool.hpp"

using namespace tools::utils;
using namespace tools::str;
using namespace tools::agency::agents;
using namespace tools::agency::agents::plugins;

namespace tools::agency::agents::plugins::ai_tools {

    template<typename T>
    class WebBrowserTool: public Tool<T> {
    public:
        
        WebBrowserTool(UserAgent<T>& user): Tool<T>(
            user,
            "web_browse", 
            { 
                { "url", PARAMETER_TYPE_STRING, true }, 
                { "method", PARAMETER_TYPE_STRING },
                { "data", PARAMETER_TYPE_STRING },
                { "cookies", PARAMETER_TYPE_STRING }, 
                { "script", PARAMETER_TYPE_STRING }, 
            }, 
            callback,
            "Load the source-code of a web page from the given URL. "
            "Use methods GET/POST/PUT etc. with data and cookies. "
            "With the script parameter you can inject javascript."
        ) {}

        static string callback(void* /*tool_void, void* model_void, void* user_void*/, const JSON& args/*, const JSON& conf*/) {
            string url = args.get<string>("url");
            string method = args.has("method") ? args.get<string>("method") : "";
            string data = args.has("data") ? args.get<string>("data") : "";
            string cookies = args.has("cookies") ? args.get<string>("cookies") : "";
            string script = args.has("script") ? args.get<string>("script") : "";
            
            // TODO: use user interface here
            cout << "Loading: " << (method.empty() ? "" : "[" + method + "] ") << url << endl;
            return Process::execute(
                "node browse/web_scraper.js"
                " --url \"" + escape(url) + "\""
                + (method.empty() ? "" : " --method " + escape(method))
                + (data.empty() ? "" : " --data \"" + escape(data) + "\"")
                + (cookies.empty() ? "" : " --cookies \"" + escape(cookies) + "\"")
                + (script.empty() ? "" : " --script \"" + escape(script) + "\"") // TODO: escaping mess up!
            );
        }

    };

}