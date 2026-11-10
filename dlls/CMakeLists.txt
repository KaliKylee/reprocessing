#
# Copyright (c) 2015 Pavlo Lavrenenko
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

cmake_minimum_required(VERSION 3.9)
project (SVDLL)

set (SVDLL_LIBRARY server)

if(NOT MSVC)
	add_compile_options(-Wno-invalid-offsetof) # GCC/Clang flag
	add_compile_options(-fvisibility=hidden) # GCC/Clang flag
	add_definitions(-D_LINUX -DLINUX) # It seems enough for all non-Win32 systems
	add_definitions(-DQUIVER -DVOXEL -DQUAKE2 -DVALVE_DLL -DCLIENT_WEAPONS -DGATESEQ)
	add_definitions(-D_snprintf=snprintf -D_vsnprintf=vsnprintf)
	if(CMAKE_BUILD_TYPE MATCHES "Release"
		OR (CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo"
		AND CMAKE_SYSTEM_NAME STREQUAL "Android"))
		add_compile_options(-fno-unwind-tables -fno-asynchronous-unwind-tables) # GCC/Clang flag
		add_compile_options(-fomit-frame-pointer) # GCC/Clang flag
		add_compile_options(-ftree-vectorize) # GCC/Clang flag
		add_compile_options(-funsafe-math-optimizations) # GCC/Clang flag
	endif()
else()
	add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE)
endif()

set (SVDLL_SOURCES
	../common/mathlib.cpp
	CGrappleTip.cpp
	UserMessages.cpp
	aflock.cpp
	agrunt.cpp
	airtank.cpp
	animating.cpp
	animation.cpp
	apache.cpp
	archer.cpp
	barnacle.cpp
	barney.cpp
	bigmomma.cpp
	bloater.cpp
	bmodels.cpp
	bullsquid.cpp
	buttons.cpp
	cbase.cpp
	client.cpp
	combat.cpp
	controller.cpp
	crossbow.cpp
	crowbar.cpp
	defaultai.cpp
	doors.cpp
	effects.cpp
	egon.cpp
	explode.cpp
	flyingmonster.cpp
	func_break.cpp
	func_tank.cpp
	game.cpp
	hf_crash.cpp
	gamerules.cpp
	gargantua.cpp
	gauss.cpp
	genericmonster.cpp
	ggrenade.cpp
	glock.cpp
	gman.cpp
	h_ai.cpp
	h_battery.cpp
	h_cine.cpp
	h_cycler.cpp
	h_export.cpp
	handgrenade.cpp
	hassassin.cpp
	hassault.cpp
	headcrab.cpp
	healthkit.cpp
	hgrunt.cpp
	hornet.cpp
	hornetgun.cpp
	houndeye.cpp
	ichthyosaur.cpp
	islave.cpp
	items.cpp
	kingpin.cpp
	leech.cpp
	lights.cpp
	m16.cpp
	male_assassin.cpp
	maprules.cpp
	mon_gonome.cpp
	monstermaker.cpp
	monsters.cpp
	monsterstate.cpp
	mortar.cpp
	mp5.cpp
	multiplay_gamerules.cpp
	nam_charley.cpp
	nihilanth.cpp
	nodes.cpp
	observer.cpp
	osprey.cpp
	otis.cpp
	panthereye.cpp
	pathcorner.cpp
	pitdrone.cpp
	plane.cpp
	plats.cpp
	player.cpp
	psay.cpp
	python.cpp
	rat.cpp
	revolver.cpp
	roach.cpp
	rpg.cpp
	satchel.cpp
	schedule.cpp
	scientist.cpp
	scripted.cpp
	shocktrooper.cpp
	shotgun.cpp
	singleplay_gamerules.cpp
	skill.cpp
	sound.cpp
	soundent.cpp
	spectator.cpp
	squadmonster.cpp
	squeakgrenade.cpp
	stukabat.cpp
	subs.cpp
	talkmonster.cpp
	teamplay_gamerules.cpp
	tempmonster.cpp
	tentacle.cpp
	triggers.cpp
	tripmine.cpp
	turret.cpp
	util.cpp
	voltigore.cpp
	weapons.cpp
	weapons_shared.cpp
	world.cpp
	xen.cpp
	zombie.cpp
	../game_shared/filesystem_utils.cpp
	../game_shared/voice_gamemgr.cpp
	../pm_shared/pm_debug.cpp
	../pm_shared/pm_math.cpp
	../pm_shared/pm_shared.cpp
	../public/interface.cpp
)

if (HAVE_STRLCPY)
	add_definitions(-DHAVE_STRLCPY)
else()
	list(APPEND SVDLL_SOURCES
		../external/openbsd/strlcpy.c)
endif()

if (HAVE_STRLCAT)
	add_definitions(-DHAVE_STRLCAT)
else()
	list(APPEND SVDLL_SOURCES
		../external/openbsd/strlcat.c)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include_directories (. wpn_shared ../common ../engine ../pm_shared ../game_shared ../public ../external)


add_library (${SVDLL_LIBRARY} SHARED ${SVDLL_SOURCES})
target_link_libraries(${SVDLL_LIBRARY} vcs_info)
if(ANDROID)
	target_link_libraries(${SVDLL_LIBRARY} log)
endif()

set_target_properties (${SVDLL_LIBRARY} PROPERTIES POSITION_INDEPENDENT_CODE 1)

if(APPLE AND NOT "${SERVER_LIBRARY_NAME_OSX}" STREQUAL "")
	# on Apple platforms we always use gamedll_osx liblist/gameinfo key, if it's set
	set(SVDLL_NAME "${SERVER_LIBRARY_NAME_OSX}")
elseif(NOT WIN32 AND NOT "${SERVER_LIBRARY_NAME_LINUX}" STREQUAL "")
	# on non-Windows platforms we use gamedll_linux liblist/gameinfo key, if it's set
	set(SVDLL_NAME "${SERVER_LIBRARY_NAME_LINUX}")
else()
	# on Windows or if no gamedll_osx or gamedll_linux is set, use default gamedll key
	set(SVDLL_NAME "${SERVER_LIBRARY_NAME}")
endif()

set_target_postfix_with_name(${SVDLL_LIBRARY} ${SVDLL_NAME})

if(HAVE_LTO)
	set_property(TARGET ${SVDLL_LIBRARY} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

if(XASH_PS4)
	# signed module and names of its functions, see cmake/PS4.cmake
	ps4_add_module(${SVDLL_LIBRARY} "${GAMEDIR}/${SERVER_INSTALL_DIR}/")
else()
	install(TARGETS ${SVDLL_LIBRARY}
		RUNTIME DESTINATION "${GAMEDIR}/${SERVER_INSTALL_DIR}/"
		LIBRARY DESTINATION "${GAMEDIR}/${SERVER_INSTALL_DIR}/"
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
			GROUP_READ GROUP_EXECUTE
			WORLD_READ WORLD_EXECUTE)
endif()

if(CMAKE_BUILD_TYPE MATCHES "Release" AND NOT WIN32 AND NOT XASH_PS4)
	if(APPLE)
		# Apple strip treats -s as a symbol list file, -x keeps exported symbols
		set(STRIP_FLAGS -x)
	else()
		set(STRIP_FLAGS -s)
	endif()
	add_custom_command(TARGET ${SVDLL_LIBRARY}
			POST_BUILD DEPENDS ${SVDLL_LIBRARY}
			COMMAND ${CMAKE_STRIP} ${STRIP_FLAGS} $<TARGET_FILE:${SVDLL_LIBRARY}>)
endif()
