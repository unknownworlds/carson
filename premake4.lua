-- On Windows this specifies the directory where the MySQL C Connector library is installed.
local mysqlversions = { "6.1", "6.0.2" }
local mysqllibdir = { ["6.1"] = "/lib", ["6.0.2"] = "/lib/opt" }
local mysqldir = "C:/Program Files (x86)/MySQL/MySQL Connector C "

local foundLibrary = false
local libVersion = nil
for v = 1, #mysqlversions do

    local lib = mysqldir .. mysqlversions[v]
    if os.is("windows") and os.isdir(lib) then
    
        libVersion = mysqlversions[v]
        foundLibrary = true
        mysqldir = lib
        break

    end

end

if not foundLibrary then

    print("Building Carson requires the MySQL C Connector library to be installed.")
    print("The library can be downloaded from: http://www.mysql.com/downloads/connector/c/")
    return

end

solution "carson"
    configurations { "Debug", "Release" }
    location "build"
    debugdir  "."
    vpaths { 
        ["Header Files"] = "**.h",
        ["Source Files"] = { "**.cpp", "**.c", "**.inl" },
    }
    flags { "StaticRuntime" } -- for easier deployment
    
    if os.is("windows") then
        defines { "_CRT_SECURE_NO_WARNINGS", "_CRT_SECURE_NO_DEPRECATE", "WIN32", "_WIN32_WINNT" }
        includedirs { mysqldir .. "/include" }
        libdirs { mysqldir .. mysqllibdir[libVersion] }
    end
    
project "carson"
    kind "ConsoleApp"
    location "build"
    language "C++"
    files { "src/**.h", "src/**.c", "src/**.cpp" }
    links { "libmysql" }
    configuration "Debug"
        defines { "DEBUG" }
        flags { "Symbols" }
        targetdir "bin/debug"
    configuration "Release"
        defines { "NDEBUG" }
        flags { "Symbols", "Optimize" }     
        targetdir "bin/release"