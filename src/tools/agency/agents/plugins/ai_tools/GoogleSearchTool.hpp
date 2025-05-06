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
    class GoogleSearchTool: public Tool<T> {
    public:
        
        GoogleSearchTool(UserAgent<T>& user): Tool<T>(
            user,
            "google_search", 
            { 
                { "query", PARAMETER_TYPE_STRING, true }, 
                { "max", PARAMETER_TYPE_INTEGER, false }, 
            }, 
            callback,
            "Performs a google search and shows the result list."
        ) {}

        static string callback(void* /*tool_void*/, /*void* model_void, void* user_void,*/ const JSON& args/*, const JSON& conf*/) {
            string query = args.get<string>("query");
            int max = args.get<int>("max");
            // TODO: use user interface here:
            cout << "Google search: '" + escape(query) + "'" << endl;
            string results = Process::execute(
                "node browse/google-search.js"
                " --query \"" + escape(query) + "\""
                " --max " + ::to_string(max));
            cout << results << endl;
            return results;
        }

    };

}