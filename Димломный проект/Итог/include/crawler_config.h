#pragma once
#include "config_parser.h"
#include <chrono>

struct CrawlerConfig : public ConfigParser {
    int max_redirects;
    int recursion_depth;
    int max_connections;
    int max_links_per_page;
    std::chrono::seconds poll_interval;

    CrawlerConfig(const std::string& filename = "../../config.ini");
};