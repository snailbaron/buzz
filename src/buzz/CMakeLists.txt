add_executable(buzz
    error.cpp
    main.cpp
    sdl.cpp
)
target_link_libraries(buzz PRIVATE
    build-info

    evening
    tempo
    thing
    ve

    SDL2::SDL2
    SDL2_image::SDL2_image
)
