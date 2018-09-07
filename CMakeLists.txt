cmake_minimum_required(VERSION 3.5)
project(correctbase VERSION 1.0.0 LANGUAGES CXX)

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

add_library(ubxdriver
	src/ubxm8p.cpp
	src/rtcm.cpp
)

target_include_directories(ubxdriver
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(main src/main.cpp)
    
target_link_libraries(main PUBLIC ubxdriver)
