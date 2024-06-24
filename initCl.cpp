#include "init.h"

void initOpenCL(cl::Device* device, cl::Context* context, cl::Platform* platform) {
    cl_int err;
    std::vector<cl::Device> devices;
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);

    for (auto& p : platforms) {
        err = p.getDevices(CL_DEVICE_TYPE_GPU, &devices);
        if (devices.size() > 0) {
            *platform = p;
            break;
        } else if (err != CL_SUCCESS) {
            std::cerr << "Failed to get OpenCL device ID" << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    if (devices.size() == 0) {
        std::cerr << "Not GPU device found" << std::endl;
        exit(EXIT_FAILURE);
    }

    *device = devices.front();
    std::cout << "GPU Used: " << (*device).getInfo<CL_DEVICE_NAME>()
            << std::endl;

    cl_context_properties props[] = {
#ifdef _WIN32
        CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(),
        CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(),
#endif
        CL_CONTEXT_PLATFORM, (cl_context_properties)(*platform)(),
        0
    };

    *context = cl::Context((*device), props, NULL, NULL, &err);
    // cl_context context = clCreateContext(props, 1, device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        std::cerr << "Failed to create OpenCL context" << std::endl;
        exit(EXIT_FAILURE);
    }

}

void initProgram(cl::Program* program, cl::Kernel* kernel, std::string kernelSource, cl::Device* device, cl::CommandQueue* queue, cl::Context* context) {

    cl::Program::Sources sources;
    sources.push_back({kernelSource.c_str(), kernelSource.length()});
    
    *program = cl::Program(*context, sources);
    if (program->build({*device}) != CL_SUCCESS) {
        std::cerr << "Error building OpenCL program: " << program->getBuildInfo<CL_PROGRAM_BUILD_LOG>(*device) << std::endl;
        exit(EXIT_FAILURE);
    }

    *queue = cl::CommandQueue(*context, *device);
    *kernel = cl::Kernel(*program, "bodyInteraction");
}