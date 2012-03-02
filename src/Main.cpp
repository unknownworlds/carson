#include "Database.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <direct.h>

extern "C"
{
#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"
}

#ifdef WIN32
#include <windows.h>
#define snprintf    _snprintf
#define popen       _popen
#define pclose      _pclose
#define getcwd      _getcwd
#define chdir       _chdir
#endif

struct Build
{
    Database*   db;
    int         projectId;
};

void UpdateLog(Build* build, const char* string, size_t length)
{
    // Update the stored log.
    build->db->Query("UPDATE project_builds SET log=log||? WHERE projectId=?", string, build->projectId );
}

extern "C"
{

/** This is used to just provide us with a unique address we can use as a key
in the Lua registry. */
static int _buildTag;
    
void luai_writestring(lua_State* L, const char* string, size_t length)
{
    lua_pushlightuserdata(L, &_buildTag);
    lua_rawget(L, LUA_REGISTRYINDEX);
    Build* build = static_cast<Build*>(lua_touserdata(L, -1));
    assert(build != NULL);

    UpdateLog(build, string, length);
    lua_pop(L, 1);
}

void luai_writeline(lua_State* L)
{
    lua_pushlightuserdata(L, &_buildTag);
    lua_rawget(L, LUA_REGISTRYINDEX);
    Build* build = static_cast<Build*>(lua_touserdata(L, -1));
    assert(build != NULL);

    luai_writestring(L, "\n", 1);
    fflush(stdout);

    lua_pop(L, 1);
}

void luai_writestringerror(lua_State* L, const char* format, const char* param)
{
    lua_pushlightuserdata(L, &_buildTag);
    lua_rawget(L, LUA_REGISTRYINDEX);
    Build* build = static_cast<Build*>(lua_touserdata(L, -1));
    assert(build != NULL);

    fprintf(stderr, format, param);
    fflush(stderr);

    lua_pop(L, 1);
}

}

void Thread_Sleep(int msecs)
{
    Sleep(msecs);
}

/** Creates the tables in the datbase */
void Install(Database& db)
{
    db.Query("CREATE TABLE projects (id INTEGER PRIMARY KEY, name TINYTEXT, trigger MEDIUMTEXT, command MEDIUMTEXT)");
    db.Query("CREATE TABLE project_builds (id INTEGER PRIMARY KEY, projectId INTEGER, state TINYTEXT, time DATETIME, log LONGTEXT)");
}

/** Initiates building of the project with the specified id. */
void BuildProject(Database& db, int projectId)
{

    char query[512];

    // Get the information about the build.

    snprintf(query, sizeof(query), "SELECT * FROM projects WHERE id='%d'", projectId);
    db.Query(query);

    if (db.GetNumRows() > 0)
    {

        int colName    = db.GetColumn("name");
        int colCommand = db.GetColumn("command");

        char** row = db.GetRow(0);
        
        const char* name = row[colName];
        printf("Running '%s'\n", name);

        bool success = true;
        
        // Save off the working directory, since the script may change it.
        char* workingDir = getcwd(NULL, 0);
        
        lua_State* L = luaL_newstate();

        // Store information about the build we're running in the Lua state so
        // that it can be accessed from the print callbacks.
    
        Build build;
        build.db         = &db;
        build.projectId = projectId;

        lua_pushlightuserdata(L, &_buildTag);
        lua_pushlightuserdata(L, &build);
        lua_rawset(L, LUA_REGISTRYINDEX);

        luaL_openlibs(L);

        if (luaL_loadstring(L, row[colCommand]) == 0)
        {

            // Change the status to building.
            snprintf(query, sizeof(query), "UPDATE project_builds SET state='building', time=date('now'), log='' WHERE projectId='%d'", projectId);
            db.Query(query);

            if (lua_pcall(L, 0, LUA_MULTRET, 0) != LUA_OK)
            {
                const char* error = lua_tostring(L, -1);
                fprintf(stderr, "%s", error);
                lua_pop(L, 1);
            }

            // Change the status to finished.
            snprintf(query, sizeof(query), "UPDATE project_builds SET state='%s', time=date('now') WHERE projectId='%d'",
                success ? "succeeded" : "failed", projectId);
            db.Query(query);
        
        }

        lua_close(L);
        L = NULL;
        
        if (workingDir != NULL)
        {
            chdir(workingDir);
            free(workingDir);
        }
    
    }

}

void Run(Database& db)
{
    const int sleepInverval = 1000 * 5;
    while (1)
    {

        Thread_Sleep(sleepInverval);

        db.Query("SELECT * FROM project_builds");

        // Check for a pending request.
        if (db.GetNumRows() > 0)
        {

            int colProjectId = db.GetColumn("projectId");
            int colState     = db.GetColumn("state");

            if (colProjectId == -1 || colState == -1)
            {
                fprintf(stderr, "Bad database format\n");
                continue;
            }

            for (int i = 0; i < db.GetNumRows(); ++i)
            {
                char** row = db.GetRow(i);
                if (strcmp(row[colState], "pending") == 0)
                {
                    int projectId = atoi(row[colProjectId]);
                    BuildProject(db, projectId);
                    break;
                }
            }

        }
        
    }
}

int main(int argc, char* argv[])
{

    const char* dbFileName = "carson.db";

    Database db;
    db.Open(dbFileName);
    
    // If there's nothing in the database, install our tables.
    db.Query("select * from sqlite_master");
    if (db.GetNumRows() == 0)
    {
        printf("Setting up database\n");
        Install(db);
    }

    Run(db);

    return 0;

}