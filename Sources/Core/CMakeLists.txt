add_library(
    spaceinvaders_core
    STATIC
    Game.cpp
    Bunker.cpp
)
add_library(invaders::core ALIAS spaceinvaders_core)

target_include_directories(
    spaceinvaders_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(spaceinvaders_core PRIVATE spaceinvaders_warnings)
set_target_properties(
    spaceinvaders_core
    PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)
