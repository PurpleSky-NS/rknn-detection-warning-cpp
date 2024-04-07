aux_source_directory(. SRC_LIST)
add_executable(main ${SRC_LIST})

target_link_libraries(main PRIVATE rknn_api pthread avcodec avformat avutil swscale)

find_package(OpenCV REQUIRED)
message(STATUS "OpenCV library status:")
message(STATUS "    version: ${OpenCV_VERSION}")
message(STATUS "    libraries: ${OpenCV_LIBS}")
message(STATUS "    include path: ${OpenCV_INCLUDE_DIRS}")
message(STATUS "    static include: ${OpenCV_STATIC_INCLUDE_DIR}")
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
find_package(spdlog CONFIG REQUIRED)
target_link_libraries(main PRIVATE spdlog::spdlog)

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