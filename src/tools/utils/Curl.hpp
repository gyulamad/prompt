#pragma once

#include <iostream>
#include <string>
#include <cstring>
#include <functional>
#include <vector>
#include <mutex>
#include <atomic>
#include <memory>
#include <curl/curl.h>

using namespace std;

namespace tools::utils {
    
    class Curl {
    public:
        using StreamCallback = function<void(const string& chunk)>;
        using ProgressCallback = function<bool(double dltotal, double dlnow,
                                               double ultotal, double ulnow)>;

        enum class Method { GET, POST, PUT, DELETE, PATCH, HEAD, OPTIONS };

        Curl() {
            call_once(global_init_flag, []() {
                curl_global_init(CURL_GLOBAL_DEFAULT);
            });
        }

        ~Curl() = default;

        struct Context {
            atomic<bool> cancelled{false};
            StreamCallback cb;
            string buffer;
            string upload_data;
            size_t upload_offset = 0;
            char error[CURL_ERROR_SIZE]{};
            exception_ptr exception;
        };

        // === Core Request Method ===
        bool Request(
            Method method, const string& url,
            const StreamCallback& callback,
            const vector<string>& req_headers = {},
            const string& data = ""
        ) {
            CURL* handle = curl_easy_init();
            if (!handle) return false;
            
            auto ctx = make_unique<Context>();
            ctx->cb = callback;
            ctx->upload_data = data;

            // Configure handle
            curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
            curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, ctx->error);
            curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteHandler);
            curl_easy_setopt(handle, CURLOPT_WRITEDATA, ctx.get());

            // Method configuration
            switch(method) {
                case Method::GET:
                    curl_easy_setopt(handle, CURLOPT_HTTPGET, 1L);
                    break;
                case Method::POST:
                    curl_easy_setopt(handle, CURLOPT_POST, 1L);
                    curl_easy_setopt(handle, CURLOPT_POSTFIELDS, data.c_str());
                    curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, data.size());
                    break;
                case Method::PUT:
                    curl_easy_setopt(handle, CURLOPT_UPLOAD, 1L);
                    curl_easy_setopt(handle, CURLOPT_READFUNCTION, ReadHandler);
                    curl_easy_setopt(handle, CURLOPT_READDATA, ctx.get());
                    curl_easy_setopt(handle, CURLOPT_INFILESIZE_LARGE, 
                                   (curl_off_t)data.size());
                    break;
                case Method::DELETE:
                    curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, "DELETE");
                    break;
                case Method::PATCH:
                    curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, "PATCH");
                    curl_easy_setopt(handle, CURLOPT_POSTFIELDS, data.c_str());
                    curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, data.size());
                    break;
                case Method::HEAD:
                    curl_easy_setopt(handle, CURLOPT_NOBODY, 1L);
                    break;
                case Method::OPTIONS:
                    curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, "OPTIONS");
                    break;
            }

            // Headers
            struct curl_slist* headers = nullptr;
            {
                lock_guard<mutex> lock(headers_mutex);
                for (const auto& h : this->headers) {
                    headers = curl_slist_append(headers, h.c_str());
                }
            }
            for (const auto& h : req_headers) {
                headers = curl_slist_append(headers, h.c_str());
            }
            if (headers) {
                curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);
            }

            // Global config
            lock_guard<mutex> lock(config_mutex);
            if (timeout_ms > 0) {
                curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, timeout_ms);
            }
            curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, verify_ssl ? 1L : 0L);
            curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, verify_ssl ? 2L : 0L);
            if (!proxy.empty()) {
                curl_easy_setopt(handle, CURLOPT_PROXY, proxy.c_str());
            }

            // Execute request
            CURLcode res = curl_easy_perform(handle);

            // Cleanup
            if (headers) curl_slist_free_all(headers);
            curl_easy_cleanup(handle);

            // Rethrow any exception caught during streaming
            if (ctx->exception) {
                rethrow_exception(ctx->exception);
            }

            // Final error check
            if (res != CURLE_OK) {
                cerr << "cURL error (" << res << "): " << ctx->error << "\n";
                return false;
            }

            // Flush remaining buffer
            if (!ctx->buffer.empty()) {
                ctx->cb(ctx->buffer);
            }

            return true;
        }

        // void cancel() {
        //     lock_guard<mutex> lock(cancel_mutex);
        //     cancelled = true;
        // }

        // === Convenience Methods ===
        bool GET(
            const string& url, 
            const StreamCallback& callback,
            const vector<string>& headers = {}
        ) {
            return Request(Method::GET, url, callback, headers);
        }

        bool POST(
            const string& url,
            const StreamCallback& callback,
            const string& data,
            const vector<string>& headers = {}
        ) {
            return Request(Method::POST, url, callback, headers, data);
        }

        bool PUT(
            const string& url,
            const StreamCallback& callback,
            const string& data,
            const vector<string>& headers = {}
        ) {
            return Request(Method::PUT, url, callback, headers, data);
        }

        bool DELETE(
            const string& url,
            const StreamCallback& callback,
            const vector<string>& headers = {}
        ) {
            return Request(Method::DELETE, url, callback, headers);
        }

        bool PATCH(
            const string& url,
            const StreamCallback& callback,
            const string& data,
            const vector<string>& headers = {}
        ) {
            return Request(Method::PATCH, url, callback, headers, data);
        }

        bool HEAD(
            const string& url,
            const StreamCallback& callback,
            const vector<string>& headers = {}
        ) {
            return Request(Method::HEAD, url, callback, headers);
        }

        bool OPTIONS(
            const string& url,
            const StreamCallback& callback,
            const vector<string>& headers = {}
        ) {
            return Request(Method::OPTIONS, url, callback, headers);
        }

        // === Configuration Methods ===
        void SetFollowRedirects(bool enable, int max_redirects = 5) {
            lock_guard<mutex> lock(config_mutex);
            follow_redirects = enable;
            max_redirects = max_redirects;
        }

        void SetAutoDecompress(bool enable) {
            lock_guard<mutex> lock(config_mutex);
            auto_decompress = enable;
        }

        void SetDNSCaching(long ttl_seconds) {
            lock_guard<mutex> lock(config_mutex);
            dns_cache_ttl = ttl_seconds;
        }

        void SetVerifySSL(bool verify) { 
            lock_guard<mutex> lock(config_mutex);
            verify_ssl = verify; 
        }

        void SetTimeout(long milliseconds) { 
            lock_guard<mutex> lock(config_mutex);
            timeout_ms = milliseconds; 
        }

        void AddHeader(const string& header) { 
            lock_guard<mutex> lock(headers_mutex);
            headers.push_back(header); 
        }

        void ClearHeaders() { 
            lock_guard<mutex> lock(headers_mutex);
            headers.clear(); 
        }

        void SetProxy(const string& proxy_server) { 
            lock_guard<mutex> lock(config_mutex);
            proxy = proxy_server; 
        }

    private:
        // atomic<bool> cancelled{false};
        mutex cancel_mutex;
        mutex config_mutex;
        mutex headers_mutex;
        vector<string> headers;
        bool follow_redirects = false;
        bool auto_decompress = false;
        long dns_cache_ttl = 60; // Default 60 seconds
        bool verify_ssl = true;
        long timeout_ms = 0;
        string proxy;
        inline static once_flag global_init_flag;

        static size_t WriteHandler(
            char* ptr, 
            size_t size, 
            size_t nmemb, 
            void* userdata
        ) {
            auto* ctx = static_cast<Context*>(userdata);
            const size_t total = size * nmemb;
            
            ctx->buffer.append(ptr, total);
            
            // Split on SSE event boundaries
            size_t pos = 0;
            while ((pos = ctx->buffer.find("\n\n")) != string::npos) {
                string chunk = ctx->buffer.substr(0, pos+2);
                ctx->buffer.erase(0, pos+2);
                
                try {
                    ctx->cb(chunk);
                } catch (...) {
                    ctx->exception = current_exception();
                    ctx->cancelled = true;  // Treat any exception as a cancellation signal
                    return 0;  // Abort transfer
                }

                // Check cancellation flag
                if (ctx->cancelled.load()) 
                    return 0;  // Abort transfer by returning different size
            }
            
            return total;
        }

        static size_t ReadHandler(char* buffer, size_t size, size_t nitems, void* userdata) {
            auto* ctx = static_cast<Context*>(userdata);
            const size_t buffer_size = size * nitems;
            const size_t remaining = ctx->upload_data.size() - ctx->upload_offset;
            const size_t copy_size = min(remaining, buffer_size);

            if (copy_size > 0) {
                memcpy(buffer, ctx->upload_data.data() + ctx->upload_offset, copy_size);
                ctx->upload_offset += copy_size;
            }
            
            return copy_size;
        }
    };
}