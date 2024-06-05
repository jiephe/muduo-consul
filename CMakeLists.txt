cmake_minimum_required(VERSION 3.10)

project(ConsulTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++0x -fpermissive")

include_directories(
                    ${CMAKE_CURRENT_SOURCE_DIR}/
)

file(GLOB SRC_FILE
                  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
                  ${CMAKE_CURRENT_SOURCE_DIR}/*.cc
)

set(DLIBS
    -L/usr/local/lib/
    -Wl,-rpath,/usr/local/lib/
    -lmuduo_base -lmuduo_net -lmuduo_http -lppconsul
    -lpthread -luuid -ldl -lstdc++fs  
)

add_executable(${PROJECT_NAME} ${SRC_FILE})
target_link_libraries(${PROJECT_NAME} ${DLIBS})
