workspace "gKit3"
    configurations { "debug", "release" }

    filter { "configurations:debug" }
        targetdir "bin/debug"
        defines { "DEBUG" }
        symbols "on"
    
    filter { "configurations:release" }
        targetdir "bin/release"
        symbols "off"
        optimize "full"
        
    filter { "system:linux" }
        cppdialect "c++14"
        buildoptions { "-mtune=native -march=native" }
        buildoptions { "-Wall -Wsign-compare -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable", "-pipe" }
        
    filter { "system:linux" }
        links { "GLEW", "SDL2", "GL" }
    
    filter { "system:linux", "configurations:debug" }
        buildoptions { "-g"}
        linkoptions { "-g"}
    
    filter { "system:linux", "configurations:release" }
        buildoptions { "-flto", "-fopenmp" }
        linkoptions { "-flto=auto", "-fopenmp" }
        
    
    filter { "system:windows", "action:vs*" }
        location "build"
        debugdir "."
        
        system "Windows"
        architecture "x64"
        disablewarnings { "4244", "4305" }
        
       -- multiprocessorcompile "on"
      --  minimalrebuild "on"
        
        defines { "WIN32", "_USE_MATH_DEFINES", "_CRT_SECURE_NO_WARNINGS" }
        defines { "NOMINMAX" } -- allow std::min() and std::max() in vc++ :(((
        
        cppdialect "c++14"
        vectorextensions "avx2"
        floatingpoint "strict"
        exceptionhandling "off"
        
    filter { "system:macosx" }
        buildoptions { "-Wno-deprecated-declarations" }
        
        cppdialect "c++14"
        defines { "GK_MACOS" }
    
    
    project "gkit3"
        language "C++"
        kind "StaticLib"
        targetdir "bin"
        files { "src/*.cpp", "src/*.h" }
        includedirs { ".", "src" }
        
