cmake_minimum_required(VERSION 3.13)
project(h264-mp4-encoder)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fpermissive")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lpthread")

add_library(h264-mp4-encoder-library
    h264-mp4-encoder.cpp
)

target_include_directories(h264-mp4-encoder-library PRIVATE
    "generated"
    "minimp4"
    "minih264"
)

#add_executable(h264-mp4-encoder-test
#    main.cpp
#)

#target_link_libraries(h264-mp4-encoder-test h264-mp4-encoder-library)

if(EMSCRIPTEN)
    set(CMAKE_CXX_FLAGS "\
        ${CMAKE_CXX_FLAGS}\
        -s ALLOW_MEMORY_GROWTH=1\
        -s DISABLE_EXCEPTION_CATCHING=0\
        -s WASM=1\
        -msimd128\
        -msse\
        -msse2\
        -msse3\
        -mfpu=neon\
        -s EXTRA_EXPORTED_RUNTIME_METHODS=['FS']\
        -s MODULARIZE=1\
        -s EXPORT_ES6=1\
        -s EXPORT_NAME=H264MP4Module\
        -Wno-invalid-source-encoding\
        -Wno-tautological-pointer-compare\
        -O3\
        --closure 1\
        -g0\
        --bind\
    ")

        #-s DISABLE_EXCEPTION_CATCHING=0\

    add_executable(h264-mp4-encoder
        emscripten.cpp
    )
    #target_compile_options(h264-mp4-encoder
    #  PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    #)
    target_link_libraries(h264-mp4-encoder h264-mp4-encoder-library)
    set_target_properties(
        h264-mp4-encoder
        PROPERTIES
            SUFFIX ".js"
    )
endif()
