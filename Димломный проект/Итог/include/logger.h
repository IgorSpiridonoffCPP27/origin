#pragma once
#include <fstream>
#include <mutex>
#include <iostream>

class Logger {
public:
    static Logger& instance();
    void log(const std::string& message);
    
private:
    Logger();
    ~Logger();
    
    std::ofstream logfile_;
    std::mutex mutex_;
};

#define LOG(msg) Logger::instance().log(msg)