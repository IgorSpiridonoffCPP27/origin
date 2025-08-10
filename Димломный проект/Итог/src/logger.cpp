#include "logger.h"
#include <ctime>

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

Logger::Logger() {
    logfile_.open("crawler_log.txt", std::ios::app);
    logfile_ << "\n\n===== NEW SESSION =====" << std::endl;
    logfile_ << "Starting at: " << __TIMESTAMP__ << std::endl;
}

Logger::~Logger() {
    logfile_ << "===== SESSION END =====" << std::endl;
    logfile_.close();
}

void Logger::log(const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::cout << "[LOG] " << message << std::endl;
    logfile_ << "[LOG] " << message << std::endl;
    logfile_.flush();
}