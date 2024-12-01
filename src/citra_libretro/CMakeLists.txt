set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/CMakeModules)

add_library(citra_libretro SHARED
        emu_window/libretro_window.cpp
        emu_window/libretro_window.h
        input/input_factory.cpp
        input/input_factory.h
        input/mouse_tracker.cpp
        input/mouse_tracker.h
        vulkan/vk_swapchain.cpp
        vulkan/vk_swapchain.h
        citra_libretro.cpp
        citra_libretro.h
        environment.cpp
        environment.h
        core_settings.cpp
        core_settings.h)

create_target_directory_groups(citra_libretro)

target_link_libraries(citra_common PRIVATE libretro)
target_link_libraries(citra_core PRIVATE libretro)
target_link_libraries(video_core PRIVATE libretro)
target_compile_definitions(citra_common PRIVATE HAVE_LIBRETRO)
target_compile_definitions(citra_core PRIVATE HAVE_LIBRETRO)
target_compile_definitions(video_core PRIVATE HAVE_LIBRETRO)

target_link_libraries(citra_libretro PRIVATE citra_common citra_core)
target_link_libraries(citra_libretro PRIVATE boost dds-ktx glad libretro robin_map sirit vulkan-headers)
target_link_libraries(citra_libretro PRIVATE ${PLATFORM_LIBRARIES} Threads::Threads)
if(DEFINED LIBRETRO_STATIC)
target_link_libraries(citra_libretro PRIVATE -static-libstdc++)
endif()

set_target_properties(citra_libretro PROPERTIES PREFIX "")
target_compile_definitions(citra_libretro PRIVATE HAVE_LIBRETRO)

if(ANDROID)
  target_compile_definitions(citra_libretro PRIVATE USING_GLES)
endif()
