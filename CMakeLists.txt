cmake_minimum_required(VERSION 3.21)

project(tunnel-drill VERSION 0.1.0)

include(./cmake/options.cmake)
include(./cmake/common.cmake)

include(./cmake/dependencies/attender.cmake)
include(./cmake/dependencies/jwt-cpp.cmake)
include(./cmake/dependencies/roar.cmake)
include(./cmake/dependencies/spdlog.cmake)

set(JWT_DISABLE_PICOJSON ON CACHE BOOL "no picojson")

# Project Native
add_subdirectory(brokerpp/src)
add_subdirectory(publisherpp/src)