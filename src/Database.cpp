//==============================================================================
//
// carson/Database.cpp
//
// Created by Max McGuire (max@unknownworlds.com)
// Copyright (c) 2012, Unknown Worlds Entertainment, Inc.
//
//==============================================================================

#include "Database.h"

#ifdef WIN32
#include <windows.h>
#endif

#include <mysql.h>
#include <stdio.h>
#include <string.h>

Database::Database()
{
    m_numRows = 0;
    m_numCols = 0;
    m_result  = NULL;
    m_field   = NULL;
}

Database::~Database()
{
    Close();
}

bool Database::Connect(const char* host, const char* userName, const char* password)
{
    m_db = mysql_init(NULL);
    if (m_db == NULL)
    {
        return false;
    }
    if (!mysql_real_connect(m_db, host, userName, password, NULL, 0, NULL, 0))
    {
        Close();
        return false;
    }
    return true;
}

void Database::Close()
{
    FreeLastQuery();
    if (m_db != NULL)
    {
        mysql_close(m_db);
        m_db = NULL;
    }
}

bool Database::Select(const char* database)
{
    return mysql_select_db(m_db, database) == 0;
}

bool Database::Query(const char* query)
{

    FreeLastQuery();

    if (mysql_query(m_db, query) != 0)
    {
        fprintf(stderr, "Error: %s in query '%s'\n", mysql_error(m_db), query);
        return false;
    }

    m_result = mysql_store_result(m_db);

    if (m_result != NULL)
    {
        m_numCols = mysql_num_fields(m_result);
        m_numRows = static_cast<int>(mysql_num_rows(m_result));
        m_field   = mysql_fetch_fields(m_result);
    }

    return true;

}

int Database::GetNumRows() const
{
    return m_numRows;
}

int Database::GetColumn(const char* name) const
{
    for (int i = 0; i < m_numCols; ++i)
    {
        if (strcmp(m_field[i].name, name) == 0)
        {
            return i;
        }
    }
    return -1;
}

char** Database::GetRow() const
{
    MYSQL_ROW row = mysql_fetch_row(m_result);
    return row;
}

void Database::FreeLastQuery()
{
    if (m_result != NULL)
    {
        mysql_free_result(m_result);
        m_result = NULL;
        m_field  = NULL;
    }
}

size_t Database::EscapeString(const char* src, size_t srcLength, char* dst)
{
    return mysql_real_escape_string(m_db, dst, src, static_cast<unsigned long>(srcLength));
}