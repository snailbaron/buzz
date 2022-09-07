add_library(build-info
    "${CMAKE_CURRENT_BINARY_DIR}/build-info.cpp"
)
target_include_directories(build-info PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
configure_file(build-info.cpp.in build-info.cpp @ONLY)
