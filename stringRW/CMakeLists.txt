cmake_minimum_required(VERSION 3.22)

add_compile_options(-Wall -Wextra)

add_executable(stringRW stringRW.cpp)
target_link_libraries(stringRW jsoncpp)
