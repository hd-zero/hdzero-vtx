################################################################################
#  Requires SDCC (Small Device C Compiler)                                     #
################################################################################

# Project: Identity
cmake_minimum_required(VERSION 3.5)
project(hdzero-vtx C)

# Project: Settings
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

# Configurable Parameters
if(CMAKE_BUILD_TYPE STREQUAL "")
  set(CMAKE_BUILD_TYPE "Release")
endif()

# Compilation Flags
set(COMMON_FLAGS "-c --model-large")
set(COMMON_DEBUG_FLAGS ${COMMON_FLAGS})
set(COMMON_RELEASE_FLAGS ${COMMON_FLAGS})
set(CMAKE_C_FLAGS_DEBUG ${COMMON_DEBUG_FLAGS})
set(CMAKE_C_FLAGS_RELEASE ${COMMON_RELEASE_FLAGS})

# Source Files
file(GLOB_RECURSE SRCS CONFIGURE_DEPENDS src/*.c*)

# Platform Attributes
set(BINARY_SUFFIX ".ihx")
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(INCLUDE_PATHS "")
  set(LIBRARY_PATHS
    /usr/local/cuda/lib64
    /usr/lib/x86_64-linux-gnu
    /lib/x86_64-linux-gnu
  )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(INCLUDE_PATHS "/opt/local/include")
  set(LIBRARY_PATHS
    /opt/local/lib
  )
endif()

# Include Paths
include_directories(
  ${INCLUDE_PATHS}
)

# Library Paths
link_directories(
  ${LIBRARY_PATHS}
)

# Target
add_executable(${PROJECT_NAME}
  ${SRCS}
)

# Target: Attributes
set_target_properties(
  ${PROJECT_NAME}
    PROPERTIES 
      SUFFIX "${BINARY_SUFFIX}"
)

