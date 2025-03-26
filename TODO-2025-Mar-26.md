| Difficulty | File | TODO |
|------------|------|------|
| 2 | src/tools/agency/agents/commands/VoiceCommand.hpp:55 |  // TODO ... - Add error handling or logging when setting the voice output state. |
| 2 | src/tools/agency/agents/UserAgentInterface.hpp:96 |  // TODO: I am not sure this one is needed anymore: - Determine if the code block is still needed and remove it if it's not. |
| 2 | src/tools/str/to_string.hpp:10 |  string to_string(bool b const string& t = "true" const string& f = "false") { // TODO to common libs - Move the to_string(bool) function to a common utility library. |
| 2 | src/tools/utils/Rotary.hpp:127 |  longest_output_length = 0; // TODO: longest output may not needed anymore... - Determine if the longest_output_length variable is still needed and remove it if it's not. |
| 3 | src/tools/agency/Agency.hpp:26 |  // TODO: these are deprecated: (commands and agents has/can have access to the agency so they can do it by themself) |
| 3 | src/tools/agency/Agency.hpp:200 | // TODO: - Improve getAgentRef method to use unordered_map for faster lookups. |
| 3 | src/tools/agency/agents/commands/SpawnCommand.hpp:45 |  Agent<T>& agent = roles[role](agency, name/*TODO: , recipients*/); - Pass recipients to the agent constructor when spawning a new agent. |
| 3 | src/tools/agency/agents/UserAgent.hpp:29 |  // TODO: make configurable: - Make the text_input_echo variable configurable. |
| 3 | src/tools/agency/agents/UserAgent.hpp:37 |  vector<string> recipients = { "echo" } // TODO: to config and have to be able to change/see message package targets - Make the recipients vector configurable and allow changing/seeing message package targets. |
| 3 | src/tools/agency/agents/UserAgent.hpp:59 |  else if (str_starts_with(input "/")) { // TODO: add is_command(input) as a command matcher using regex or a callback function. |
| 3 | src/tools/agency/agents/UserAgentInterface.hpp:19 |  class UserAgentInterface { // TODO: public UserInterface (abstract) - Make UserAgentInterface inherit from an abstract UserInterface class. |
| 3 | src/tools/agency/agents/UserAgentInterface.hpp:21 |  // TODO: make configurable: - Make the voice_input_echo variable configurable. |
| 3 | src/tools/agency/agents/UserAgentInterface.hpp:24 |  // TODO: make configurable: - Make the speech_ignores_rgxs vector configurable. |
| 3 | src/tools/agency/agents/UserAgentInterface.hpp:141 |  if (stt_voice_input && sequence.size() == 1 &&  sequence[0] == 27) { // TODO: ESC key - to config - Make the ESC key configurable. |
| 3 | src/tools/build/compile.cpp:43 |  string depcachepath = outpath_hash + "/.depcache"; // TODO: to config - Make the dependency cache path configurable. |
| 3 | src/tools/cmd/CommandFactory.hpp:39 | // TODO: add tests - Add unit tests for the CommandFactory class. |
| 3 | src/tools/utils/foreach.hpp:16 |  // TODO: foreach needs tests!! - Add unit tests for the foreach utility. |
| 3 | src/tools/utils/InputPipeInterceptor.hpp:34 |  input_handler(*this pipe_fds[1]); // TODO: it hooks the input (1) but we may want to parameterize it for output also? - Parameterize the InputPipeInterceptor to allow hooking of both input and output streams. |
| 3 | src/tools/utils/InputPipeInterceptor.hpp:92 |  int timeout = 100; // TODO make configurable: 100ms timeout (adjust as needed) - Make the timeout value configurable. |
| 4 | src/tools/agency/Agency.hpp:95 |  vector<Agent<T>*> agents; // TODO: Replace vector<Agent<T>*> with unordered_map<string, Agent<T>*>, O(1) lookup vs. O(n), huge win with many agents. |
| 5 | src/tools/utils/Config.hpp:11 |  // TODO: !!! DEPRECATED !!! - use Settings instead! - Replace all usages of the Config class with the Settings class. |
