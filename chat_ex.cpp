#include "init.h"
#include <chrono>
// #include <ctime>


int NUM_PARTICLES, LOCAL_SIZE, GROUP_SIZE;

float* hPosCol; // host opengl object for Points
float* hVel; // host opengl object for Points

float pSize, vel_limit;
int pos_x_limit = 1, pos_y_limit = 1, pos_z_limit = 1;

cl::Device device;
cl::Platform platform;
cl::CommandQueue queue;
cl::Context context;
cl::Program program;
cl::Kernel kernel;

cl::BufferGL posColBuff;
cl::Buffer velBuff;

// pos
GLuint posColVao;
GLuint posColVbo;
GLuint ShaderProgram;

const char* vertex_shader = "#version 330 core\n"
"in vec3 position;\n"
"in vec3 color;\n"
"out vec3 fragColor;\n"
"void main() {\n"
"fragColor = color;\n"
"gl_Position = vec4(position, 1.0f);\n"
"}\0"
;

const char* fragment_shader = "#version 330 core\n"
"in vec3 fragColor;\n"
"out vec4 outColor;\n"
"void main() {\n"
"    outColor = vec4(fragColor, 1.0f);\n"
"}\0"
;


void setBuffers() {
    int vertexSh = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexSh, 1, &vertex_shader, NULL);
    glCompileShader(vertexSh);
    int success;
    char infoLog[512];
    glGetShaderiv(vertexSh, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexSh, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    int fragmentSh = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentSh, 1, &fragment_shader, NULL);
    glCompileShader(fragmentSh);
    glGetShaderiv(fragmentSh, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentSh, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    ShaderProgram = glCreateProgram();
    glAttachShader(ShaderProgram, vertexSh);
    glAttachShader(ShaderProgram, fragmentSh);
    glLinkProgram(ShaderProgram);
    glGetProgramiv(ShaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(ShaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexSh);
    glDeleteShader(fragmentSh);
    // glUseProgram(ShaderProgram);


    hPosCol = new float[NUM_PARTICLES*6];
    hVel = new float[NUM_PARTICLES*3];
    init_values(pos_x_limit, pos_y_limit, pos_z_limit, hPosCol, NUM_PARTICLES*2);
    for (int i = 0; i < NUM_PARTICLES*3; i+=3) {
        hVel[i] = (float) rand()/RAND_MAX;
        hVel[i+1] = (float) rand()/RAND_MAX;
        hVel[i+2] = (float) rand()/RAND_MAX;
    }

    glGenVertexArrays(1, &posColVao);
    glBindVertexArray(posColVao);

    glGenBuffers(1, &posColVbo);
    glBindBuffer(GL_ARRAY_BUFFER, posColVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * NUM_PARTICLES, hPosCol, GL_DYNAMIC_DRAW);

    
    // pos coords
    GLuint position = glGetAttribLocation(ShaderProgram, "position");
    glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, sizeof(float)*6, 0);
    glEnableVertexAttribArray(position);

    // col coords
    GLuint color = glGetAttribLocation(ShaderProgram, "color");
    glVertexAttribPointer(color, 3, GL_FLOAT, GL_FALSE, sizeof(float)*6, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(color);

    glBindVertexArray(0);
    glDisableVertexAttribArray(position);
    glDisableVertexAttribArray(color);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    cl_int err;
    posColBuff = cl::BufferGL(context, CL_MEM_READ_WRITE, posColVbo, &err);
    if (err != CL_SUCCESS) {
        std::cerr << "Failed to create OpenCL buffer from OpenGL buffer" << std::endl;
        exit(EXIT_FAILURE);
    }

    velBuff = cl::Buffer(context, CL_MEM_READ_WRITE, (size_t)NUM_PARTICLES*3, NULL, &err);
    if (err != CL_SUCCESS) {
        std::cerr << "Failed to create OpenCL buffer for vel" << std::endl;
        exit(EXIT_FAILURE);
    }

    queue.enqueueWriteBuffer(velBuff, CL_TRUE, 0, NUM_PARTICLES*3, hVel);
}

void updatePos(float dt) {
    cl::Event ev;
    glFinish();
    // Acquiring OpenGL objects in OpenCL
    std::vector<cl::Memory> glObjects = {posColBuff};
    cl_int res = queue.enqueueAcquireGLObjects(&glObjects, NULL, &ev);
    ev.wait();
    // std::cout<<5<<std::endl;
    if (res!=CL_SUCCESS) {
        std::cout<<"Failed acquiring GL object: "<<res<<std::endl;
        exit(248);
    }

    // float step = 0.0001f;
    // Set the kernel arguments
    kernel.setArg(0, posColBuff);
    kernel.setArg(1, velBuff);
    kernel.setArg(2, NUM_PARTICLES);
    kernel.setArg(3, dt);
    cl::NDRange GlobalWorkSize(GROUP_SIZE, 1, 1);
    cl::NDRange LocalWorkSize(LOCAL_SIZE, 1, 1);

    queue.enqueueNDRangeKernel(kernel, cl::NullRange, GlobalWorkSize, LocalWorkSize);
    
    res = queue.enqueueReleaseGLObjects(&glObjects);
    if (res!=CL_SUCCESS) {
        std::cout<<"Failed releasing GL object: "<<res<<std::endl;
        exit(247);
    }

    queue.finish();
}

void animate(GLFWwindow* window) {
    // glBindBuffer( GL_ARRAY_BUFFER, posColVbo);
    // glVertexPointer( 3, GL_FLOAT, sizeof(float)*3, (void *)0 );
    // glEnableClientState( GL_VERTEX_ARRAY );
    // glColorPointer( 3, GL_FLOAT, sizeof(float)*3, (void *)(sizeof(float)*3) );
    // glEnableClientState( GL_COLOR_ARRAY );
    // glPointSize( 2. );
    // glDrawArrays( GL_POINTS, 0, NUM_PARTICLES );
    // glPointSize( 1. );

    glBindVertexArray(posColVao); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
    glPointSize( pSize );

    glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);

    // glDisableClientState( GL_VERTEX_ARRAY );
    // glDisableClientState( GL_COLOR_ARRAY );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glfwSwapBuffers(window);
    glfwPollEvents();
}

int main(int argc, char* argv[]) {
    using std::chrono::milliseconds;

    NUM_PARTICLES = std::atoi(argv[1]);
    LOCAL_SIZE = std::atoi(argv[2]);
    GROUP_SIZE = std::atoi(argv[3]);
    pSize = std::stof(argv[4]);
    vel_limit = std::stof(argv[5]);
    std::cout << NUM_PARTICLES << std::endl;
    std::cout << LOCAL_SIZE << std::endl;
    GLFWwindow* window;
    initOpenGL(&window);


    initOpenCL(&device, &context, &platform);
    std::string src_code = load_from_file("kernel.cl");
    initProgram(&program, &kernel, src_code, &device, &queue, &context);

    setBuffers();
    glClearColor(1.0, 1.0, 1.0, 1.0);

    float lastFrameTime = glfwGetTime();
    float currentFrameTime;
    float deltaTime;
    while (!glfwWindowShouldClose(window)) {
        currentFrameTime = glfwGetTime();
        deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;
        updatePos(deltaTime/10);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(ShaderProgram);
        animate(window);
    }

    glDeleteBuffers(1, &posColVbo);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
