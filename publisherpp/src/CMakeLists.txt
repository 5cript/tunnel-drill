add_library(publisher-lib STATIC
  publisherpp/connector.cpp
)

target_include_directories(
  publisher-lib
  PUBLIC
  ${CMAKE_SOURCE_DIR}/publisherpp/include
)

target_link_libraries(
  publisher-lib
  PRIVATE
    project-settings
    project-warnings
  PUBLIC
    attender
)

add_executable(publisher 
  publisherpp/main.cpp
)

target_link_libraries(
  publisher
  PRIVATE
    project-settings
    project-warnings
    publisher-lib
)

apply_project_properties(publisher)