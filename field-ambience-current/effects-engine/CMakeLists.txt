cmake_minimum_required(VERSION 3.16)
project(ambient_effects_handoff C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(ambient_effects STATIC firmware/src/ambient_effects.c)
target_include_directories(ambient_effects PUBLIC firmware/include)
target_compile_options(ambient_effects PRIVATE
    -Wall -Wextra -Wpedantic -Wshadow -Wconversion)
target_link_libraries(ambient_effects PUBLIC m)

add_executable(ambient_effects_verify tests/effects_verify.c)
target_link_libraries(ambient_effects_verify PRIVATE ambient_effects)

add_executable(ambient_effects_benchmark tests/effects_benchmark.c)
target_link_libraries(ambient_effects_benchmark PRIVATE ambient_effects)

add_executable(render_ambient_effects tools/render_effects.c)
target_link_libraries(render_ambient_effects PRIVATE ambient_effects)

enable_testing()
add_test(NAME ambient_effects_verify COMMAND ambient_effects_verify)
