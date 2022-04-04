add_executable(private-server 
  private-server/main.cpp
)

target_link_libraries(
  private-server
  PRIVATE
    project-settings
)

apply_project_properties(private-server)