#
# Note Prototype CMake build configuration
#
#               Copyright Acer Cloud Technologies 2013
#

cmake_minimum_required (VERSION 2.6)

project (notes)

file (TO_CMAKE_PATH "../../../../../../" SOURCEROOT)
file (TO_CMAKE_PATH "$ENV{BUILDROOT}/release/msvc/gvm_core/daemons/ccd/sdk/" CCD_SDK_DIR)
file (TO_CMAKE_PATH "$ENV{BUILDROOT}/build_msvc/PersonalCloudAll" SLN_BUILD_DIR)

message ("Using SOURCEROOT: " ${SOURCEROOT})
message ("Using BUILDROOT: " $ENV{BUILDROOT})
message ("Using SDK directory: " ${CCD_SDK_DIR})
message ("Using Solution Build directory: " ${SLN_BUILD_DIR})

add_executable (notes "main.cpp" "db.cpp" "sync.cpp" "file.cpp" "event.cpp" "task.cpp" "thread.cpp" "command.cpp"
    "db-tasks.cpp" "file-tasks.cpp" "sync-tasks.cpp")

if (WIN32)
  target_link_libraries (notes ws2_32 iphlpapi shlwapi)
endif ()

include_directories (
    "${SOURCEROOT}/gvm_core/daemons/ccd/include"
    "${SOURCEROOT}/gvm_core/daemons/ccd/types/src"
    "${SOURCEROOT}/gvm_core/vplex/include"
    "${SOURCEROOT}/gvm_core/internal/error_codes/include"
    "${SOURCEROOT}/gvm_core/internal/db_utils/include"
    "${SOURCEROOT}/gvm_core/external/sqlite3"
    "${CCD_SDK_DIR}/include")

if (WIN32)
    include_directories ("${SOURCEROOT}/gvm_core/vplex/platform_win32/include")
endif ()

# Uncomment if you wish to test with CCD and dxshell: begin
#find_file (CCD_DEBUG_EXE "CCD.exe" PATHS ${SLN_BUILD_DIR} PATH_SUFFIXES "Debug/CCD/Win32")
#find_file (CCD_RELEASE_EXE "CCD.exe" PATHS ${SLN_BUILD_DIR} PATH_SUFFIXES "Release/CCD/Win32")
#find_file (DXSHELL_DEBUG_EXE "dxshell.exe" PATHS ${SLN_BUILD_DIR} PATH_SUFFIXES "Debug/dxshell/Win32")
#find_file (DXSHELL_RELEASE_EXE "dxshell.exe" PATHS ${SLN_BUILD_DIR} PATH_SUFFIXES "Release/dxshell/Win32")
#find_file (CCD_CONF_TMPL "ccd.conf.tmpl" PATHS "${SOURCEROOT}/gvm_core/conf")
# Uncomment if you wish to test with CCD and dxshell: end

find_library (SQLITE_DEBUG_LIB "sqlite3" PATHS "${SLN_BUILD_DIR}" PATH_SUFFIXES "Debug/sqlite3/Win32")
find_library (SQLITE_RELEASE_LIB "sqlite3" PATHS "${SLN_BUILD_DIR}" PATH_SUFFIXES "Release/sqlite3/Win32")

find_library (CCDI_DEBUG_LIB "CCDIClientAndDeps" PATHS "${SLN_BUILD_DIR}" PATH_SUFFIXES "Debug/CCDIClientAndDeps/Win32")
find_library (CCDI_RELEASE_LIB "CCDIClientAndDeps" PATHS "${SLN_BUILD_DIR}" PATH_SUFFIXES "Release/CCDIClientAndDeps/Win32")

find_library (INTERNAL_DEBUG_LIB "internal_libs" PATHS "${SLN_BUILD_DIR}" PATH_SUFFIXES "Debug/internal_libs/Win32")
find_library (INTERNAL_RELEASE_LIB "internal_libs" PATHS "${SLN_BUILD_DIR}" PATH_SUFFIXES "Release/internal_libs/Win32")

target_link_libraries (notes debug ${CCDI_DEBUG_LIB} debug ${INTERNAL_DEBUG_LIB} debug ${SQLITE_DEBUG_LIB})
target_link_libraries (notes optimized ${CCDI_RELEASE_LIB} optimized ${INTERNAL_RELEASE_LIB} optimized ${SQLITE_RELEASE_LIB})

install (TARGETS notes DESTINATION "bin" CONFIGURATIONS Debug Release)

# Uncomment if you wish to test with CCD and dxshell: begin
#install (PROGRAMS ${CCD_DEBUG_EXE} DESTINATION "bin" CONFIGURATIONS Debug)
#install (PROGRAMS ${CCD_RELEASE_EXE} DESTINATION "bin" CONFIGURATIONS Release)
#install (PROGRAMS ${DXSHELL_DEBUG_EXE} DESTINATION "bin" CONFIGURATIONS Debug)
#install (PROGRAMS ${DXSHELL_RELEASE_EXE} DESTINATION "bin" CONFIGURATIONS Release)
#install (FILES ${CCD_CONF_TMPL} DESTINATION "bin")
# Uncomment if you wish to test with CCD and dxshell: end
