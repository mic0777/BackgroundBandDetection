cmake_minimum_required(VERSION 3.5)
project(BackgroundBand VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

include_directories("/usr/local/include/opencv4/opencv2/include/")

SET("OpenCV_DIR" "/home/user1/opencv/opencv/cvbuild/")
find_package( OpenCV REQUIRED )

set(PROJECT_SOURCES
    worker.h
    worker.cpp
    main.cpp
    utility.h
    utility.cpp
)

add_executable(BackgroundBand ${PROJECT_SOURCES})
target_compile_features(BackgroundBand PRIVATE cxx_std_17)
target_link_libraries(BackgroundBand ${OpenCV_LIBS} stdc++fs)

MESSAGE(STATUS "Include dirs ${OpenCV_INCLUDE_DIRS}")
MESSAGE(STATUS "LINK LIBRARIES ${OpenCV_LIBS}")
