#ifndef DATABASE_H
#define DATABASE_H

struct sqlite3;
struct sqlite3_stmt;

class Database
{

public:

    Database();

    bool Open(const char* dbFileName);
    void Close();

    bool Query(const char* query);
    
    /** Query with parameter substitution. */
    template <typename A1>
    bool Query(const char* query, A1 arg1);

    template <typename A1, typename A2>
    bool Query(const char* query, A1 arg1, A2 arg2);

    /** Returns the result from the previous query for the specified column. */
    const char* GetResult(const char* colName);

    /** Returns the number of rows returned from the last query. */
    int GetNumRows() const;

    /** Returns the index of the column returned by the last query with the 
    specified name. If the column didn't exist, returns -1. */
    int GetColumn(const char* name) const;

    /** Returns an array of values for the specified row. The array should be
    indexed using values returned from GetColumn. */
    char** GetRow(int row) const;

private:

    void FreeQueryResult();

    sqlite3_stmt* QueryBegin(const char* query);

    void QueryBind(sqlite3_stmt* statement, int index, int arg);
    void QueryBind(sqlite3_stmt* statement, int index, const char* arg);

    bool QueryEnd(sqlite3_stmt* statement);

private:

    bool        m_verbose;

    sqlite3*    m_db;
    int         m_numRows;
    int         m_numCols;
    char**      m_result;

};

#include "Database.inl"

#endif