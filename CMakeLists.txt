cmake_minimum_required(VERSION 3.21)

project(tunnel-drill VERSION 0.1.0)

include(./cmake/options.cmake)
include(./cmake/common.cmake)
include(./cmake/sanitize.cmake)

include(./cmake/dependencies/cxxopts.cmake)
include(./cmake/dependencies/jwt-cpp.cmake)
include(./cmake/dependencies/spdlog.cmake)

set(JWT_DISABLE_PICOJSON ON CACHE BOOL "no picojson")

if (ENABLE_SANITIZERS)
    enable_address_sanitizer()
    enable_undefined_sanitizer()
    enable_thread_sanitizer()
endif()

# Inlined Dependencies
add_subdirectory(dependencies/roar)

# Project Native
add_subdirectory(sharedpp/src)
add_subdirectory(brokerpp/src)
add_subdirectory(publisherpp/src)