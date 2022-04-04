cmake_minimum_required(VERSION 3.21)

project(tunnel-drill VERSION 0.1.0)

include(./cmake/common.cmake)

add_subdirectory(private-server/src)
add_subdirectory(public-server/src)