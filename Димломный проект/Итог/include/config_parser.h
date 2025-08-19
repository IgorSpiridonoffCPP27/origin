// config_parser.h
#pragma once
#include <map>
#include <string>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <vector>
#include <filesystem>

class ConfigParser {
protected:
    std::map<std::string, std::string> config_map;

    void load_config(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open config file: " + filename);
        }

        std::string line;
        std::string current_section;
        while (std::getline(file, line)) {
            // Удаление комментариев
            size_t comment_pos = line.find(';');
            if (comment_pos != std::string::npos) {
                line = line.substr(0, comment_pos);
            }
            
            // Удаление пробелов и кавычек (исправлено: безопасно приводим к unsigned char)
            line.erase(std::remove_if(line.begin(), line.end(),
                       [](char c) { return std::isspace(static_cast<unsigned char>(c)) || c == '"'; }),
                       line.end());
            
            if (line.empty()) continue;
            
            if (line.front() == '[' && line.back() == ']') {
                current_section = line.substr(1, line.size() - 2);
            } else {
                size_t delimiter = line.find('=');
                if (delimiter != std::string::npos) {
                    std::string key = line.substr(0, delimiter);
                    std::string value = line.substr(delimiter + 1);
                    config_map[current_section + "." + key] = value;
                }
            }
        }
    }

public:
    ConfigParser(const std::string& filename = "../../config.ini") {
        std::vector<std::string> candidates;
        candidates.push_back(filename);
        candidates.push_back("config.ini");
        candidates.push_back("./config.ini");
        candidates.push_back("../config.ini");
        candidates.push_back("../../config.ini");

        bool loaded = false;
        for (const auto& p : candidates) {
            try {
                if (std::filesystem::exists(std::filesystem::path(p))) {
                    load_config(p);
                    loaded = true;
                    break;
                }
            } catch (...) {
                // ignore and try next
            }
        }
        if (!loaded) {
            throw std::runtime_error("Cannot open config file: " + filename);
        }
    }

    std::string get(const std::string& key, const std::string& default_value = "") const {
        auto it = config_map.find(key);
        return (it != config_map.end()) ? it->second : default_value;
    }

    int get_int(const std::string& key, int default_value = 0) const {
        try {
            return std::stoi(get(key, std::to_string(default_value)));
        } catch (...) {
            return default_value;
        }
    }
};