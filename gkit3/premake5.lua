dofile "gkit.lua"

 -- description des projets		 
projects = {
	"tp1",
	"tp2",
}

for i, name in ipairs(projects) do
    project(name)
        language "C++"
        kind "ConsoleApp"
        targetdir "bin"
       links { "gkit3" }
        includedirs { ".", "src" }
        
        files { "projets/" .. name .. ".cpp" }
--~ 	buildoptions { "-Weffc++" }
end
project("camera")
  language "C++"
  kind "ConsoleApp"
  targetdir "bin"
  buildoptions ( "-std=c++17" )
  project("camera")
  language "C++"
  kind "ConsoleApp"
  targetdir "bin"
  buildoptions ( "-std=c++17" )
  files ( gkit_files )
  files { "projets/camera-etu/src/*.cpp" }
links { "gkit3" }
    includedirs { ".", "src" }

project("bezier")
  language "C++"
  kind "ConsoleApp"
  targetdir "bin"
  links { "gkit3" }
  buildoptions ( "-std=c++17" )
  includedirs { ".", "src" }
  files { "projets/bezier-etu/src/*.cpp" }
