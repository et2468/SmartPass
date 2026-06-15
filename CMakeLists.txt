cmake_minimum_required(VERSION 3.16)
project(MyProject VERSION 1.0 LANGUAGES CXX)

# MSVC 런타임 라이브러리 명시적 설정 (Debug/Release 모드 동적 대응)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# FFmpeg 모듈 경로 직접 지정
list(APPEND CMAKE_MODULE_PATH "C:/vcpkg/installed/x64-windows/share/ffmpeg")

# Qt5
find_package(Qt5 REQUIRED COMPONENTS Widgets)

# FFmpeg
find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h HINTS "C:/vcpkg/installed/x64-windows/include")
find_library(AVCODEC_LIB avcodec HINTS "C:/vcpkg/installed/x64-windows/lib")
find_library(AVFORMAT_LIB avformat HINTS "C:/vcpkg/installed/x64-windows/lib")
find_library(AVUTIL_LIB avutil HINTS "C:/vcpkg/installed/x64-windows/lib")
find_library(SWSCALE_LIB swscale HINTS "C:/vcpkg/installed/x64-windows/lib")

# OpenCV
find_package(OpenCV REQUIRED HINTS "C:/vcpkg/installed/x64-windows/share/opencv4")

# dlib
find_package(dlib REQUIRED HINTS "C:/vcpkg/installed/x64-windows/share/dlib")

# PostgreSQL
find_path(PGSQL_INCLUDE_DIR libpq-fe.h HINTS "C:/Program Files/PostgreSQL/16/include")
find_library(PGSQL_LIB pq HINTS "C:/Program Files/PostgreSQL/16/lib")

# 실행 파일 구성
add_executable(MyProject
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    rtspworker.cpp
    rtspworker.h
    facemanager.cpp
    facemanager.h
    dbworker.cpp
    dbworker.h
)

target_include_directories(MyProject PRIVATE
    ${AVCODEC_INCLUDE_DIR}
    ${OpenCV_INCLUDE_DIRS}
    ${PGSQL_INCLUDE_DIR}
)

target_link_libraries(MyProject PRIVATE
    Qt5::Widgets
    ${AVCODEC_LIB}
    ${AVFORMAT_LIB}
    ${AVUTIL_LIB}
    ${SWSCALE_LIB}
    ${OpenCV_LIBS}
    dlib::dlib  # 💡 vcpkg가 알아서 CONFIG별(Debug/Release) 알맞은 lib을 매칭하도록 유도
    ${PGSQL_LIB}
)

set_target_properties(MyProject PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

# 💡 [개선] 바이너리 파일이 복사 중 깨지는 것을 방지하기 위해 
# CMAKE_COMMAND 내장 복사 대신 OS 고유의 바이너리 복사 명령(robocopy) 활용 트리거
# 또한, 멀티 구성(Debug/Release) 빌드 폴더 경로를 안전하게 추적하도록 변경
add_custom_command(TARGET MyProject POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/models"
    "$<TARGET_FILE_DIR:MyProject>/models"
    COMMENT "💡 [최종 안전 모드] CMake 내장 명령으로 models 파일 복사 중..."
)