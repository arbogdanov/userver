cmake_minimum_required(VERSION 3.12)
cmake_policy(SET CMP0025 NEW)
project(userver)

set(USERVER_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}")

set(USERVER_NOT_INCLUDED_AS_SUBDIR OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(USERVER_NOT_INCLUDED_AS_SUBDIR ON)
endif()
option(USERVER_IS_THE_ROOT_PROJECT "Build tests, samples and helper tools" ${USERVER_NOT_INCLUDED_AS_SUBDIR})
if (USERVER_IS_THE_ROOT_PROJECT)
  message(STATUS "Building userver as a primary project")
else()
  message(STATUS "Building userver as a subproject")
endif()

execute_process(
  COMMAND bash -c "apt-key adv --list-keys '7FCD11186050CD1A' || (brew tap | grep 'yandex')"
  RESULT_VARIABLE USERVER_NOT_YANDEX_ENV
  OUTPUT_QUIET
  ERROR_QUIET
)

option(USERVER_OPEN_SOURCE_BUILD "Build without Yandex and MLU packages" ${USERVER_NOT_YANDEX_ENV})
if (NOT USERVER_OPEN_SOURCE_BUILD)
  message(STATUS "Using internal to Yandex build setup")
endif()

option(USERVER_DOWNLOAD_PACKAGES "Download missing third party packages and use the downloaded versions" ${USERVER_OPEN_SOURCE_BUILD})

option(USERVER_FEATURE_CRYPTOPP_BLAKE2 "Provide wrappers for blake2 algorithms of crypto++" ON)
if (NOT USERVER_FEATURE_CRYPTOPP_BLAKE2)
  add_definitions("-DUSERVER_NO_CRYPTOPP_BLAKE2=1")
endif()

option(USERVER_FEATURE_CRYPTOPP_BASE64_URL "Provide wrappers for Base64 URL decoding and encoding algorithms of crypto++" ON)
if (NOT USERVER_FEATURE_CRYPTOPP_BASE64_URL)
  add_definitions("-DUSERVER_NO_CRYPTOPP_BASE64_URL=1")
endif()

option(USERVER_CHECK_PACKAGE_VERSIONS "Check package versions" ON)

include(cmake/SetupEnvironment.cmake)
include(AddGoogleTests)
include(CheckSubmodule)
include(Sanitizers)
include(FindPackageRequired)
include(IncludeWhatYouUse)
include(FindPython)

message(STATUS "Generating cmake files ...")
execute_process(
    COMMAND
        ${PYTHON} -u ${CMAKE_CURRENT_SOURCE_DIR}/plugins/external_deps/impl/cmake_generator.py
        --repo-dir=${CMAKE_CURRENT_SOURCE_DIR}
        --build-dir=${CMAKE_BINARY_DIR}
    RESULT_VARIABLE RESULT
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

if (RESULT)
    message(FATAL_ERROR
        "Generating cmake files failed with exit code: ${RESULT}"
    )
endif(RESULT)

set(USERVER_THIRD_PARTY_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/third_party CACHE INTERNAL "")

init_debian_depends()

add_subdirectory(core "${CMAKE_BINARY_DIR}/userver/core")
add_subdirectory("${USERVER_THIRD_PARTY_DIRS}/boost_stacktrace")
add_subdirectory("${USERVER_THIRD_PARTY_DIRS}/compiler-rt")
add_subdirectory(uboost_coro)

if (USERVER_IS_THE_ROOT_PROJECT)
    include(testsuite/SetupUserverTestsuiteEnv.cmake)
    add_subdirectory(testsuite)
    add_subdirectory(tools/engine)
    add_subdirectory(tools/json2yaml)
    add_subdirectory(tools/httpclient)
    add_subdirectory(tools/netcat)
    add_subdirectory(tools/dns_resolver)
    add_subdirectory(tools/congestion_control_emulator)
endif()

option(USERVER_FEATURE_MONGODB "Provide asynchronous driver for MongoDB" ON)
if (USERVER_FEATURE_MONGODB)
    add_subdirectory(mongo "${CMAKE_BINARY_DIR}/userver/mongo")
endif()

option(USERVER_FEATURE_POSTGRESQL "Provide asynchronous driver for PostgreSQL" ON)
if (USERVER_FEATURE_POSTGRESQL)
    add_subdirectory(postgresql "${CMAKE_BINARY_DIR}/userver/postgresql")
endif()

option(USERVER_FEATURE_REDIS "Provide asynchronous driver for Redis" ON)
if (USERVER_FEATURE_REDIS)
    add_subdirectory(redis "${CMAKE_BINARY_DIR}/userver/redis")
endif()

option(USERVER_FEATURE_GRPC "Provide asynchronous driver for gRPC" ON)
if (USERVER_FEATURE_GRPC)
    add_subdirectory(grpc "${CMAKE_BINARY_DIR}/userver/grpc")
endif()

option(USERVER_FEATURE_CLICKHOUSE "Provide asynchronous driver for ClickHouse" ON)
if (USERVER_FEATURE_CLICKHOUSE)
    add_subdirectory(clickhouse "${CMAKE_BINARY_DIR}/userver/clickhouse")
endif()

option(USERVER_FEATURE_UNIVERSAL "Provide a universal utilities library that does not use coroutines" ON)
if (USERVER_FEATURE_UNIVERSAL)
  add_subdirectory(universal "${CMAKE_BINARY_DIR}/userver/universal")
endif()

if (USERVER_IS_THE_ROOT_PROJECT)
    add_subdirectory(samples)
endif()
