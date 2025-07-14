#pragma once
#include<iostream>
#include <string.h>
#include <vector>
#include <map>


class database {
	std::vector<std::string> column;
	std::string table;
	std::map<std::string, std::string> search;

public:
	void setcolumn(const std::string col) {
		column.push_back(col);
	}
	void settable(const std::string tab) {
		table=tab;
	}
	void setsearch(const std::string name, const std::string value) {
		search[name] = value;
	}

	std::vector<std::string>& getcolumn() {
		return column;
	}
	std::string& gettable() {
		return table;
	}
	std::map<std::string, std::string>& getsearch() {
		return search;
	}
	
};