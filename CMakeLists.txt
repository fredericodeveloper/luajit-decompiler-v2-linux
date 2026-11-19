cmake_minimum_required(VERSION 3.16)
project(LuaJITDecompilerV2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-fpermissive -w)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-z,stack-size=268435456")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(luajit-decompiler-v2
    main.cpp
    ast/ast.cpp
    bytecode/bytecode.cpp
    bytecode/prototype.cpp
    lua/lua.cpp
)

target_include_directories(luajit-decompiler-v2 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
