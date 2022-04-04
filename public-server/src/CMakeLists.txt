add_library(public-server-lib STATIC
  public-server/connection_broker.cpp
)

target_include_directories(
  public-server-lib
  PUBLIC
  ${CMAKE_SOURCE_DIR}/public-server/include
)

target_link_libraries(
  public-server-lib
  PRIVATE
    project-settings
    project-warnings
  PUBLIC
    attender
    spdlog
)

add_executable(public-server 
  public-server/main.cpp
)

target_link_libraries(
  public-server
  PRIVATE
    project-settings
    project-warnings
    public-server-lib
)

message("${CMAKE_SOURCE_DIR}/public-server/include")

apply_project_properties(public-server)