set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/CMakeModules)

set(SRCS
        emu_window/libretro_window.cpp
        input/input_factory.cpp
        input/mouse_tracker.cpp
        citra_libretro.cpp
        environment.cpp
        core_settings.cpp
        )
set(HEADERS
        emu_window/libretro_window.h
        input/input_factory.h
        input/mouse_tracker.h
        core_settings.h
        citra_libretro.h
        environment.h
        )

create_directory_groups(${SRCS} ${HEADERS})

add_library(citra_libretro SHARED ${SRCS} ${HEADERS})
target_link_libraries(citra_libretro PRIVATE common core)
target_link_libraries(citra_libretro PRIVATE glad libretro)

set_target_properties(citra_libretro PROPERTIES PREFIX "")
