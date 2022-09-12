add_executable(buzz
    config.cpp
    error.cpp
    main.cpp
    scene.cpp
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
