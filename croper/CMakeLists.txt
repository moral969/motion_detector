add_executable(video_cropper
    video_cropper.cpp
)

target_include_directories(video_cropper PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(video_cropper PRIVATE ${OpenCV_LIBS})
