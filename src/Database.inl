template <typename A1>
bool Database::Query(const char* query, A1 arg1)
{
    sqlite3_stmt* dbps = QueryBegin(query);
    QueryBind(dbps, 1, arg1);
    return QueryEnd(dpbs);
}

template <typename A1, typename A2>
bool Database::Query(const char* query, A1 arg1, A2 arg2)
{
    sqlite3_stmt* dbps = QueryBegin(query);
    QueryBind(dbps, 1, arg1);
    QueryBind(dbps, 2, arg2);
    return QueryEnd(dbps);
}
