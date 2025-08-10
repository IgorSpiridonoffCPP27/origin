#include "proxy_config.h"

ProxyConfig::ProxyConfig(const std::string& filename) 
    : ConfigParser(filename) {
    http_port = get_int("web_proxy.http_port");
    https_port = get_int("web_proxy.https_port");
    cert_file = get("web_proxy.cert_file");
    key_file = get("web_proxy.key_file");
    api_host = get("web_proxy.api_host");
    api_port = get("web_proxy.api_port");
    thread_pool_size = get_int("web_proxy.thread_pool_size");
}