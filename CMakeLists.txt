cmake_minimum_required(VERSION 3.16)
project(TextRenderer)

set(CMAKE_C_STANDARD 11)

# Find packages
find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

# Find FreeType and HarfBuzz
pkg_check_modules(FREETYPE REQUIRED freetype2)
pkg_check_modules(HARFBUZZ REQUIRED harfbuzz)

# Include directories
include_directories(${FREETYPE_INCLUDE_DIRS})
include_directories(${HARFBUZZ_INCLUDE_DIRS})
include_directories(src)

# Add executable (no more GLAD source file needed!)
add_executable(text_renderer text_renderer.c)
add_executable(test test.c)

# Link libraries
target_link_libraries(text_renderer 
    OpenGL::GL
    glfw
    ${FREETYPE_LIBRARIES}
    ${HARFBUZZ_LIBRARIES}
    m
)

# Link libraries for test
target_link_libraries(test 
    OpenGL::GL
    glfw
    ${FREETYPE_LIBRARIES}
    ${HARFBUZZ_LIBRARIES}
    m
)

# Compiler flags
target_compile_options(text_renderer PRIVATE ${FREETYPE_CFLAGS_OTHER} ${HARFBUZZ_CFLAGS_OTHER})
# test is broken
#target_compile_options(test PRIVATE ${FREETYPE_CFLAGS_OTHER} ${HARFBUZZ_CFLAGS_OTHER})

