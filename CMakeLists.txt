################################################################################
#  Requires SDCC (Small Device C Compiler)                                     #
################################################################################

# Project: Identity
cmake_minimum_required(VERSION 3.5)
project(hdzero-vtx C)

# Project: Settings
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../bin")

# Configurable Parameters
if(CMAKE_BUILD_TYPE STREQUAL "")
  set(CMAKE_BUILD_TYPE "Release")
endif()

# Compilation Flags
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_C_FLAGS "--model-large --stack-auto")
set(CMAKE_EXE_LINKER_FLAGS "--model-large")

# Source Files
file(GLOB_RECURSE SRCS CONFIGURE_DEPENDS src/*.c*)

# Platform Attributes
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
      SUFFIX ".hex"
)

add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -Iihex -Obinary ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.hex ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.bin
)

