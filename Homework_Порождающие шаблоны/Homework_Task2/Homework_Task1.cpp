#include "sql_query_builder.h"
#include <cassert>

int main()
{
    SqlSelectQueryBuilder query_builder;
    query_builder.AddColumn("name").AddColumn("phone");
    query_builder.AddFrom("students");
    query_builder.AddWhere("id", "42").AddWhere("name", "John");
    std::string query = query_builder.BuildQuery();
    std::cout << query << std::endl;

    query_builder.AddColumns({ "FIO", "email" });
    query_builder.AddFrom("people");
    query_builder.AddWhere("id", "409").AddWhere("FIO", "SON");

    query = query_builder.BuildQuery();
    std::cout << query << std::endl;
}