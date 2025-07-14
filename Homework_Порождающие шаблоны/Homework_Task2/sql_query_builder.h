#pragma once
#include "database.h"
class SqlSelectQueryBuilder {
	database db;
public:
	SqlSelectQueryBuilder();
	SqlSelectQueryBuilder& AddColumn(const std::string);
	SqlSelectQueryBuilder& AddFrom(const std::string );
	SqlSelectQueryBuilder& AddWhere(const std::string, const std::string);
	std::string BuildQuery();
	SqlSelectQueryBuilder& AddWhere(const std::map<std::string, std::string>& kv) noexcept;
	SqlSelectQueryBuilder& AddColumns(const std::vector<std::string>& columns) noexcept;
};