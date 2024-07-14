file(GLOB SOURCES *.cpp *.h)

add_library(NBodyProblemDependencies ${SOURCES})
target_link_libraries(NBodyProblemDependencies PUBLIC glfw3 glad glm ${OpenCL_LIBRARY} imgui)
# target_link_libraries(surfaceLeveldependencies PUBLIC glfw3 glad glm imgui)
target_link_libraries(imgui PRIVATE glfw3)
target_include_directories(NBodyProblemDependencies PUBLIC "${CMAKE_SOURCE_DIR}/include" ${OPENGL_INCLUDE_DIRS} ${GLFW3_INCLUDE_DIRS})

set(CL_VERSION ${OpenCL_VERSION_MAJOR}${OpenCL_VERSION_MINOR}0)
target_compile_definitions(${PROJECT_NAME} PRIVATE CL_HPP_TARGET_OPENCL_VERSION=${CL_VERSION} CL_HPP_MINIMUM_OPENCL_VERSION=${CL_VERSION})