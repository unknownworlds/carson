#include "Database.h"

#include <stdio.h>
#include <string.h>

#include "sqlite3/sqlite3.h"

Database::Database()
{
    m_db        = NULL;
    m_result    = NULL;
    m_numRows   = 0;
    m_numCols   = 0;
    m_verbose   = false;
}

bool Database::Open(const char* dbFileName)
{
    if (sqlite3_open(dbFileName, &m_db) != SQLITE_OK)
    {
        sqlite3_close(m_db);
        return false;
    }
    return true;
}

void Database::Close()
{
    FreeQueryResult();
    sqlite3_close(m_db);
    m_db = NULL;
}

bool Database::Query(const char* query)
{

    // Free any previous queries result.
    FreeQueryResult();

    bool success = false;

    char* errorMessage;
    if ( sqlite3_get_table(m_db, query, &m_result, &m_numRows, &m_numCols, &errorMessage) != SQLITE_OK )
    {
        fprintf(stderr, "SQL error: %s\n", errorMessage);
        sqlite3_free(errorMessage);
        success = true;
    }
    else if (m_verbose)
    {
        printf("%s\n", query);
        for (int r = 0; r < m_numRows; ++r)
        {
            printf("Row %d:\n", r);
            for (int c = 0; c < m_numCols; ++c)
            {
                printf("    %s = %s\n", m_result[c], m_result[m_numCols + r * m_numRows + c]);
            }
        }
    }
    return success;
}

int Database::GetNumRows() const
{
    return m_numRows;
}

int Database::GetColumn(const char* name) const
{
    for (int i = 0; i < m_numCols; ++i)
    {
        if (strcmp(m_result[i], name) == 0)
        {
            return i;
        }
    }
    return -1;
}

char** Database::GetRow(int row) const
{
    return m_result + (row + 1) * m_numCols;
}

void Database::FreeQueryResult()
{
    if (m_result != NULL)
    {
        sqlite3_free_table(m_result);
        m_result = NULL;
        m_numRows = 0;
        m_numCols = 0;
    }
}

void Database::QueryBind(sqlite3_stmt* statement, int index, int value)
{
    sqlite3_bind_int(statement, index, value);
}

void Database::QueryBind(sqlite3_stmt* statement, int index, const char* value)
{
    sqlite3_bind_text(statement, index, value, -1, SQLITE_TRANSIENT);
}

sqlite3_stmt* Database::QueryBegin(const char* query)
{
    sqlite3_stmt* dbps;
    sqlite3_prepare_v2(m_db, query, -1, &dbps, NULL);
    return dbps;
}

bool Database::QueryEnd(sqlite3_stmt* dbps)
{
    int result = sqlite3_step(dbps);
    sqlite3_finalize(dbps); 
    return result == SQLITE_DONE;
}