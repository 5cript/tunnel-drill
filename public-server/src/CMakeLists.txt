add_executable(public-server 
  public-server/main.cpp
)

target_link_libraries(
  public-server
  PRIVATE
    project-settings
)

apply_project_properties(public-server)