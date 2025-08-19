#include "logger.h"
#include "crawler.h"
#include <csignal>
#include <windows.h>

void signal_handler(int signal) {
    LOG("Interrupt signal (" + std::to_string(signal) + ") received");
    exit(signal);
}

int main(int argc, char* argv[]) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    std::set_terminate([]() {
        LOG("Terminate called!");
        if (auto exc = std::current_exception()) {
            try {
                std::rethrow_exception(exc);
            } catch (const std::exception& e) {
                LOG("Uncaught exception: " + std::string(e.what()));
            } catch (...) {
                LOG("Unknown exception");
            }
        }
        LOG("Aborting...");
        std::abort();
    });

    bool force_restart = false;
    if (argc > 1 && std::string(argv[1]) == "--force") {
        force_restart = true;
        LOG("Force restart mode enabled");
    }

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    try {
        CrawlerConfig crawler_config;
        LOG("Initializing database...");
        DBuse db(crawler_config.get("Database.host"), crawler_config.get("Database.dbname"), 
                crawler_config.get("Database.user"), crawler_config.get("Database.password"));
        LOG("Creating tables...");
        db.create_tables();
        db.run_migrations();
        LOG("Initializing crawler...");
        Crawler crawler(db, crawler_config);
        LOG("Starting crawler...");
        crawler.start();

        // New behavior: crawl the start URL from config and index all words
        LOG("Crawling start URL and indexing words...");
        crawler.crawl_start_url();

        LOG("Stopping crawler...");
        crawler.stop();
        LOG("Crawler stopped");
    } catch (const std::exception& e) {
        LOG("Fatal error: " + std::string(e.what()));
        return 1;
    } catch (...) {
        LOG("Unknown fatal error");
        return 2;
    }
    LOG("Application exited normally");
    return 0;
}

