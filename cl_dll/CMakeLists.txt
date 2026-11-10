#
# Copyright (c) 2016 Alibek Omarov
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
project (CLDLL)

set (CLDLL_LIBRARY client)

add_definitions(-DCLIENT_DLL -DCLIENT_WEAPONS -DHL_DLL -DGATESEQ)

if(NOT MSVC)
	add_compile_options(-fvisibility=hidden) # GCC/Clang flag
	add_definitions(-D_LINUX -DLINUX) # It seems enough for all non-Win32 systems
	if(NOT MINGW)
		add_definitions(-D_snprintf=snprintf -D_vsnprintf=vsnprintf)
	endif()
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

if (GOLDSOURCE_SUPPORT)
	add_definitions(-DGOLDSOURCE_SUPPORT)
endif()

if (USE_STATIC_FREEVGUI)
	add_definitions(-DINTERNAL_VGUI_SUPPORT)
endif()

set (CLDLL_SOURCES
	GameStudioModelRenderer.cpp
	Kingpin.cpp
	StudioModelRenderer.cpp
	ammo.cpp
	ammo_secondary.cpp
	ammohistory.cpp
	battery.cpp
	cdll_int.cpp
	com_weapons.cpp
	death.cpp
	demo.cpp
	entity.cpp
	ev_common.cpp
	ev_hldm.cpp
	events.cpp
	flashlight.cpp
	geiger.cpp
	health.cpp
	hl/hl_baseentity.cpp
	hl/hl_events.cpp
	hl/hl_objects.cpp
	hl/hl_weapons.cpp
	hud.cpp
	hud_msg.cpp
	hud_redraw.cpp
	hud_spectator.cpp
	hud_update.cpp
	in_camera.cpp
	input.cpp
	input_xash.cpp
	interpolation.cpp
	menu.cpp
	message.cpp
	mp3.cpp
	particleman/CBaseParticle.cpp
	particleman/CFrustum.cpp
	particleman/CMiniMem.cpp
	particleman/IParticleMan_Active.cpp
	saytext.cpp
	status_icons.cpp
	statusbar.cpp
	studio_util.cpp
	text_message.cpp
	train.cpp
	tri.cpp
	unicode.cpp
	util.cpp
	vgui_ClassMenu.cpp
	vgui_CustomObjects.cpp
	vgui_MOTDWindow.cpp
	vgui_SchemeManager.cpp
	vgui_ScorePanel.cpp
	vgui_SpectatorPanel.cpp
	vgui_TeamFortressViewport.cpp
	vgui_int.cpp
	vgui_screenmsg.cpp
	vgui_subtitles.cpp
	vgui_teammenu.cpp
	view.cpp
	voice_status.cpp
	zoom.cpp
	../common/mathlib.cpp
	../common/parsemsg.cpp
	../dlls/crossbow.cpp
	../dlls/crowbar.cpp
	../dlls/egon.cpp
	../dlls/gauss.cpp
	../dlls/glock.cpp
	../dlls/handgrenade.cpp
	../dlls/hornetgun.cpp
	../dlls/m16.cpp
	../dlls/mp5.cpp
	../dlls/python.cpp
	../dlls/revolver.cpp
	../dlls/rpg.cpp
	../dlls/satchel.cpp
	../dlls/shotgun.cpp
	../dlls/squeakgrenade.cpp
	../dlls/tripmine.cpp
	../dlls/weapons_shared.cpp
	../dlls/hf_crash.cpp
	../game_shared/filesystem_utils.cpp
	../game_shared/vgui_checkbutton2.cpp
	../game_shared/vgui_grid.cpp
	../game_shared/vgui_helpers.cpp
	../game_shared/vgui_listbox.cpp
	../game_shared/vgui_loadtga.cpp
	../game_shared/vgui_scrollbar2.cpp
	../game_shared/vgui_slider2.cpp
	../game_shared/voice_banmgr.cpp
	../pm_shared/pm_debug.cpp
	../pm_shared/pm_math.cpp
	../pm_shared/pm_shared.cpp
	../public/interface.cpp
)

if (HAVE_STRLCPY)
	add_definitions(-DHAVE_STRLCPY)
else()
	list(APPEND CLDLL_SOURCES
		../external/openbsd/strlcpy.c)
endif()

if (HAVE_STRLCAT)
	add_definitions(-DHAVE_STRLCAT)
else()
	list(APPEND CLDLL_SOURCES
		../external/openbsd/strlcat.c)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include_directories (. hl/ particleman ../dlls ../dlls/wpn_shared ../common ../engine ../pm_shared ../game_shared ../public ../external)

if (GOLDSOURCE_SUPPORT)
	include_directories (../external/)
endif()


include_directories(../utils/vgui/include)

add_library (${CLDLL_LIBRARY} SHARED ${CLDLL_SOURCES})
target_link_libraries(${CLDLL_LIBRARY} vcs_info)
if(ANDROID)
	target_link_libraries(${CLDLL_LIBRARY} log)
endif()

if (GOLDSOURCE_SUPPORT)
	target_link_libraries( ${CLDLL_LIBRARY} ${CMAKE_DL_LIBS} )
endif()

target_link_libraries(${CLDLL_LIBRARY} vgui)

if(WIN32)
	target_link_libraries( ${CLDLL_LIBRARY} user32.lib )
	if (GOLDSOURCE_SUPPORT)
		target_link_libraries( ${CLDLL_LIBRARY} winmm.lib )
	endif()
endif()

set_target_properties (${CLDLL_LIBRARY} PROPERTIES
	POSITION_INDEPENDENT_CODE 1)

set_target_postfix(${CLDLL_LIBRARY})

if(HAVE_LTO)
	set_property(TARGET ${CLDLL_LIBRARY} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

if(XASH_PS4)
	# signed module and names of its functions, see cmake/PS4.cmake
	ps4_add_module(${CLDLL_LIBRARY} "${GAMEDIR}/${CLIENT_INSTALL_DIR}/")
else()
	install(TARGETS ${CLDLL_LIBRARY}
		RUNTIME DESTINATION "${GAMEDIR}/${CLIENT_INSTALL_DIR}/"
		LIBRARY DESTINATION "${GAMEDIR}/${CLIENT_INSTALL_DIR}/"
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
	add_custom_command(TARGET ${CLDLL_LIBRARY}
			POST_BUILD DEPENDS ${CLDLL_LIBRARY}
			COMMAND ${CMAKE_STRIP} ${STRIP_FLAGS} $<TARGET_FILE:${CLDLL_LIBRARY}>)
endif()
