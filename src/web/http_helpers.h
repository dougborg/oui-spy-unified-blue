#pragma once

#include <Print.h>
#include <esp_http_server.h>

#include <FS.h>

// ============================================================================
// HTTP Helper Utilities for esp_http_server migration
// ============================================================================

namespace web {

// ----------------------------------------------------------------------------
// ChunkedPrint — Print subclass wrapping httpd_req_t* for chunked streaming
// Allows module methods to write via Print& (framework-agnostic)
// ----------------------------------------------------------------------------
class ChunkedPrint : public Print {
  public:
    ChunkedPrint(httpd_req_t* req, const char* contentType) : _req(req) {
        httpd_resp_set_type(req, contentType);
    }

    size_t write(uint8_t c) override {
        char ch = (char)c;
        httpd_resp_send_chunk(_req, &ch, 1);
        return 1;
    }

    size_t write(const uint8_t* buf, size_t size) override {
        if (size > 0) {
            httpd_resp_send_chunk(_req, (const char*)buf, size);
        }
        return size;
    }

    // Call when done writing to signal end of chunked response
    esp_err_t end() {
        return httpd_resp_send_chunk(_req, nullptr, 0);
    }

  private:
    httpd_req_t* _req;
};

// ----------------------------------------------------------------------------
// Form/Query param helpers
// ----------------------------------------------------------------------------

// Read all form params at once (since httpd_req_recv can only be called once)
// Stores raw body in provided buffer, then use httpd_query_key_value on it
inline int readFormBody(httpd_req_t* req, char* buf, int bufSize) {
    int contentLen = req->content_len;
    if (contentLen <= 0 || contentLen >= bufSize)
        return -1;
    int received = httpd_req_recv(req, buf, contentLen);
    if (received <= 0)
        return -1;
    buf[received] = '\0';
    return received;
}

inline bool getFormValue(const char* body, const char* key, String& out) {
    char valueBuf[512];
    if (httpd_query_key_value(body, key, valueBuf, sizeof(valueBuf)) == ESP_OK) {
        out = valueBuf;
        return true;
    }
    return false;
}

// Parse a key from URL query string
inline bool getQueryParam(httpd_req_t* req, const char* key, String& out) {
    int queryLen = httpd_req_get_url_query_len(req);
    if (queryLen <= 0)
        return false;

    char* queryStr = (char*)malloc(queryLen + 1);
    if (!queryStr)
        return false;

    if (httpd_req_get_url_query_str(req, queryStr, queryLen + 1) != ESP_OK) {
        free(queryStr);
        return false;
    }

    char valueBuf[256];
    esp_err_t err = httpd_query_key_value(queryStr, key, valueBuf, sizeof(valueBuf));
    free(queryStr);

    if (err == ESP_OK) {
        out = valueBuf;
        return true;
    }
    return false;
}

// ----------------------------------------------------------------------------
// Response helpers
// ----------------------------------------------------------------------------

inline esp_err_t sendJSON(httpd_req_t* req, int status, const char* json) {
    char statusBuf[32];
    const char* reason;
    switch (status) {
    case 200: reason = "OK"; break;
    case 400: reason = "Bad Request"; break;
    case 404: reason = "Not Found"; break;
    case 500: reason = "Internal Server Error"; break;
    default:
        snprintf(statusBuf, sizeof(statusBuf), "%d Error", status);
        reason = nullptr;
        break;
    }
    if (reason) {
        snprintf(statusBuf, sizeof(statusBuf), "%d %s", status, reason);
    }
    httpd_resp_set_status(req, statusBuf);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, json);
}

inline esp_err_t sendError(httpd_req_t* req, int status, const char* msg) {
    String json = "{\"error\":\"";
    json += msg;
    json += "\"}";
    return sendJSON(req, status, json.c_str());
}

inline esp_err_t sendFile(httpd_req_t* req, fs::FS& fs, const char* path, const char* contentType,
                          const char* disposition) {
    File f = fs.open(path, "r");
    if (!f) {
        return sendError(req, 404, "file not found");
    }

    httpd_resp_set_type(req, contentType);
    if (disposition) {
        httpd_resp_set_hdr(req, "Content-Disposition", disposition);
    }

    char buf[512];
    while (f.available()) {
        int bytesRead = f.read((uint8_t*)buf, sizeof(buf));
        if (bytesRead > 0) {
            httpd_resp_send_chunk(req, buf, bytesRead);
        }
    }
    f.close();
    httpd_resp_send_chunk(req, nullptr, 0);
    return ESP_OK;
}

// Helper to register a URI handler on both HTTPS and HTTP servers
inline void registerOnBoth(httpd_handle_t https, httpd_handle_t http, const httpd_uri_t* uri) {
    if (https)
        httpd_register_uri_handler(https, uri);
    if (http)
        httpd_register_uri_handler(http, uri);
}

} // namespace web
