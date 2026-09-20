cmake_minimum_required(VERSION 3.20)
project(avtool CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDATD_REQUIRED ON)

set(FFMPEG_ROOT "D:/FFmpeg/ffmpeg-master-latest-win64-gpl-shared")

add_executable(avtool src/main.cpp)

target_include_directories(avtool PRIVATE "${FFMPEG_ROOT}/include")
target_link_directories(avtool PRIVATE "${FFMPEG_ROOT}/lib")
target_link_libraries(avtool PRIVATE avformat avcodec avutil swscale swresample)
