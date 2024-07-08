#include "init.h"
#include "shader_m.h"
#include "camera3.h"
#include <chrono>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window, float deltaTime);


int NUM_PARTICLES, LOCAL_SIZE, GROUP_SIZE;
int SCR_WIDTH, SCR_HEIGHT;

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
GLuint posColVbo;

// camera
Camera* globCamera;

// body
Body body;

void setBuffers() {
    hPosCol = new float[NUM_PARTICLES*6];
    hVel = new float[NUM_PARTICLES*3];
    init_values(pos_x_limit, pos_y_limit, pos_z_limit, hPosCol, NUM_PARTICLES*2);
    for (int i = 0; i < NUM_PARTICLES*3; i+=3) {
        hVel[i] = (float) rand()/RAND_MAX;
        hVel[i+1] = (float) rand()/RAND_MAX;
        hVel[i+2] = (float) rand()/RAND_MAX;
    }


    glGenBuffers(1, &posColVbo);
    glBindBuffer(GL_ARRAY_BUFFER, posColVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * NUM_PARTICLES, hPosCol, GL_DYNAMIC_DRAW);

    // pos coords
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(float)*6, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribDivisor(3, 1);

    // col coords
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(float)*6, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(4, 1);

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
    radius = std::stof(argv[4]);
    subdivision = std::stoi(argv[5]);
    vel_limit = std::stof(argv[6]);
    SCR_WIDTH = std::atoi(argv[7]);
    SCR_HEIGHT = std::atoi(argv[8]);

    std::cout << NUM_PARTICLES << std::endl;
    std::cout << LOCAL_SIZE << std::endl;
    GLFWwindow* window;
    // char *windowName = "N-Body Problem";
    initOpenGL(&window, SCR_WIDTH, SCR_HEIGHT, "N-Body Problem");

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    initOpenCL(&device, &context, &platform);
    std::string src_code = load_from_file("kernel.cl");
    initProgram(&program, &kernel, src_code, &device, &queue, &context);


    // set shaders
    Shader mainShader("vertexSh.txt", "fragmentSh.txt");
    mainShader.use();
    
    // set buffers
    setBuffers();

    // set camera
    Camera camera(SCR_WIDTH, SCR_HEIGHT);
    camera.SetPosition(0.0f, 5.0f, 0.0f);
    camera.SetFront(0.0f, -1.0f, 0.0f);
    camera.SetUp(0.0f, 0.0f, 1.0f);
    globCamera = &camera;


    glClearColor(1.0, 1.0, 1.0, 1.0);

    float lastFrameTime = glfwGetTime();
    float currentFrameTime;
    float deltaTime;
    while (!glfwWindowShouldClose(window)) {
        // updating delta between frames
        currentFrameTime = glfwGetTime();
        deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;

        // camera movement and rotation
        processInput(window, deltaTime);
        camera.OnRender(deltaTime*10.0f);

        mainShader.use();
        mainShader.setMat4("model", globCamera->getModel());
        mainShader.setMat4("projection", globCamera->getProjection());
        mainShader.setMat4("view", globCamera->getView());

        updatePos(deltaTime/10);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        animate(window);
    }

    glDeleteBuffers(1, &posColVbo);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window, float deltaTime)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        globCamera->OnKeyboard(1, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        globCamera->OnKeyboard(2, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        globCamera->OnKeyboard(3, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        globCamera->OnKeyboard(4, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        globCamera->OnKeyboard(5, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        globCamera->OnKeyboard(6, deltaTime);

    
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height){
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    SCR_WIDTH = width; SCR_HEIGHT = height;
    globCamera->SetScrSize(width, height);
    glViewport(0, 0, width, height);
}


void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    globCamera->OnMouse((float)xposIn, (float)yposIn);
}

// // glfw: whenever the mouse scroll wheel scrolls, this callback is called
// // ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    globCamera->OnScroll(static_cast<float>(yoffset));
}