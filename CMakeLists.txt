# CMakeLists.txt — Halftone & Torn Edges (After Effects effect plugin)
#
# This builds an After Effects effect plugin against the Adobe After Effects
# SDK. The SDK is NOT redistributable, so you must download it yourself from
# https://developer.adobe.com/after-effects/ and point AESDK_ROOT at it.
#
# Quick start
# -----------
#   cmake -B build -DAESDK_ROOT=/path/to/AfterEffectsSDK/Examples
#   cmake --build build --config Release
#
# Output:
#   Windows : HalftoneTornEdges.aex   (copy to the AE "Plug-ins" folder)
#   macOS   : HalftoneTornEdges.plugin (copy to the AE "Plug-ins" folder)
#
# See docs/build.md for full, step-by-step instructions including how the PiPL
# resource is compiled on each platform.

cmake_minimum_required(VERSION 3.20)
project(HalftoneTornEdges LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED AESDK_ROOT)
	if(DEFINED ENV{AESDK_ROOT})
		set(AESDK_ROOT $ENV{AESDK_ROOT})
	else()
		message(FATAL_ERROR
			"AESDK_ROOT is not set. Pass -DAESDK_ROOT=/path/to/AfterEffectsSDK/Examples")
	endif()
endif()

# Standard AE SDK include locations (Examples/ layout).
set(AE_INCLUDE_DIRS
	"${AESDK_ROOT}/Headers"
	"${AESDK_ROOT}/Headers/SP"
	"${AESDK_ROOT}/Util"
	"${AESDK_ROOT}/Resources")

set(PLUGIN_SOURCES
	src/HalftoneTornEdges.cpp
	src/HalftoneTornEdges_Strings.cpp
	# AE SDK utility sources commonly required by effects:
	"${AESDK_ROOT}/Util/AEGP_SuiteHandler.cpp"
	"${AESDK_ROOT}/Util/MissingSuiteError.cpp")

set(PLUGIN_HEADERS
	src/HalftoneTornEdges.h
	src/HalftoneTornEdges_Process.h
	src/HalftoneTornEdges_Strings.h)

# ---------------------------------------------------------------------------
# macOS: build a .plugin bundle and compile the PiPL with Rez.
# ---------------------------------------------------------------------------
if(APPLE)
	add_library(HalftoneTornEdges MODULE ${PLUGIN_SOURCES} ${PLUGIN_HEADERS})
	target_include_directories(HalftoneTornEdges PRIVATE ${AE_INCLUDE_DIRS} src)
	target_compile_definitions(HalftoneTornEdges PRIVATE AE_OS_MAC=1)

	set_target_properties(HalftoneTornEdges PROPERTIES
		BUNDLE TRUE
		BUNDLE_EXTENSION "plugin"
		OSX_ARCHITECTURES "x86_64;arm64"
		MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/mac/Info.plist"
		OUTPUT_NAME "HalftoneTornEdges")

	# Compile the PiPL into the bundle's resources via Rez.
	find_program(REZ_EXECUTABLE Rez HINTS /usr/bin /Library/Developer/CommandLineTools/usr/bin)
	if(REZ_EXECUTABLE)
		set(PIPL_RSRC "${CMAKE_CURRENT_BINARY_DIR}/HalftoneTornEdges.rsrc")
		add_custom_command(TARGET HalftoneTornEdges POST_BUILD
			COMMAND ${REZ_EXECUTABLE}
				-d AE_OS_MAC=1 -useDF
				-i "${AESDK_ROOT}/Headers"
				-i "${AESDK_ROOT}/Resources"
				-arch x86_64
				"${CMAKE_SOURCE_DIR}/src/HalftoneTornEdgesPiPL.r"
				-o "$<TARGET_BUNDLE_DIR:HalftoneTornEdges>/Contents/Resources/HalftoneTornEdges.rsrc"
			COMMENT "Compiling PiPL with Rez")
	else()
		message(WARNING "Rez not found; PiPL must be compiled manually (see docs/build.md).")
	endif()

# ---------------------------------------------------------------------------
# Windows: build a .aex DLL and link the PiPL resource.
# ---------------------------------------------------------------------------
elseif(WIN32)
	add_library(HalftoneTornEdges MODULE
		${PLUGIN_SOURCES} ${PLUGIN_HEADERS}
		win/HalftoneTornEdges.rc)
	target_include_directories(HalftoneTornEdges PRIVATE ${AE_INCLUDE_DIRS} src)
	target_compile_definitions(HalftoneTornEdges PRIVATE
		AE_OS_WIN=1 MSWindows=1 WIN32=1 _WINDOWS=1)

	set_target_properties(HalftoneTornEdges PROPERTIES
		SUFFIX ".aex"
		OUTPUT_NAME "HalftoneTornEdges"
		LINK_FLAGS "/EXPORT:PluginDataEntryFunction2 /EXPORT:EffectMain")

	# The .rc includes a compiled PiPL (.rrc). Generate it with PiPLtool from
	# the SDK before/at build time. See docs/build.md and win/HalftoneTornEdges.rc.
	find_program(PIPLTOOL PiPLtool HINTS "${AESDK_ROOT}/Resources")
	if(PIPLTOOL)
		set(PIPL_RR "${CMAKE_CURRENT_BINARY_DIR}/HalftoneTornEdges.rr")
		set(PIPL_RRC "${CMAKE_CURRENT_BINARY_DIR}/HalftoneTornEdgesPiPL.rrc")
		# Use a cmake -P helper script to run cl.exe /EP and capture stdout to a
		# file. This avoids the nested cmd /c quoting that breaks under Ninja.
		set(_pipl_script "${CMAKE_CURRENT_BINARY_DIR}/preprocess_pipl.cmake")
		file(WRITE "${_pipl_script}"
			"execute_process(\n"
			"    COMMAND \"${CMAKE_CXX_COMPILER}\" /nologo /EP\n"
			"            /DMSWindows=1 /DAE_OS_WIN=1\n"
			"            /I \"${AESDK_ROOT}/Headers\"\n"
			"            /I \"${AESDK_ROOT}/Resources\"\n"
			"            \"${CMAKE_SOURCE_DIR}/src/HalftoneTornEdgesPiPL.r\"\n"
			"    OUTPUT_FILE \"${PIPL_RR}\"\n"
			"    RESULT_VARIABLE _res)\n"
			"if(_res)\n"
			"    message(FATAL_ERROR \"cl.exe /EP failed: \${_res}\")\n"
			"endif()\n"
		)
		add_custom_command(
			OUTPUT "${PIPL_RRC}"
			COMMAND ${CMAKE_COMMAND} -P "${_pipl_script}"
			COMMAND "${PIPLTOOL}" "${PIPL_RR}" "${PIPL_RRC}"
			DEPENDS "${CMAKE_SOURCE_DIR}/src/HalftoneTornEdgesPiPL.r"
			COMMENT "Compiling PiPL with PiPLtool")
		target_sources(HalftoneTornEdges PRIVATE "${PIPL_RRC}")
		target_include_directories(HalftoneTornEdges PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
	else()
		message(WARNING "PiPLtool not found; generate the PiPL .rrc manually (see docs/build.md).")
	endif()
else()
	message(FATAL_ERROR "After Effects plugins build only on Windows or macOS.")
endif()
