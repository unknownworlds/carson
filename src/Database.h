#ifndef DATABASE_H
#define DATABASE_H

struct st_mysql;
struct st_mysql_res;
struct st_mysql_field;

class Database
{

public:

    Database();

    bool Connect(const char* host, const char* userName, const char* password);
    void Close();

    bool Select(const char* database);

    bool Query(const char* query);
    
    /** Returns the result from the previous query for the specified column. */
    const char* GetResult(const char* colName);

    /** Returns the number of rows returned from the last query. */
    int GetNumRows() const;

    /** Returns the index of the column returned by the last query with the 
    specified name. If the column didn't exist, returns -1. */
    int GetColumn(const char* name) const;

    /** Returns an array of values for the next row. The array should be
    indexed using values returned from GetColumn. */
    char** GetRow() const;

    /** The dst buffer should be allocated to include 2*srcLength + 1 characters */
    size_t EscapeString(const char* src, size_t srcLength, char* dst);

private:

    void FreeLastQuery();

private:

    bool            m_verbose;

    st_mysql*       m_db;
    int             m_numRows;
    int             m_numCols;
    st_mysql_res*   m_result;
    st_mysql_field* m_field;

};

#endif