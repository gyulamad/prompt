#pragma once

#include "tools/llm/Model.hpp"
#include "tools/Process.hpp"

using namespace std;
using namespace tools;
using namespace tools::llm;

namespace tools {

    class NothingTool: public Tool {
    public:
        
        NothingTool(): Tool(
            "do_nothing", 
            { }, 
            callback,
            "Performs nothing. Litterally nothing. It's just a placeholder stuff..."
        ) {}

        static string callback(void*, const JSON&) { return ""; }

    } nothingTool;


    class GoogleSearchTool: public Tool {
    public:
        
        GoogleSearchTool(): Tool(
            "google_search", 
            { 
                { "query", PARAMETER_TYPE_STRING, true }, 
                { "max", PARAMETER_TYPE_INTEGER, true }, 
            }, 
            callback,
            "Performs a google search and shows the result list."
        ) {}

        static string callback(void* tool_void, const JSON& args) {
            string query = args.get<string>("query");
            int max = args.get<int>("max");
            cout << "Google search: '" + query + "' ..." << endl;
            return Process::execute(
                "node browse/google-search.js"
                " --query \"" + escape(query) + "\""
                " --max " + to_string(max));
        }

    } googleSearchTool;
    

    class WebBrowserTool: public Tool {
    public:
        
        WebBrowserTool(): Tool(
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

        static string callback(void* tool_void, const JSON& args) {
            string url = args.get<string>("url");
            string method = args.has("method") ? args.get<string>("method") : "";
            string data = args.has("data") ? args.get<string>("data") : "";
            string cookies = args.has("cookies") ? args.get<string>("cookies") : "";
            string script = args.has("script") ? args.get<string>("script") : "";
            
            cout << "Loading: " << (method.empty() ? "" : "[" + method + "] ") << url << " ..." << endl;
            return Process::execute(
                "node browse/web_scraper.js"
                " --url \"" + escape(url) + "\""
                + (method.empty() ? "" : " --method " + escape(method))
                + (data.empty() ? "" : " --data \"" + escape(data) + "\"")
                + (cookies.empty() ? "" : " --cookies \"" + escape(cookies) + "\"")
                + (script.empty() ? "" : " --script \"" + escape(script) + "\"")
            );
        }

    } webBrowserTool;

}