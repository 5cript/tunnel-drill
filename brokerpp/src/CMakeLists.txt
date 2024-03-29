add_library(broker-lib STATIC
    brokerpp/config.cpp
    brokerpp/user_control.cpp
    brokerpp/authority.cpp
    brokerpp/program_options.cpp
    brokerpp/control/control_session.cpp
    brokerpp/control/subscription.cpp
    brokerpp/control/dispatcher.cpp
    brokerpp/control/stream_parser.cpp
    brokerpp/publisher/publisher.cpp
    brokerpp/publisher/service.cpp
    brokerpp/publisher/tunnel_session.cpp
    brokerpp/publisher/publisher_token.cpp
    brokerpp/request_listener/authenticator.cpp
    brokerpp/request_listener/page_control_provider.cpp
)

target_include_directories(
    broker-lib
    PUBLIC
        ${CMAKE_SOURCE_DIR}/brokerpp/include
)

find_package(Boost 1.81.0 REQUIRED COMPONENTS system)

target_link_libraries(
    broker-lib
    PRIVATE
        project-settings
        project-warnings
    PUBLIC
        spdlog::spdlog
        roar
        jwt-cpp::jwt-cpp
        cxxopts::cxxopts
        Boost::system
        shared-lib
)

add_executable(broker 
    brokerpp/main.cpp
)

target_link_libraries(
    broker
    PRIVATE
        project-settings
        project-warnings
        broker-lib
)

set(DEBUG_COMPILE_FLAGS -g -fsized-deallocation)
set(RELEASE_COMPILE_FLAGS -O3 -fsized-deallocation)
target_compile_options(broker PRIVATE $<$<CONFIG:DEBUG>:${DEBUG_COMPILE_FLAGS}>)
target_compile_options(broker-lib PRIVATE $<$<CONFIG:DEBUG>:${DEBUG_COMPILE_FLAGS}>)
target_compile_options(broker PRIVATE $<$<CONFIG:RELEASE>:${RELEASE_COMPILE_FLAGS}>)
target_compile_options(broker-lib PRIVATE $<$<CONFIG:RELEASE>:${RELEASE_COMPILE_FLAGS}>)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if (MSYS2_CLANG)
        add_custom_command(TARGET broker POST_BUILD
            COMMAND bash -c "cp /mingw64/bin/libatomic-1.dll ${CMAKE_BINARY_DIR}/bin/libatomic-1.dll"
            VERBATIM
        )
    endif()
endif()

message("${CMAKE_SOURCE_DIR}/brokerpp/include")

apply_project_properties(broker)