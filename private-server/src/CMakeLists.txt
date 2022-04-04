add_library(private-server-lib STATIC
  private-server/connector.cpp
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
    attender
)

target_include_directories(
  private-server
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

apply_project_properties(private-server)