#include "logger.h"
#include "crawler.h"
#include <csignal>
#include <windows.h>
#include <atomic>
#include <thread>
#include <iostream>
#include <string>

static std::atomic<bool> g_should_stop{false};

void signal_handler(int signal) {
    LOG("Interrupt signal (" + std::to_string(signal) + ") received");
    g_should_stop = true;
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
        LOG("Starting crawler (monitoring disabled)...");
        crawler.start();

        // Первичный обход стартовой страницы
        LOG("Crawling start URL and indexing words...");
        crawler.crawl_start_url();

        // Интерактивный режим: запрос пользователя о повторном анализе
        while (!g_should_stop) {
            LOG("Анализ завершен, слова добавлены в базу данных");
            LOG("Можете изменить ссылку в config.ini и запустить меня повторно");
            LOG("Запускаем? (y/n): ");
            
            std::string user_input;
            std::getline(std::cin, user_input);
            
            if (user_input == "y" || user_input == "Y" || user_input == "yes" || user_input == "YES") {
                // Перечитываем конфиг и проверяем изменения
                CrawlerConfig fresh_config;
                std::string current_start_url = fresh_config.get("Crowler.start_page");
                std::string last_start_url = crawler_config.get("Crowler.start_page");
                
                if (current_start_url != last_start_url) {
                    LOG("Обнаружено изменение ссылки в config.ini");
                    LOG("Новая ссылка: " + current_start_url);
                    crawler_config = fresh_config;
                    // Обновляем конфиг в краулере
                    crawler.update_config(fresh_config);
                } else {
                    LOG("Ссылка не изменилась, повторяем анализ текущей: " + current_start_url);
                }
                
                LOG("Начинаем повторный анализ...");
                crawler.crawl_start_url();
            } else if (user_input == "n" || user_input == "N" || user_input == "no" || user_input == "NO") {
                LOG("Пользователь выбрал завершение работы");
                break;
            } else {
                LOG("Неверный ввод. Пожалуйста, введите 'y' для продолжения или 'n' для завершения");
                continue;
            }
        }

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

