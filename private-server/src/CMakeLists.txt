add_library(private-server-lib STATIC
  private-server/connector.cpp
)

target_include_directories(
  private-server-lib
  PUBLIC
  ${CMAKE_SOURCE_DIR}/private-server/include
)

target_link_libraries(
  private-server-lib
  PRIVATE
    project-settings
    project-warnings
  PUBLIC
    attender
)

add_executable(private-server 
  private-server/main.cpp
)

target_link_libraries(
  private-server
  PRIVATE
    project-settings
    project-warnings
    private-server-lib
)

apply_project_properties(private-server)