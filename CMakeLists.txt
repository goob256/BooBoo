project("booboo")
cmake_minimum_required(VERSION 2.8.12)

# Options

option(DEBUG "Debug build" OFF)
option(USE_STATIC_LIBS "Link with static libraries" ON)
option(STEAMWORKS "Build for Steam" OFF)

set(USER_INCLUDE_PATH "" CACHE FILEPATH "Any extra include directories you want")
set(USER_LIBRARY_PATH "" CACHE FILEPATH "Any extra library directories you want")

include_directories("include")
include_directories(${USER_INCLUDE_PATH})
link_directories(${USER_LIBRARY_PATH})

if(USE_STATIC_LIBS)
	set(SHIM4_LIB shim4_static)
	set(TGUI6_LIB tgui6_static)
else()
	set(SHIM4_LIB shim4)
	set(TGUI6_LIB tgui6)
endif()

# Inputs

set(SOURCES
	src/booboo.cpp
	src/main.cpp
)

if(WIN32)
	list(APPEND SOURCES booboo.manifest versioninfo.rc windows_icons.rc)
endif()

# Outputs

add_executable("booboo" ${SOURCES})

# Platform specifics

if(WIN32)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /IGNORE:4075 /IGNORE:4099")
	SET(FLAGS "${FLAGS} /D_CRT_SECURE_NO_WARNINGS /D_USE_MATH_DEFINES /EHsc /MP /W3 /wd4244 /wd4800 /wd4101 /MT")
	SET(FLAGS "${FLAGS} /DNTDDI_VERSION=0x06010000 /D_WIN32_WINNT=0x601")
	SET(FLAGS "${FLAGS} /DUSE_TTF /DUSE_PNG")
	if(DEBUG)
		set(FLAGS "${FLAGS} /DDEBUG=1")
	endif()
	if(USE_STATIC_LIBS)
		set(FLAGS "${FLAGS} /DTGUI6_STATIC /DSHIM4_STATIC")
	endif()
	if(STEAMWORKS)
		set(FLAGS "${FLAGS} /DSTEAMWORKS")
		set(LIBS ${LIBS} steam_api)
	endif()
	#set(FLAGS "${FLAGS} /DUSE_TTF")
	set_target_properties("booboo" PROPERTIES COMPILE_FLAGS "${FLAGS}")
	set_target_properties("booboo" PROPERTIES OUTPUT_NAME "BooBoo")
	#set(LIBS ${LIBS} ${SHIM4_LIB} ${TGUI6_LIB} SDL2 SDL2main libvorbisfile_static libvorbis_static ogg zlibstatic d3d9 winmm dbghelp shcore)
	set(LIBS ${LIBS} ${SHIM4_LIB} ${TGUI6_LIB} SDL2 SDL2main libpng16_static ogg libvorbis_static FLAC freetype zlibstatic d3d9 winmm dbghelp shcore msvcrt)
elseif(APPLE)
	SET(FLAGS "-Wall -Wno-extern-c-compat -Wno-c++11-narrowing -Wno-overloaded-virtual -DTGUI6_STATIC -DSHIM4_STATIC")
	set(FLAGS "${FLAGS} -stdlib=libc++ -std=c++11")
	if(DEBUG)
		set(FLAGS "${FLAGS} -DDEBUG=1 -g")
	endif()
	if(STEAMWORKS)
		set(FLAGS "${FLAGS} -DSTEAMWORKS")
		set(LIBS ${LIBS} steam_api)
	endif()
	if(DEMO)
		set(FLAGS "${FLAGS} -DDEMO")
	endif()
	set_target_properties("booboo" PROPERTIES BUILD_WITH_INSTALL_RPATH on)
	set_target_properties("booboo" PROPERTIES INSTALL_RPATH "@executable_path/")
	set_target_properties("booboo" PROPERTIES COMPILE_FLAGS "${FLAGS}")
	set_target_properties("booboo" PROPERTIES LINK_FLAGS "-framework Cocoa -framework IOKit -framework OpenGL -framework CoreAudio -framework AudioUnit -framework ForceFeedback -framework Carbon")
	set_target_properties("booboo" PROPERTIES OUTPUT_NAME "BooBoo")
	#set(LIBS ${LIBS} ${SHIM4_LIB} ${TGUI6_LIB} SDL2 vorbisfile vorbis ogg z)
	set(LIBS ${LIBS} ${SHIM4_LIB} ${TGUI6_LIB} SDL2 z)
else() # Linux
	SET(FLAGS "-Wall -Wno-comment -Wno-strict-aliasing -std=c++0x")
	SET(FLAGS "${FLAGS} -DUSE_TTF -DUSE_PNG")
	set(LFLAGS "-Wl,-rpath,\$ORIGIN")
	if(DEBUG)
		set(FLAGS "${FLAGS} -DDEBUG=1 -g -fsanitize=address -fsanitize=leak")
		set(LFLAGS "${LFLAGS} -fsanitize=address -fsanitize=leak")
	else()
		set(FLAGS "${FLAGS}")
	endif()
	if(STEAMWORKS)
		set(FLAGS "${FLAGS} -DSTEAMWORKS")
		set(LIBS ${LIBS} steam_api)
	endif()
	if(DEMO)
		set(FLAGS "${FLAGS} -DDEMO")
	endif()
	set_target_properties("booboo" PROPERTIES COMPILE_FLAGS "${FLAGS}")
	set_target_properties("booboo" PROPERTIES LINK_FLAGS "${LFLAGS}")
	set_target_properties("booboo" PROPERTIES OUTPUT_NAME "BooBoo")
	#set(LIBS ${LIBS} shim4_static tgui6_static SDL2 GL Xcursor X11 z vorbisfile vorbis ogg)
	#set(LIBS ${LIBS} shim4_static tgui6_static SDL2 GL Xcursor X11 FLAC ogg z)
	set(LIBS ${LIBS} shim4_static tgui6_static SDL2 GL Xcursor X11 FLAC vorbis ogg freetype png z)
endif()

target_link_libraries("booboo" ${LIBS})
