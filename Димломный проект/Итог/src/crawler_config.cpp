#include "crawler_config.h"

CrawlerConfig::CrawlerConfig(const std::string& filename) 
    : ConfigParser(filename) {
    max_redirects = get_int("Crowler.max_redirects");
    recursion_depth = get_int("Crowler.recursion_depth");
    max_connections = get_int("Crowler.max_connections");
    max_links_per_page = get_int("Crowler.max_links_per_page");
    poll_interval = std::chrono::seconds(get_int("Crowler.poll_interval"));
}