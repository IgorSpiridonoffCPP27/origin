#include "url_tools.h"

std::string url_encode(const std::string& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (auto c : value) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (std::isalnum(uc) || uc == '-' || uc == '_' || uc == '.' || uc == '~') {
            escaped << c;
        } else {
            escaped << '%' << std::setw(2) << static_cast<int>(uc);
        }
    }
    return escaped.str();
}

std::string normalize_url(const std::string& url, const std::string& base_url) {
    if (url.empty() || url[0] == '#' || url.find("javascript:") == 0) {
        return "";
    }
    if (url.find("://") != std::string::npos) {
        return url;
    }
    // protocol-relative URL (//example.com/path)
    if (url.rfind("//", 0) == 0) {
        size_t protocol_pos = base_url.find("://");
        if (protocol_pos != std::string::npos) {
            std::string proto = base_url.substr(0, protocol_pos);
            return proto + ":" + url;
        }
    }
    size_t protocol_pos = base_url.find("://");
    if (protocol_pos == std::string::npos) {
        return "";
    }
    size_t domain_end = base_url.find('/', protocol_pos + 3);
    if (domain_end == std::string::npos) {
        domain_end = base_url.length();
    }
    if (!url.empty() && url[0] == '/') {
        return base_url.substr(0, domain_end) + url;
    } else {
        size_t last_slash = base_url.rfind('/');
        if (last_slash != std::string::npos && last_slash >= protocol_pos + 3) {
            return base_url.substr(0, last_slash + 1) + url;
        }
    }
    return "";
}

std::string get_host(const std::string& url) {
    size_t protocol_pos = url.find("://");
    if (protocol_pos == std::string::npos) {
        return "";
    }
    size_t host_start = protocol_pos + 3;
    size_t host_end = url.find('/', host_start);
    if (host_end == std::string::npos) {
        host_end = url.length();
    }
    return url.substr(host_start, host_end - host_start);
}

// Кодирует только не-ASCII и пробелы в %HH, оставляя / ? & = : и др. разделители
std::string encode_http_target(const std::string& target) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;
    for (unsigned char uc : target) {
        if (uc <= 0x20 || uc >= 0x7F) {
            escaped << '%' << std::uppercase << std::setw(2) << int(uc) << std::nouppercase;
        } else {
            switch (uc) {
                case ' ': escaped << "%20"; break;
                default: escaped << static_cast<char>(uc); break;
            }
        }
    }
    return escaped.str();
}