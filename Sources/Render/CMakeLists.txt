add_library(
    spaceinvaders_render
    STATIC
    Canvas.cpp
    PixelFont.cpp
    Scene.cpp
)
add_library(invaders::render ALIAS spaceinvaders_render)

target_include_directories(
    spaceinvaders_render
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(spaceinvaders_render PUBLIC invaders::core PRIVATE spaceinvaders_warnings)
set_target_properties(
    spaceinvaders_render
    PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)
