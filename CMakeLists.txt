################################################################################
#  Requires SDCC (Small Device C Compiler)                                     #
################################################################################

# Project: Identity
cmake_minimum_required(VERSION 3.5)
project(hdzero-vtx LANGUAGES C)

# Project: Settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/../bin")

# Configurable Parameters
if(CMAKE_BUILD_TYPE STREQUAL "")
  set(CMAKE_BUILD_TYPE "Release")
endif()

# Compilation Flags
set(COMMON_DEBUG_FLAGS "")
set(COMMON_RELEASE_FLAGS "")
set(CMAKE_C_FLAGS_DEBUG ${COMMON_DEBUG_FLAGS})
set(CMAKE_C_FLAGS_RELEASE ${COMMON_RELEASE_FLAGS})

# Source Files
file(GLOB_RECURSE SRCS CONFIGURE_DEPENDS src/*.c*)
list(FILTER SRCS EXCLUDE REGEX "_keil.*")

# Platform Attributes
set(BINARY_SUFFIX ".bin")
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
  src/STARTUP.A51
  ${SRCS}
)

# Target: Attributes
set_target_properties(
  ${PROJECT_NAME}
    PROPERTIES 
      SUFFIX "${BINARY_SUFFIX}"
)

# Linker
target_link_libraries(${PROJECT_NAME}
  -Wl,-rpath,lib/${PLATFORM_TYPE}/
)

