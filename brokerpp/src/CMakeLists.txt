add_library(broker-lib STATIC
  brokerpp/config.cpp
  brokerpp/load_home_file.cpp
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
  brokerpp/publisher/pipe_operation.cpp
  brokerpp/request_listener/authenticator.cpp
  brokerpp/request_listener/page_control_provider.cpp
)

target_include_directories(
  broker-lib
  PUBLIC
  ${CMAKE_SOURCE_DIR}/brokerpp/include
)

find_package(Boost 1.78.0 REQUIRED COMPONENTS system program_options)

target_link_libraries(
  broker-lib
  PRIVATE
    project-settings
    project-warnings
  PUBLIC
    roar
    attender
    spdlog
    jwt-cpp::jwt-cpp
    Boost::program_options
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