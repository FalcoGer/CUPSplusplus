cmake_minimum_required(VERSION 3.10.0)
project(hardcopy VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if (DEFINED CLANG_GCC_TOOLCHAIN_PATH)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --gcc-toolchain=${CLANG_GCC_TOOLCHAIN_PATH}")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --gcc-toolchain=${CLANG_GCC_TOOLCHAIN_PATH}")
    endif()
    if(DEFINED CLANG_CXX_COMPILER_FLAGS)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CLANG_CXX_COMPILER_FLAGS}")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	if(DEFINED GCC_CXX_COMPILER_FLAGS)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_CXX_COMPILER_FLAGS}")
    endif()
endif()


find_package(PkgConfig REQUIRED)
pkg_check_modules(CUPS cups REQUIRED)

set(cups ${CUPS_LIBRARIES})
message(NOTICE "Found CUPS: ${cups}")

add_library(CupsPlusPlus Cups.cpp)

add_executable(hardcopy main.cpp)

target_link_libraries(hardcopy PRIVATE ${cups} CupsPlusPlus)

