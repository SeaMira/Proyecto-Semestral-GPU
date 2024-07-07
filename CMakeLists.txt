cmake_minimum_required(VERSION 3.16.3)
project(NBodyProblem)
set(CMAKE_CXX_STANDARD 17)

add_executable(${PROJECT_NAME} chat_ex.cpp)


find_package(OpenGL REQUIRED)
find_package(GLFW3 REQUIRED)
set(OpenCL_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(OpenCL_LIBRARY "C:/Windows/System32/OpenCL.dll")

add_subdirectory(extern)
add_subdirectory(src)




target_link_libraries(${PROJECT_NAME} NBodyProblemDependencies)