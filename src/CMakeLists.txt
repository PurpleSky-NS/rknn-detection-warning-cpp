aux_source_directory(. SRC_LIST)
add_executable(main ${SRC_LIST})
# add_executable(main main.cpp)

find_package(OpenCV REQUIRED)
message(STATUS "OpenCV library status:")
message(STATUS "    version: ${OpenCV_VERSION}")
message(STATUS "    libraries: ${OpenCV_LIBS}")
message(STATUS "    include path: ${OpenCV_INCLUDE_DIRS}")
message(STATUS "    static include: ${OpenCV_STATIC_INCLUDE_DIR}")
include_directories(${OpenCV_INCLUDE_DIRS})

target_link_libraries(main ${OpenCV_LIBS}) 
target_link_libraries(main rknn_api pthread avcodec avformat avutil swscale)