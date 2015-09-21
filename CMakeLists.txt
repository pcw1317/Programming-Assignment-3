cmake_minimum_required(VERSION 3.0)
project(InstantRadiosity)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#------------------------------------------------------------------------------
# Required CPM Setup - See: http://github.com/iauns/cpm
#------------------------------------------------------------------------------
set(CPM_DIR "${CMAKE_CURRENT_BINARY_DIR}/cpm-packages" CACHE TYPE STRING)
find_package(Git)
if(NOT GIT_FOUND)
  message(FATAL_ERROR "CPM requires Git.")
endif()
if ((NOT DEFINED CPM_MODULE_CACHE_DIR) AND (NOT "$ENV{CPM_CACHE_DIR}" STREQUAL ""))
  set(CPM_MODULE_CACHE_DIR "$ENV{CPM_CACHE_DIR}")
endif()
if ((NOT EXISTS ${CPM_DIR}/CPM.cmake) AND (DEFINED CPM_MODULE_CACHE_DIR))
  if (EXISTS "${CPM_MODULE_CACHE_DIR}/github_iauns_cpm")
    message(STATUS "Found cached version of CPM.")
    file(COPY "${CPM_MODULE_CACHE_DIR}/github_iauns_cpm/" DESTINATION ${CPM_DIR})
  endif()
endif()
if (NOT EXISTS ${CPM_DIR}/CPM.cmake)
  message(STATUS "Cloning repo (https://github.com/iauns/cpm)")
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" clone https://github.com/iauns/cpm ${CPM_DIR}
    RESULT_VARIABLE error_code
    OUTPUT_QUIET ERROR_QUIET)
  if(error_code)
    message(FATAL_ERROR "CPM failed to get the hash for HEAD")
  endif()
endif()
include(${CPM_DIR}/CPM.cmake)

#GL
CPM_AddModule("gl_platform"
    GIT_REPOSITORY "https://github.com/iauns/cpm-gl-platform"
    GIT_TAG "1.3.7")

CPM_AddModule("glm"
	GIT_REPOSITORY "https://github.com/iauns/cpm-glm"
    GIT_TAG "1.0.2"
    USE_EXISTING_VER TRUE)

CPM_AddModule("glfw"
	GIT_REPOSITORY "https://github.com/iauns/cpm-glfw"
    GIT_TAG "1.0.2"
    USE_EXISTING_VER TRUE)

CPM_Finish()

#Embree setting
IF(UNIX)
SET(CONTRIB_DIR "${PROJECT_SOURCE_DIR}/contrib/linux")
ELSEIF(APPLE)
SET(CONTRIB_DIR "${PROJECT_SOURCE_DIR}/contrib/macosx")
ELSEIF(WIN32)
SET(CONTRIB_DIR "${PROJECT_SOURCE_DIR}/contrib/windows")
ELSE()
MESSAGE(WARNING "Unsupported OS. Manually copy the required library file from embree.")
SET(CONTRIB_DIR "${PROJECT_SOURCE_DIR}/contrib")
ENDIF()

FIND_LIBRARY(EMBREE_LIBRARY embree PATHS "${CONTRIB_DIR}/lib")
file(GLOB CONTRIB_LIST
  "${CONTRIB_DIR}/*"
)
file(COPY ${CONTRIB_LIST} DESTINATION "${PROJECT_BINARY_DIR}/")

INCLUDE_DIRECTORIES("${CONTRIB_DIR}/include")

#Main codes
INCLUDE_DIRECTORIES("${PROJECT_SOURCE_DIR}/src")
file(GLOB_RECURSE SOURCE_LIST
	"${PROJECT_SOURCE_DIR}/src/*.c"
	"${PROJECT_SOURCE_DIR}/src/*.cpp"
	"${PROJECT_SOURCE_DIR}/src/*.cc"
	"${PROJECT_SOURCE_DIR}/src/*.cxx"
	"${PROJECT_SOURCE_DIR}/src/*.h"
	"${PROJECT_SOURCE_DIR}/src/*.hpp"
	"${PROJECT_SOURCE_DIR}/src/*.hh"
	"${PROJECT_SOURCE_DIR}/src/*.hxx")

add_executable(${PROJECT_NAME} ${SOURCE_LIST})
target_link_libraries(${PROJECT_NAME} ${CPM_LIBRARIES} ${EMBREE_LIBRARY})

