#pragma once
#include "pch.h"
class DBuse{
public:
    DBuse(const std::string&, const std::string&,const std::string&,const std::string&);
    void create_tables();
    void add_word_to_tables(const std::string& );
    void add_unique_constraint();
private:

    pqxx::connection conn;


};