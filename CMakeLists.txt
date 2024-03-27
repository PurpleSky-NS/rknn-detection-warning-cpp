PROJECT(PURPLE)
cmake_minimum_required(VERSION 3.5.2)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

# add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)
# add_definitions(-Dgoogle=mindxsdk_private)
# add_compile_options(-std=c++17 -fPIC -fstack-protector-all -Wall)
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-z,relro,-z,now,-z,noexecstack -s -pie")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-z,relro,-z,now,-z,noexecstack -pie")  # 加-s会把调试信息删掉
# set(CMAKE_SKIP_RPATH TRUE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    include
    3rd
)

# link_directories(
#     ${MX_SDK_HOME}/lib/
#     ${MX_SDK_HOME}/lib/modelpostprocessors/
#     ${MX_SDK_HOME}/opensource/lib/
#     ${MX_SDK_HOME}/opensource/lib64/
# )

add_subdirectory(src)