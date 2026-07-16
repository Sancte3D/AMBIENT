cmake_minimum_required(VERSION 3.16)
project(sancte_ambient_models C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(sancte_ambient_models STATIC
    firmware/src/ambient_models.c
    firmware/src/ambient_visuals.c
    firmware/src/ambient_palettes.c)
target_include_directories(sancte_ambient_models PUBLIC firmware/include)
target_compile_options(sancte_ambient_models PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(sancte_ambient_models PUBLIC m)

add_executable(ambient_verify tests/verify.c)
target_link_libraries(ambient_verify PRIVATE sancte_ambient_models)

add_executable(ambient_benchmark tests/benchmark.c)
target_link_libraries(ambient_benchmark PRIVATE sancte_ambient_models)

add_executable(render_ambient_audio tools/render_audio.c)
target_link_libraries(render_ambient_audio PRIVATE sancte_ambient_models)

add_executable(render_ambient_ui tools/render_ui.c)
target_link_libraries(render_ambient_ui PRIVATE sancte_ambient_models)

enable_testing()
add_test(NAME ambient_verify COMMAND ambient_verify)
