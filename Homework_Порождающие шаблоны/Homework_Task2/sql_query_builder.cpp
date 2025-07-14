#include "sql_query_builder.h"

SqlSelectQueryBuilder::SqlSelectQueryBuilder() {

}



SqlSelectQueryBuilder& SqlSelectQueryBuilder::AddColumn(const std::string column) 
{
	db.setcolumn(column);
	return *this;
}
SqlSelectQueryBuilder& SqlSelectQueryBuilder::AddColumns(const std::vector<std::string>& columns) noexcept 
{
	for (auto i : columns) {
		db.setcolumn(i);
	}
	return *this;
}


SqlSelectQueryBuilder& SqlSelectQueryBuilder::AddFrom(const std::string table) {
	db.settable(table);
	return *this;
}

SqlSelectQueryBuilder& SqlSelectQueryBuilder::AddWhere(const std::map<std::string, std::string>& kv) noexcept {
	for (auto i : kv) {
		db.setsearch(i.first, i.second);
	}
	return *this;
};

SqlSelectQueryBuilder& SqlSelectQueryBuilder::AddWhere(const std::string name, const std::string value) {
	db.setsearch(name,value);
	return *this;
}

std::string SqlSelectQueryBuilder::BuildQuery() {

	std::string query_search;
	bool first = true;
	for (const auto& [key, value] : db.getsearch()) {
		if (!first) {
			query_search += " AND ";
		}
		query_search += key + "=" + value;
		first = false;
	}

	std::string query_table=db.gettable();
	
	first = true;
	std::string query_column;
	for (auto text : db.getcolumn())
	{
		if (!first) {
			query_column += ", ";
		}
		query_column += text;
		first = false;
	}

	db.getsearch().clear();
	db.getcolumn().clear();

	if (query_column != "") {
		return "SELECT " + query_column + " FROM " + query_table + " WHERE " + query_search + ";";
		}
	else {
		return "SELECT * FROM " + query_table + " WHERE " + query_search + ";";
	}

}