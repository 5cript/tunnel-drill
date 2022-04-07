add_library(broker-lib STATIC
  brokerpp/connection_acceptor.cpp
  brokerpp/session.cpp
  brokerpp/controller.cpp
  brokerpp/control/control_session.cpp
  brokerpp/control/subscription.cpp
  brokerpp/control/dispatcher.cpp
  brokerpp/control/stream_parser.cpp
)

target_include_directories(
  broker-lib
  PUBLIC
  ${CMAKE_SOURCE_DIR}/brokerpp/include
)

target_link_libraries(
  broker-lib
  PRIVATE
    project-settings
    project-warnings
  PUBLIC
    attender
    spdlog
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

message("${CMAKE_SOURCE_DIR}/brokerpp/include")

apply_project_properties(broker)