add_executable(public-server 
  public-server/main.cpp
)

target_link_libraries(
  public-server
  PRIVATE
    project-settings
    project-warnings
)

target_include_directories(
  public-server
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

apply_project_properties(public-server)