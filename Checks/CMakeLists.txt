add_executable(spaceinvaders_checks main.cpp)
target_link_libraries(spaceinvaders_checks PRIVATE invaders::core spaceinvaders_warnings)
set_target_properties(
    spaceinvaders_checks
    PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)
add_test(NAME simulation-checks COMMAND spaceinvaders_checks)
