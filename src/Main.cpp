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
#define strdup      _strdup
#endif

struct Build
{
    Database*   db;
    int         projectId;
};

/** Appends a string to the end of the log for a project. */
void AppendToLog(Database& db, int projectId, const char* message)
{

    const char* queryStart = "UPDATE project_builds SET log=CONCAT(log,'";
    const char* queryEnd   = "') WHERE projectId=%d";

    size_t queryStartLength = strlen(queryStart);
    size_t queryEndLength   = strlen(queryEnd);

    size_t messageLength = strlen(message);

    size_t queryLength;
    queryLength  = queryStartLength;    // room for the first part of the query.
    queryLength += messageLength * 2;   // room for the escaped message.
    queryLength += queryEndLength;      // room for the last part of the query.

    char* query = new char[queryLength + 1];

    strcpy(query, queryStart);
    size_t length = db.EscapeString(message, messageLength, query + queryStartLength);
    sprintf(query + queryStartLength + length, queryEnd, projectId);

    db.Query(query);

    delete [] query;

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
    AppendToLog(*build->db, build->projectId, string);
    lua_pop(L, 1);
}

void luai_writeline(lua_State* L)
{
    luai_writestring(L, "\n", 1);
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

void SetProjectStatus(Database& db, int projectId, const char* error)
{
    char query[256];
    if (error != NULL)
    {
        AppendToLog(db, projectId, error);
    }
    snprintf(query, sizeof(query), "UPDATE project_builds SET state='%s', time=NOW() WHERE projectId='%d'",
        (error == NULL) ? "succeeded" : "failed", projectId);
    db.Query(query);
}

int LuaChdir(lua_State* L)
{
    const char* dir = luaL_checkstring(L, 1);
    lua_pushinteger( L, chdir(dir) );
    return 1;
}

/** Binds auxiliary functions/variables. */
void BindLuaLibrary(lua_State* L)
{
    lua_getglobal(L, "os");
    lua_pushcfunction(L, LuaChdir);
    lua_setfield(L, -2, "chdir");
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

        char** row = db.GetRow();
        
        const char* name = row[colName];
        char* command = strdup(row[colCommand]);

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
        BindLuaLibrary(L);

        // Change the status to building.
        snprintf(query, sizeof(query), "UPDATE project_builds SET state='building', time=NOW(), log='' WHERE projectId='%d'", projectId);
        db.Query(query);

        if (luaL_loadstring(L, command) == 0)
        {
            if (lua_pcall(L, 0, LUA_MULTRET, 0) != LUA_OK)
            {
                const char* error = lua_tostring(L, -1);
                SetProjectStatus(db, projectId, error);
                lua_pop(L, 1);
            }
            else
            {
                SetProjectStatus(db, projectId, NULL);
            }
        }
        else
        {
            const char* error = lua_tostring(L, -1);
            SetProjectStatus(db, projectId, error);
            lua_pop(L, 1);
        }

        // Change the status to finished.
        snprintf(query, sizeof(query), "UPDATE project_builds SET state='%s', time=NOW() WHERE projectId='%d'",
            success ? "succeeded" : "failed", projectId);
        db.Query(query);

        lua_close(L);
        L = NULL;
        
        if (workingDir != NULL)
        {
            chdir(workingDir);
            free(workingDir);
            workingDir = NULL;
        }

        free(command);
        command = NULL;
    
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
                char** row = db.GetRow();
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

    const char* configFileName = "carson.config";

    // Load the config.

    FILE* file = fopen(configFileName, "rt");
    if (file == NULL)
    {
        fprintf(stderr, "Couldn't load config file '%s'\n", configFileName);
        exit(EXIT_FAILURE);
    }

    char dbHost[256]     = { 0 };
    char dbUserName[256] = { 0 };
    char dbPassword[256] = { 0 };

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), file))
    {
        char field[256];
        char value[256];
        if (sscanf(buffer, "%s = '%s", field, value) == 2)
        {
            char* quote = strchr(value, '\'');
            if (quote == NULL) continue;
            *quote = 0;

            if (strcmp(field, "DB_HOST") == 0) strcpy(dbHost, value);
            else if (strcmp(field, "DB_USERNAME") == 0) strcpy(dbUserName, value);
            else if (strcmp(field, "DB_PASSWORD") == 0) strcpy(dbPassword, value);
        }
    }

    fclose(file);
    file = NULL;

    Database db;
    if (!db.Connect(dbHost, dbUserName, dbPassword) || !db.Select("carson"))
    {
        fprintf(stderr, "Couldn't connect to database\n");
        exit(EXIT_FAILURE);
    }
    Run(db);
    return 0;
}