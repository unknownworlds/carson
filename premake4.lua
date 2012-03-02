solution "carson"
    configurations { "Debug", "Release" }
    location "build"
	debugdir  "."
    defines { "_CRT_SECURE_NO_WARNINGS", "_CRT_SECURE_NO_DEPRECATE", "WIN32", "_WIN32_WINNT" }
    vpaths { 
        ["Header Files"] = "**.h",
        ["Source Files"] = { "**.cpp", "**.c", "**.inl" },
    }

project "carson"
    kind "ConsoleApp"
    location "build"
    language "C++"
    files { "src/**.h", "src/**.c", "src/**.cpp" , "src/**.inl" }
    configuration "Debug"
        defines { "DEBUG" }
        flags { "Symbols" }
        targetdir "bin/debug"
    configuration "Release"
        defines { "NDEBUG" }
        flags { "Optimize" }     
        targetdir "bin/release"