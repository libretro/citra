set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/CMakeModules)

add_library(citra_libretro SHARED
        emu_window/libretro_window.cpp
        emu_window/libretro_window.h
        input/input_factory.cpp
        input/input_factory.h
        input/mouse_tracker.cpp
        input/mouse_tracker.h
        citra_libretro.cpp
        citra_libretro.h
        environment.cpp
        environment.h
        core_settings.cpp
        core_settings.h
        )

create_target_directory_groups(citra_libretro)

target_link_libraries(citra_libretro PRIVATE common core)
target_link_libraries(citra_libretro PRIVATE glad libretro)

set_target_properties(citra_libretro PROPERTIES PREFIX "")
