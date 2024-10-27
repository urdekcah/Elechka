cmake_minimum_required(VERSION 3.25.0)
project(Elechka)

include_directories(include)
file(GLOB_RECURSE SRC_FILES src/*.cc)

add_executable(Elechka ${SRC_FILES})
target_link_libraries(Elechka TgBot boost_system ssl crypto pthread)