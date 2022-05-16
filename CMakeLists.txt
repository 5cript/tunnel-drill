cmake_minimum_required(VERSION 3.21)

project(tunnel-drill VERSION 0.1.0)

include(./cmake/options.cmake)
include(./cmake/common.cmake)

set(JWT_DISABLE_PICOJSON ON CACHE BOOL "no picojson")

# Submodules
add_subdirectory(roar)
add_subdirectory(jwt-cpp)
add_subdirectory(special-paths)
add_subdirectory(attender)
add_subdirectory(spdlog)

# Project Native
add_subdirectory(brokerpp/src)
add_subdirectory(publisherpp/src)