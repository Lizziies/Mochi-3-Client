#include "Net.hpp"
#include "Files.hpp"

#include <windows.h>
#include <winhttp.h>

#include <fstream>

namespace net {

namespace {

struct Handle {
    HINTERNET h = nullptr;
    explicit Handle(HINTERNET v = nullptr) : h(v) {}
    ~Handle() {
        if (h) WinHttpCloseHandle(h);
    }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    explicit operator bool() const { return h != nullptr; }
};

struct Request {
    Handle session, conn, req;
    DWORD status = 0;
    int64_t length = -1;
};

bool open(Request& r, const std::string& url, int timeoutMs) {
    std::wstring wurl = files::widen(url);
    wchar_t host[256] = {}, path[2048] = {};
    URL_COMPONENTSW uc{sizeof(uc)};
    uc.lpszHostName = host;
    uc.dwHostNameLength = 256;
    uc.lpszUrlPath = path;
    uc.dwUrlPathLength = 2048;
    if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &uc)) return false;

    r.session.h = WinHttpOpen(L"MochiLauncher", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME,
                              WINHTTP_NO_PROXY_BYPASS, 0);
    if (!r.session) return false;
    WinHttpSetTimeouts(r.session.h, timeoutMs, timeoutMs, timeoutMs, timeoutMs);

    r.conn.h = WinHttpConnect(r.session.h, host, uc.nPort, 0);
    if (!r.conn) return false;

    DWORD flags = uc.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0;
    r.req.h = WinHttpOpenRequest(r.conn.h, L"GET", path, nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!r.req) return false;

    static const wchar_t* headers = L"Accept: application/vnd.github+json\r\n";
    if (!WinHttpSendRequest(r.req.h, headers, (DWORD)-1, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) return false;
    if (!WinHttpReceiveResponse(r.req.h, nullptr)) return false;

    DWORD size = sizeof(r.status);
    WinHttpQueryHeaders(r.req.h, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX,
                        &r.status, &size, WINHTTP_NO_HEADER_INDEX);

    wchar_t len[32] = {};
    size = sizeof(len);
    if (WinHttpQueryHeaders(r.req.h, WINHTTP_QUERY_CONTENT_LENGTH, WINHTTP_HEADER_NAME_BY_INDEX, len, &size,
                            WINHTTP_NO_HEADER_INDEX))
        r.length = _wtoi64(len);
    return r.status == 200;
}

bool read(Request& r, const std::function<bool(const char*, size_t)>& sink) {
    DWORD avail = 0;
    std::string chunk;
    while (WinHttpQueryDataAvailable(r.req.h, &avail) && avail) {
        chunk.resize(avail);
        DWORD got = 0;
        if (!WinHttpReadData(r.req.h, chunk.data(), avail, &got)) return false;
        if (!sink(chunk.data(), got)) return false;
    }
    return true;
}

}

std::optional<std::string> get(const std::string& url, int timeoutMs) {
    Request r;
    if (!open(r, url, timeoutMs)) return std::nullopt;
    std::string body;
    bool ok = read(r, [&](const char* data, size_t n) {
        body.append(data, n);
        return body.size() < (16u << 20);
    });
    if (!ok) return std::nullopt;
    return body;
}

bool download(const std::string& url, const std::filesystem::path& to, const std::function<void(float)>& progress) {
    Request r;
    if (!open(r, url, 15000)) return false;

    std::ofstream out(to, std::ios::binary | std::ios::trunc);
    if (!out) return false;

    int64_t total = 0;
    bool ok = read(r, [&](const char* data, size_t n) {
        out.write(data, (std::streamsize)n);
        total += (int64_t)n;
        if (r.length > 0) progress(float(total) / float(r.length));
        return bool(out);
    });
    out.close();
    if (!ok) {
        std::error_code ec;
        std::filesystem::remove(to, ec);
    }
    return ok;
}

}
