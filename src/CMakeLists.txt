aux_source_directory(. SRC_LIST)
add_executable(main ${SRC_LIST})

find_package(FFMPEG MODULE)
target_include_directories(main PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(main PRIVATE ${FFMPEG_LIBRARY_DIRS})
# target_link_libraries(main PRIVATE avfilter avformat avcodec avutil swresample avdevice swscale)
target_link_libraries(main PRIVATE ${FFMPEG_LIBRARIES})

find_package(spdlog CONFIG REQUIRED)
target_link_libraries(main PRIVATE spdlog::spdlog)

find_package(OpenCV REQUIRED)
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
include_directories(${OpenCV_INCLUDE_DIRS})
target_link_libraries(main PRIVATE ${OpenCV_LIBS})

# find_package(Freetype REQUIRED)
# message(STATUS "Freetype library status")
# message(STATUS "    ${FREETYPE_INCLUDE_DIRS}")
# message(STATUS "    ${FREETYPE_INCLUDE_DIR_ft2build}")
# message(STATUS "    ${FREETYPE_LIBRARIES}")
# include_directories(${FREETYPE_INCLUDE_DIRS})
# target_link_libraries(main ${FREETYPE_LIBRARIES})
# target_link_libraries(main ${FREETYPE_LIBRARIES})

find_package(argparse CONFIG REQUIRED)
target_link_libraries(main PRIVATE argparse::argparse)

find_package(jsoncpp CONFIG REQUIRED)
target_link_libraries(main PRIVATE JsonCpp::JsonCpp)

find_package(httplib CONFIG REQUIRED)
target_link_libraries(main PRIVATE httplib::httplib)

# vcpkg这里的导入有问题，这个不是header-only的库，需要对.cpp进行编译
find_path(CPP_BASE64_SOURCE_DIR "cpp-base64/base64.cpp")
aux_source_directory(${CPP_BASE64_SOURCE_DIR}/cpp-base64 CPP_BASE64_SOURCE)
target_sources(main PRIVATE ${CPP_BASE64_SOURCE})

target_link_libraries(main PRIVATE pthread librknn_api.so)

# add_executable(test test.cpp api.cpp)  
# target_link_libraries(test PRIVATE spdlog::spdlog)
# target_link_libraries(test PRIVATE JsonCpp::JsonCpp)
# target_link_libraries(test PRIVATE httplib::httplib)