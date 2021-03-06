//==============================================================================
//
// carson/Main.cpp
//
// Created by Max McGuire (max@unknownworlds.com)
// Copyright (c) 2012, Unknown Worlds Entertainment, Inc.
//
//==============================================================================

#include "Database.h"
#include "Process.h"

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
#define putenv      _putenv
#endif

struct Build
{

    Build() { bufferLength = 0; }

    Database*   db;
    int         projectId;
    int         exitCode;
    bool        log;
    char        buffer[160];
    size_t      bufferLength;

};

void FlushBuffer(Build* build)
{

    if (build->bufferLength == 0)
    {
        return;
    }

#define QUERY_START "UPDATE project_builds SET log=CONCAT(log,'"
#define QUERY_END   "') WHERE projectId=%d"

    const size_t queryStartLength = sizeof(QUERY_START) - 1;
    const size_t queryEndLength   = sizeof(QUERY_END) - 1;

    char query[queryStartLength + sizeof(build->buffer) * 2 + queryEndLength + 1];

    strcpy(query, QUERY_START);
    size_t length = build->db->EscapeString(build->buffer, build->bufferLength, query + queryStartLength);
    sprintf(query + queryStartLength + length, QUERY_END, build->projectId);

    build->db->Query(query);
    build->bufferLength = 0;

#undef QUERY_START
#undef QUERY_END

}

void ClearLog(Build* build)
{
    char query[256];
    snprintf(query, sizeof(query), "UPDATE project_builds SET log='' WHERE projectId='%d'", build->projectId);
    build->db->Query(query);
}

/** Appends a string to the end of the log for a project. */
void AppendToLog(Build* build, const char* message, size_t messageLength)
{

    if (message == NULL)
    {
        return;
    }

    while (messageLength > 0)
    {

        // Copy into the buffer.

        size_t maxLength = sizeof(build->buffer) - build->bufferLength;
        size_t length = messageLength;

        if (length > maxLength)
        {
            length = maxLength;
        }

        memcpy(build->buffer + build->bufferLength, message, length);
        build->bufferLength += length;
        
        message += length;
        messageLength -= length;

        if (build->bufferLength == sizeof(build->buffer))
        {
            FlushBuffer(build);
        }

    }

}

void AppendToLog(Build* build, const char* message)
{
    if (message != NULL)
    {
        AppendToLog(build, message, strlen(message));
    }
}

/** This is used to just provide us with a unique address we can use as a key
in the Lua registry. */
static int _buildTag;
static int _atexitTag;

extern "C"
{
    
void luai_writestring(lua_State* L, const char* string, size_t length)
{
    lua_pushlightuserdata(L, &_buildTag);
    lua_rawget(L, LUA_REGISTRYINDEX);
    Build* build = static_cast<Build*>(lua_touserdata(L, -1));
    assert(build != NULL);
    if (build->log)
    {
        AppendToLog(build, string, length);
    }

    // Update the log variable.
    lua_getglobal(L, "_LOG");
    lua_pushlstring(L, string, length);
    lua_concat(L, 2);
    lua_setglobal(L, "_LOG");

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

void SetProjectStatus(Database& db, int projectId, int exitCode)
{
    char query[256];
    snprintf(query, sizeof(query), "UPDATE project_builds SET state='%s', time=NOW() WHERE projectId='%d'",
        (exitCode == EXIT_SUCCESS) ? "succeeded" : "failed", projectId);
    db.Query(query);
}

int OsChdir(lua_State* L)
{
    const char* dir = luaL_checkstring(L, 1);
    lua_pushinteger( L, chdir(dir) );
    return 1;
}

int OsExecute(lua_State* L)
{
    struct Locals
    {
        static void Callback(void* userData, const char* string, size_t length)
        {
            lua_State* L = static_cast<lua_State*>(userData);
            luai_writestring(L, string, length);
        }
	};

    const char* command = lua_tostring(L, 1);
	if (command == NULL)
	{
		int stat = system(NULL);
		lua_pushboolean(L, stat);
		return 1;
	}

    int result = 0;
	Process_Run(L, command, Locals::Callback, &result);

	lua_pushboolean(L, result != 0);
	lua_pushstring(L, "exit");
    lua_pushinteger(L, result);
    return 3;
}

int OsCapture(lua_State* L)
{
    struct Locals
    {
        static void Callback(void* userData, const char* string, size_t length)
        {
            lua_State* L = static_cast<lua_State*>(userData);
            lua_pushlstring(L, string, length);
            lua_concat(L, 2);
        }
        static void CallbackWithMirror(void* userData, const char* string, size_t length)
        {
            lua_State* L = static_cast<lua_State*>(userData);
            luai_writestring(L, string, length);
            lua_pushlstring(L, string, length);
            lua_concat(L, 2);
        }
    };

    const char* command = luaL_checkstring(L, 1);
    int         mirror  = lua_toboolean(L, 2);

    lua_pushstring(L, "");

    int result = 0;
    if (!Process_Run(L, command, mirror ? Locals::CallbackWithMirror : Locals::Callback, &result))
    {
        lua_pop(L, 1);
        lua_pushnil(L);
    }
    return 1;
}

/** Replacement for os.exit which will just set the exit code and not kill
exit the process. */
int OsExit(lua_State* L)
{
    lua_pushlightuserdata(L, &_buildTag);
    lua_rawget(L, LUA_REGISTRYINDEX);
    Build* build = static_cast<Build*>(lua_touserdata(L, -1));

    if (lua_isboolean(L, 1))
    {
        build->exitCode = (lua_toboolean(L, 1) ? EXIT_SUCCESS : EXIT_FAILURE);
    }
    else
    {
        build->exitCode = luaL_optint(L, 1, EXIT_SUCCESS);
    }
    lua_pop(L, 1);
    return lua_yield(L, 0);
}

/** Sets up a function that will be called on exit. */
int OsAtExit(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TFUNCTION);

    lua_pushlightuserdata(L, &_atexitTag);
    lua_rawget(L, LUA_REGISTRYINDEX);

    lua_pushvalue(L, 1);
    lua_pushvalue(L, 1);
    lua_rawset(L, -3);
    
    lua_pop(L, 1);

    return 0;
}

void OsCallAtExit(lua_State* L, int exitCode)
{

    lua_pushlightuserdata(L, &_atexitTag);
    lua_rawget(L, LUA_REGISTRYINDEX);

    int t = lua_gettop(L);

    lua_pushnil(L);
    while (lua_next(L, t) != 0)
    {
        lua_pushboolean(L, exitCode == EXIT_SUCCESS);
        lua_pcall(L, 1, 0, 0);
    }

}

/** Binds auxiliary functions/variables. */
void BindLuaLibrary(lua_State* L)
{

    static const luaL_Reg oslib[] =
        {
            {"execute", OsExecute },
            {"chdir",   OsChdir },
            {"capture", OsCapture },
            {"exit",    OsExit },
            {"atexit",  OsAtExit },
            { NULL,     NULL }
        };
    
    lua_getglobal(L, "os");
    luaL_setfuncs(L, oslib, 0);

    // Setup the table we'll use for atexit.
    lua_pushlightuserdata(L, &_atexitTag);
    lua_newtable(L);
    lua_rawset(L, LUA_REGISTRYINDEX);
 
}

int RunScript(Database& db, const char* projectName, const char* command, int projectId, bool log)
{

    // Save off the working directory, since the script may change it.
    char* workingDir = getcwd(NULL, 0);
    
    lua_State* L = luaL_newstate();

    // Store information about the build we're running in the Lua state so
    // that it can be accessed from the print callbacks.

    Build build;
    build.db        = &db;
    build.projectId = projectId;
    build.exitCode  = EXIT_SUCCESS;
    build.log       = log;

    lua_pushlightuserdata(L, &_buildTag);
    lua_pushlightuserdata(L, &build);
    lua_rawset(L, LUA_REGISTRYINDEX);

    luaL_openlibs(L);
    BindLuaLibrary(L);

    // Store the last run time as a global.

    char query[256];
    snprintf(query, sizeof(query), "SELECT UNIX_TIMESTAMP(time) FROM project_builds WHERE projectId='%d'", projectId);
    db.Query(query);

    time_t lastTimeRun = 0;

    if (db.GetNumRows() > 0)
    {
        char** row = db.GetRow();
        lastTimeRun = atoi(row[0]);
    }

    lua_pushnumber(L, static_cast<lua_Number>(lastTimeRun));
    lua_setglobal(L, "_LAST_TIME_RUN");

    if (projectName != NULL)
    {
        lua_pushstring(L, projectName);
        lua_setglobal(L, "_PROJECT_NAME");
    }

    lua_pushstring(L, "");
    lua_setglobal(L, "_LOG");

    int exitCode = EXIT_FAILURE;

    // We use a new thread so that we can exit from it if os.exit is called.

    lua_State* T = lua_newthread(L);

    if (luaL_loadstring(T, command) == 0)
    {
        int result = lua_resume(T, L, 0);
        if (result != LUA_OK && result != LUA_YIELD)
        {
            if (!log)
            {
                ClearLog(&build);
            }
            const char* error = lua_tostring(T, -1);
            AppendToLog(&build, error);
            lua_pop(T, 1);
        }
        else
        {
            exitCode = build.exitCode;
        }
    }
    else
    {
        if (!log)
        {
            ClearLog(&build);
        }
        const char* error = lua_tostring(T, -1);
        AppendToLog(&build, error);
        lua_pop(T, 1);
    }
    
    OsCallAtExit(L, exitCode);

    lua_close(L);
    L = NULL;

    FlushBuffer(&build);
    
    if (workingDir != NULL)
    {
        chdir(workingDir);
        free(workingDir);
        workingDir = NULL;
    }

    return exitCode;

}

/** Initiates building of the project with the specified id. */
void BuildProject(Database& db, int projectId)
{

    // Get the information about the build.

    char query[256];
    snprintf(query, sizeof(query), "SELECT name, command FROM projects WHERE id='%d'", projectId);
    db.Query(query);

    if (db.GetNumRows() > 0)
    {

        int colName    = db.GetColumn("name");
        int colCommand = db.GetColumn("command");

        char** row = db.GetRow();
        
        char* name    = strdup(row[colName]);
        char* command = strdup(row[colCommand]);

        printf("> Project '%s' started\n", name);

        // Change the status to building.
        snprintf(query, sizeof(query), "UPDATE project_builds SET state='building', time=NOW(), log='' WHERE projectId='%d'", projectId);
        db.Query(query);

        int exitCode = RunScript(db, name, command, projectId, true);
        SetProjectStatus(db, projectId, exitCode);

        printf("> Project %s\n", (exitCode == EXIT_SUCCESS) ? "succeeded" : "failed");

        free(command);
        command = NULL;

        free(name);
        name = NULL;
    
    }

}

void BuildRequestedProjects(Database& db)
{

    db.Query("SELECT projectId, state FROM project_builds");

    if (db.GetNumRows() > 0)
    {

        int colProjectId = db.GetColumn("projectId");
        int colState     = db.GetColumn("state");

        if (colProjectId == -1 || colState == -1)
        {
            fprintf(stderr, "Bad database format\n");
            return;
        }

        for (int i = 0; i < db.GetNumRows(); ++i)
        {
            char** row = db.GetRow();
            if (strcmp(row[colState], "pending") == 0)
            {
                int projectId = atoi(row[colProjectId]);
                BuildProject(db, projectId);
            }
        }

    }

}

void BuildTriggeredProjects(Database& db)
{

    bool done  = false;
    int lastId = -1;

    while (!done)
    {

        char query[256];
        snprintf(query, sizeof(query), "SELECT id, test, name FROM projects WHERE test!='' AND id>'%d' AND enabled='1' LIMIT 1", lastId);
        db.Query(query);

        if (db.GetNumRows() == 0)
        {
            done = true;
        }
        else
        {

            int colId   = db.GetColumn("id");
            int colTest = db.GetColumn("test");
            int colName = db.GetColumn("name");

            if (colName == -1 || colId == -1 || colTest == -1)
            {
                fprintf(stderr, "Bad database format\n");
                return;
            }

            char** row = db.GetRow();

            int projectId = atoi(row[colId]);
            char* command = strdup(row[colTest]);
            char* name    = strdup(row[colName]);

            int exitCode = RunScript(db, name, command, projectId, false);

            free(command);
            command = NULL;

            free(name);
            name = NULL;

            if (exitCode == EXIT_SUCCESS)
            {
                BuildProject(db, projectId);
            }

            lastId = projectId;

        }

    }

}

void Run(Database& db)
{
    const int sleepInverval = 1000 * 5;
    bool done = false;
    while (!done)
    {
        Thread_Sleep(sleepInverval);
        BuildRequestedProjects(db);
        BuildTriggeredProjects(db);
    }
}

int main(int, char*[])
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
        
        char* field = strtok(buffer, "= ");
        strtok(NULL, " '");
        char* value = strtok(NULL,  "'\r\n");

        if (field != NULL && value != NULL)
        {
            if (strcmp(field, "DB_HOST") == 0) strcpy(dbHost, value);
            else if (strcmp(field, "DB_USERNAME") == 0) strcpy(dbUserName, value);
            else if (strcmp(field, "DB_PASSWORD") == 0) strcpy(dbPassword, value);
            else
            {
                // Store the option as an environment variable which scripts can
                // access.
                char temp[256];
                snprintf(temp, sizeof(temp), "%s=%s", field, value);
                putenv(temp);
            }
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

    // If any projects were running when we exited, update the DB to indicate they
    // are no longer.
    db.Query("UPDATE project_builds SET state='failed' WHERE state='building'");

    Run(db);
    return 0;
}