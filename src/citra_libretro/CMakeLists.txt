set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/CMakeModules)

set(SRCS
        citra_libretro.cpp libretro_window.cpp environment.cpp
        )
set(HEADERS
        citra_libretro.h libretro_window.h environment.h)

create_directory_groups(${SRCS} ${HEADERS})

add_library(citra_libretro SHARED ${SRCS} ${HEADERS})
target_link_libraries(citra_libretro PRIVATE common core input_common)
target_link_libraries(citra_libretro PRIVATE inih glad libretro)
