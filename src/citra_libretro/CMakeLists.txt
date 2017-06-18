set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/CMakeModules)

set(SRCS
        citra_libretro.cpp emu_window/libretro_window.cpp environment.cpp
        input/input_factory.cpp
        )
set(HEADERS
        citra_libretro.h emu_window/libretro_window.h environment.h
        input/input_factory.h)

create_directory_groups(${SRCS} ${HEADERS})

add_library(citra_libretro SHARED ${SRCS} ${HEADERS})
target_link_libraries(citra_libretro PRIVATE common core)
target_link_libraries(citra_libretro PRIVATE inih glad libretro)

set_target_properties(citra_libretro PROPERTIES PREFIX "")
