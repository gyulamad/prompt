#include <iostream>
#include <string>
// #include <fstream>

#include "Proc.hpp"
#include "tools.hpp"
#include "JSON.hpp"

using namespace std;

class Speech {
private:
    const string speechf;
    const string elogf;

public:
    Speech(
        const string& speechf = "/tmp/temp-speech.wav", 
        const string& elogf = "/dev/null"
    ): 
        speechf(speechf), 
        elogf(elogf)
    {}
    
    ~Speech() {}

    string rec(int timeout = 20) {
        const string secret = trim(file_get_contents("hugging.key"));

        Proc proc;
        string output;

        while (true) {
// cout << "Delete: " << speechf << endl;
            proc.writeln("rm -f " + speechf + " && echo done");
            while ((output = proc.read()).empty()); // waiting for "done"
// cout << "outp: " << output << endl;

            // recording the speech chunk
// cout << "Recording... (timeout " + to_string(timeout) + ")" << endl;
            proc.writeln(
                "timeout " + to_string(timeout) + " "
                "arecord -f cd -t wav -r 16000 -c 1 2>" + elogf + " | " // TODO error to a log that is "/dev/null" by default + add every parameter cusomizable
                "sox -t wav - -t wav " + speechf + " silence 1 0.1 2% 1 1.0 2% 2>/dev/null && "
                "echo \"done\"");
            
            //cout << proc.read() << endl;
            while ((output = proc.read()).empty()); // waiting for "done"
// cout << "outp: " << output << endl;


            // skipp transcriptions if silence
// cout << "Silent?" << endl;
            proc.writeln("sox " + speechf + " -n stat 2>&1 | grep \"Maximum amplitude\" | awk '{print $3}'");
            while ((output = proc.read()).empty()); // waiting for sox to tell if silent?
// cout << "??" << output << endl;
            if (str_starts_with(output, "0.00")) continue;


            // transcribe
// cout << "transcribe..." << endl;
            proc.writeln(
                "curl https://api-inference.huggingface.co/models/jonatasgrosman/wav2vec2-large-xlsr-53-hungarian \
                    -X POST \
                    --data-binary '@" + speechf + "' \
                    -H 'Authorization: Bearer " + secret + "' -s");
            while ((output = proc.read()).empty()); // waiting for transcriptions
// cout << "resp: " << output << endl;

            // extract text

            JSON json(output);
            if (!json.isDefined("text")) {
                cerr << "STT transcibe failed:" + json.dump() << endl;
                break;  // TODO throw ERROR("STT transcibe failed:" + json.dump());
            }
            output = json.get<string>("text");

            if (!trim(output).empty()) break; // restart/break recording if it was silent
        }

        return output;
    }


};

// void start_recording(const string& output_file) {
//     string temp_file = "/tmp/speach-rec.wav";
    
//     // Construct the Bash command
//     string command = "timeout 10 arecord -f cd -t wav -r 16000 -c 1 2>/dev/null | " // TODO error to a log that is "/dev/null" by default + add every parameter cusomizable
//                      "sox -t wav - -t wav " + output_file + " silence 1 0.1 2% 1 1.0 2% 2>/dev/null";
//     // Create a Proc instance to run the command
//     Proc proc;
//     proc.writeln(command);
//     while(proc.ready());
// }
// bool start_recording(const string& output_file) {
//     // Construct the Bash command
//     string command = "timeout 10 arecord -f cd -t wav -r 16000 -c 1 2>/dev/null | "
//                      "sox -t wav - -t wav " + output_file + " silence 1 0.1 2% 1 1.0 2% 2>/dev/null";
//     // string command = "timeout 10 arecord -f cd -t wav -r 16000 -c 1 2>&1 | "
//     //                  "sox -t wav - -t wav " + output_file + " silence 1 0.1 2% 1 1.0 2% 2>&1";
    
//     // Create a Proc instance to run the command
//     Proc proc;
//     proc.writeln(command);

//     // Wait for the command to complete and capture output
//     string output = proc.read();
//     cout << "aasdasdas:" <<  output << endl;

//     // Check if the output file was created
//     ifstream file(output_file);
//     if (file.good()) {
//         cout << "Recording saved to " << output_file << endl;
//         return true;
//     } else {
//         cerr << "Error: Failed to create " << output_file << endl;
//         if (!output.empty()) {
//             cerr << "Command output: " << output << endl;
//         }
//         return false;
//     }
// }

int main() {
    cout << "starte..." << endl;
    Speech speech;
    string said;
    // while ((said = stt.rec()).empty());
    while (true) {
        said = speech.rec();
        cout << "USER SAY:" << said << endl;
    }
    // string output_file = "speach.wav";
    // start_recording(output_file);
    cout << "ende..." << endl;
    return 0;
}