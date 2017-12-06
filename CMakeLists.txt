# The Project "Evaluation of Implementation Details for Zero" contains the languages C (?) and C++:
PROJECT("Evaluation of Implementation Details for Zero" C CXX)

# The lowest version of CMake supprted by this project is 3.8:
CMAKE_MINIMUM_REQUIRED(VERSION 3.8)

MESSAGE(STATUS)
MESSAGE(STATUS "========================================================")
MESSAGE(STATUS "OS Kernel:         ${CMAKE_SYSTEM}")
MESSAGE(STATUS "CPU Architecture:  ${CMAKE_SYSTEM_PROCESSOR}")
MESSAGE(STATUS "C-Compiler         ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION} at ${CMAKE_C_COMPILER}")
MESSAGE(STATUS "C++-Compiler:      ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} at ${CMAKE_CXX_COMPILER}")
MESSAGE(STATUS "========================================================")
MESSAGE(STATUS)

# No reason to set CMAKE_CONFIGURATION_TYPES if it's not a multiconfig generator
# Also no reason mess with CMAKE_BUILD_TYPE if it's a multiconfig generator.
SET(DEFAULT_BUILD_TYPE "Release")
IF(CMAKE_CONFIGURATION_TYPES) # multiconfig generator?
    SET(CMAKE_CONFIGURATION_TYPES "Release" CACHE STRING "" FORCE)
ELSE(CMAKE_CONFIGURATION_TYPES)
    IF(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "Release")
        SET(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
    ELSE(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "Release")
        MESSAGE(FATAL_ERROR "The selected build type ${CMAKE_BUILD_TYPE} is invalid! The only valid build type is Release.")
    ENDIF(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "Release")
    SET_PROPERTY(CACHE CMAKE_BUILD_TYPE PROPERTY HELPSTRING "Choose the type of build")
    # set the valid options for cmake-gui drop-down list
    SET_PROPERTY(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Release")
ENDIF(CMAKE_CONFIGURATION_TYPES)

MESSAGE(STATUS "Selected Build Type: ${CMAKE_BUILD_TYPE}")

# Generate a database with the exact compiler calls for all translation units of the project in machine readable format:
SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)
MESSAGE(STATUS "INFO: The exact compiler calls for all translation units of ${CMAKE_PROJECT_NAME} can be found at ${PROJECT_BINARY_DIR}/compile_commands.json.")

# Suppress the full output (e.g. incl. full compiler calls) of the makefile:
SET(CMAKE_VERBOSE_MAKEFILE OFF)

# Load the project specific CMake modules:
SET(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_SOURCE_DIR}/cmake/Modules/")

# Set the used C++-Standard:CMakeFiles/free_list_queue_alternatives.dir/src/free_list_queue_alternatives/main.cpp.o: In function `cds::Initialize(unsigned int)':
SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED YES)
SET(CMAKE_CXX_EXTENSIONS OFF)

# Allow the usage of pthreads:
SET(THREADS_PREFER_PTHREAD_FLAG ON)
FIND_PACKAGE(Threads REQUIRED)

## _REENTRANT and _POSIX_PTHREAD_SEMANTICS are needed on Solaris to ensure
# that the posix-compliant clib interfaces are there (e.g., ctime_r)
# (Actually, you're supposed to get them both with _POSIX_C_SOURCE > 199506L
# but I'm not that trusting.)
SET_PROPERTY(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS _REENTRANT _POSIX_PTHREAD_SEMANTICS)

FIND_PACKAGE(Boost REQUIRED COMPONENTS program_options thread)

FIND_PACKAGE(LibCDS REQUIRED)

FIND_PACKAGE(TBB REQUIRED)

FIND_PACKAGE(Glog REQUIRED)

FIND_PACKAGE(Folly REQUIRED)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src/third_party)

IF("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-strict-aliasing")
ELSEIF("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-strict-aliasing")
ELSEIF("${CMAKE_CXX_COMPILER_ID}" MATCHES "Intel")
    SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -no-ansi-alias")
ELSE("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    MESSAGE(AUTHOR_WARNING "The C++-Compiler is neither Clang, GCC nor ICPC so some compiler-specific flags cannot be set.")
ENDIF("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")

ADD_EXECUTABLE(free_list_queue_alternatives ${CMAKE_SOURCE_DIR}/src/free_list_queue_alternatives/free_list_queue_alternatives.cpp)

TARGET_LINK_LIBRARIES(free_list_queue_alternatives
                      Threads::Threads
                      ${Boost_LIBRARIES}
                      LibCDS::LibCDS
                      tbb
                      Folly::Folly
                      glog)