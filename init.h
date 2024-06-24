#include <CL/cl.hpp>
#include <CL/cl_gl.h>
#include <iostream>
#include <vector>
#include "auxilliary.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Incluye los encabezados específicos de Windows
#ifdef _WIN32
#include <Windows.h>
#include <GL/wglew.h>
#endif


void initOpenCL(cl::Device* device, cl::Context* context, cl::Platform* platform);
void initProgram(cl::Program* program, cl::Kernel* kernel, std::string kernelSource, cl::Device* device, cl::CommandQueue* queue, cl::Context* context);
void initOpenGL(GLFWwindow** window);