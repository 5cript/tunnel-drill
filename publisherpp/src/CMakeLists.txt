add_library(publisher-lib STATIC
    publisherpp/publisher.cpp
    publisherpp/config.cpp
    publisherpp/service.cpp
    publisherpp/service_session.cpp
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
        roar
        shared-lib
        spdlog
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

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if (MSYS2_CLANG)
        add_custom_command(TARGET publisher POST_BUILD
            COMMAND bash -c "cp /mingw64/bin/libatomic-1.dll ${CMAKE_BINARY_DIR}/bin/libatomic-1.dll"
            VERBATIM
        )
    endif()
endif()

apply_project_properties(publisher)