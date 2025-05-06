#pragma once

#include "../../../../utils/JSON.h"
#include "../../../../datetime/ms_to_datetime.hpp"
#include "../../UserAgentInterface.hpp"
#include "../Tool.hpp"

using namespace tools::datetime;
using namespace tools::utils;
using namespace tools::agency::agents;

namespace tools::agency::agents::plugins::ai_tools {

    template<typename T>
    class DateTimeTool: public Tool<T> {
    public:
        
        DateTimeTool(
            UserAgent<T>& user,
            const string& date_format,
            bool millis,
            bool local
        ): 
            Tool<T>(
                user,
                "datetime", 
                { }, 
                callback,
                "This tools shows the systems current date and time "
                "as the AI does not have access to the real time data but sometimes it's needed."
            ),
            date_format(date_format),
            millis(millis),
            local(local)
        {}

        static string callback(void* tool_void, /*void* model_void, void* user_void,*/ const JSON& /*args*//*, const JSON& conf*/) {
            DateTimeTool* tool = (DateTimeTool*)safe(tool_void);
            return ms_to_datetime(get_time_ms(), tool->date_format.c_str(), tool->millis, tool->local); 
        }

        string date_format;
        bool millis;
        bool local;

    };

}
