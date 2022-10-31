add_library(shared-lib STATIC
    sharedpp/load_home_file.cpp
)

target_include_directories(
    shared-lib
    PUBLIC
        ${CMAKE_SOURCE_DIR}/sharedpp/include
)

target_link_libraries(
    shared-lib
    PRIVATE
        project-settings
        project-warnings
        roar
    PUBLIC
)