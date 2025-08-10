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
        LOG("Initializing crawler...");
        Crawler crawler(db, crawler_config);
        LOG("Starting crawler...");
        crawler.start();

        auto initial_words = db.get_all_words();
        LOG("Found " + std::to_string(initial_words.size()) + " initial words");

        for (const auto& word : initial_words) {
            int word_id = db.get_word_id(word);
            if (word_id != -1) {
                std::string wiki_url = crawler_config.get("Crowler.start_page") + crawler.prepare_wiki_url(word);
                if (force_restart || !db.url_exists_for_word(word_id, wiki_url)) {
                    LOG("Scheduling initial word: " + word);
                    net::post(crawler.get_pool(), [&crawler, word]() { crawler.process_word(word); });
                } else {
                    LOG("Skipping already processed word: " + word);
                }
            }
        }

        LOG("Crawler started. Press Enter to exit...");
        std::cin.get();
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

