-- On Windows this specifies the directory where the MySQL C Connector library is installed.
local mysqldir = "C:/Program Files (x86)/MySQL/MySQL Connector C 6.0.2"

if os.is("windows") and not os.isdir(mysqldir) then
	print("Building Carson requires the MySQL C Connector library to be installed.")
	print("The library can be downloaded from: http://www.mysql.com/downloads/connector/c/")
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
		libdirs { mysqldir .. "/lib/opt" }
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