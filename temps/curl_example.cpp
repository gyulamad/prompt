// g++ -std=c++17 -O2 -o curl_example curl_example.cpp -lcurl

#include "../src/tools/files.hpp"
#include "../src/tools/Curl.hpp"

using namespace tools;

int main() {
    try {
        Curl curl;
        
        // Configure client
        curl.AddHeader("Accept: application/json");
        curl.SetTimeout(5000); // 5 seconds
        curl.SetVerifySSL(true);
        curl.SetFollowRedirects(true);
        curl.SetProxy("http://proxy.example.com:8080");
        curl.SetAutoDecompress(true);
        curl.SetDNSCaching(300); // 5 minutes DNS cache
        
        // Set progress callback
        curl.SetProgressCallback([](double dlTotal, double dlNow,
                                   double ulTotal, double ulNow) {
            cout << "Progress: ↓" << dlNow << "/" << dlTotal
                      << " ↑" << ulNow << "/" << ulTotal << "\n";
            return true;
        });

        // GET with custom headers
        curl.GET("https://api.example.com/data",
            [](auto& chunk) { /*...*/ },
            {"X-Request-ID: 123"});

        // POST JSON payload
        curl.POST("https://api.example.com/create",
            [](auto& response) { /*...*/ },
            R"({"name": "John"})",
            {"Content-Type: application/json"});

        // PUT file upload
        string file_content = file_get_contents("temps/largefile.bin");
        curl.PUT("https://storage.example.com/file.txt",
            [](auto& response) { /*...*/ },
            file_content,
            {"Content-MD5: xyz"});

        // DELETE resource
        curl.DELETE("https://api.example.com/item/42",
            [](auto& confirmation) { /*...*/ });

        // Streaming GET
        curl.Request(Curl::Method::GET,
                    "https://api.example.com/stream",
                    [](const string& chunk) {
                        cout << "Chunk: " << chunk << "\n";
                    },
                    {"X-Custom-Header: value"}); // Per-request headers

        // JSON POST
        curl.Request(Curl::Method::POST,
                    "https://api.example.com/data",
                    [](const string& response) {
                        cout << "Response: " << response << "\n";
                    },
                    {"Content-Type: application/json"},
                    R"({"key": "value"})");

        // File Upload with PUT
        string file_data = file_get_contents("temps/largefile.bin");
        curl.Request(Curl::Method::PUT,
                    "https://storage.example.com/upload",
                    [](const string& response) {
                        cout << "Upload complete: " << response << "\n";
                    },
                    {},
                    file_data);

    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    return 0;
}