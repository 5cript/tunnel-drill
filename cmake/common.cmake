add_library(project-settings INTERFACE)

function(apply_project_properties target)
  set_target_properties(${target}
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  )
endfunction()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  target_link_libraries(project-settings INTERFACE -lstdc++)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libstdc++")
endif()

add_library(project-warnings INTERFACE)
include(${CMAKE_SOURCE_DIR}/cmake/compilerwarnings.cmake)
set_project_warnings(project-warnings)