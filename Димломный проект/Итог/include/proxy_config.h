#pragma once
#include "config_parser.h"
#include <string>

struct ProxyConfig : public ConfigParser {
    unsigned short http_port;
    unsigned short https_port;
    std::string cert_file;
    std::string key_file;
    std::string api_host;
    std::string api_port;
    int thread_pool_size;

    ProxyConfig(const std::string& filename = "../../config.ini");
};