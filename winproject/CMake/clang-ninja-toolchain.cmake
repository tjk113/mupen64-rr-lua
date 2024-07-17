set(CMAKE_SYSTEM_NAME Windows)

# Search for clang compiler
find_program(CLANG_EXECUTABLE NAMES clang)

# Search for clang++ compiler
find_program(CLANGXX_EXECUTABLE NAMES clang++)

if(CLANG_EXECUTABLE AND CLANGXX_EXECUTABLE)
    # Set found compilers
    set(CMAKE_C_COMPILER "${CLANG_EXECUTABLE}")
    set(CMAKE_CXX_COMPILER "${CLANGXX_EXECUTABLE}")

    message(STATUS "Found Clang compiler: ${CLANG_EXECUTABLE}")
    message(STATUS "Found Clang++ compiler: ${CLANGXX_EXECUTABLE}")
else()
    message(FATAL_ERROR "Clang compilers not found.")
endif()

# Specify compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2b -m32")

# Specify build type
set(CMAKE_BUILD_TYPE Release)

# Specify generator
set(CMAKE_GENERATOR "Ninja")
