cmake_minimum_required(VERSION 3.10)
project(MotionTools)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Найти OpenCV
find_package(OpenCV REQUIRED)

# motion_detector executable
add_executable(motion_detector
    moution_detector/motion_detector.cpp
    moution_detector/markCreator.cpp
)

target_include_directories(motion_detector PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(motion_detector PRIVATE ${OpenCV_LIBS})

# Добавить подпроект croper
add_subdirectory(croper)
