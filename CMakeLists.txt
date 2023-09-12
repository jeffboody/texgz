cmake_minimum_required(VERSION 3.4.1)

# Compiler options
add_compile_options(-Wall)

if(TEXGZ_USE_PNG)
    set(SOURCE_PNG
        texgz_png.c)
endif()

if(TEXGZ_USE_JPEG)
    set(SOURCE_JPEG
        texgz_jpeg.c)
    set(LIBS_JPEG
        myjpeg)
endif()

# Submodule library
add_library(texgz
            STATIC

            # Source
            ${SOURCE_PNG}
            ${SOURCE_JPEG}
            pil_lanczos.c
            texgz_tex.c)

# Linking
target_link_libraries(texgz

                      # Submodule libraries
                      ${LIBS_JPEG}

                      # NDK libraries
                      z
                      log)
