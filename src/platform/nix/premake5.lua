workspace "OpenLara"
   configurations { "Debug", "Release" }

project "OpenLara"
   kind "ConsoleApp"
   language "C++"
   includedirs { "../../" }
   exceptionhandling "Off"
   rtti "Off"

   files { "main.cpp", "../../libs/stb_vorbis/stb_vorbis.c", "../../libs/minimp3/minimp3.cpp", "../../libs/tinf/tinflate.c" }

   filter { "system:Linux", "toolset:gcc or clang" }
      links { "X11", "GL", "m", "pthread", "pulse-simple", "pulse" }
	    defines { "POSIX_THREADS", "POSIX_READER_WRITER_LOCKS" }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "Size"
