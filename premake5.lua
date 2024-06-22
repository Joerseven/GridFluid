workspace "Fluid"
  configurations { "Debug", "Release" }

project "Fluid"
  kind "ConsoleApp"
  language "C++"
  targetdir "build/%{cfg.buildcfg}"
  libdirs { "external/raylib/lib" }
  links { "raylib" }
  includedirs { "external/raylib/include", "src/" }
  buildoptions { "-std=c++17" }
  linkoptions { "-rpath @executable_path/../../external/raylib/lib"}

  files { "**.h", "**.cpp" }

  filter "configurations:Debug"
    defines { "DEBUG" }
    symbols "On"

  filter "configurations:Release"
    defines { "NDEBUG" }
    optimize "On"



