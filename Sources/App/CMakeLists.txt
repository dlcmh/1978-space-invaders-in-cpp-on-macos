add_executable(
    spaceinvaders_app
    AppDelegate.mm
    main.mm
    ../Platform/GameView.mm
    ../Platform/AudioEngine.cpp
    ../Render/MetalRenderer.mm
)

target_include_directories(
    spaceinvaders_app
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/Sources/Platform
        ${CMAKE_SOURCE_DIR}/Sources/Render
)

target_link_libraries(
    spaceinvaders_app
    PRIVATE
        invaders::render
        invaders::core
        spaceinvaders_warnings
        "-framework AppKit"
        "-framework Metal"
        "-framework MetalKit"
        "-framework QuartzCore"
        "-framework AudioToolbox"
        "-framework Foundation"
)

target_compile_options(spaceinvaders_app PRIVATE -fobjc-arc)
set_target_properties(
    spaceinvaders_app
    PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
        MACOSX_BUNDLE NO
)
