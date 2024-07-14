#include "body.h"
#include "init.h"
#include "interface.h"
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
int pos_x_limit = 5, pos_y_limit = 5, pos_z_limit = 5,mode=0;
float framerate=60.0f,radius;
float dtSpeed = 0.5f;
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


// illumination

// directional light default settings
glm::vec3 defDirDirection(0.05f, 0.05f, 0.05f);
glm::vec3 defDirAmbient(0.5f, 0.5f, 0.5f);
glm::vec3 defDirDiffuse(0.4f, 0.4f, 0.4f);
glm::vec3 defDirSpecular(0.5f, 0.5f, 0.5f);

// general illumination settigns
glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
glm::vec3 objectColor(1.0f, 0.5f, 0.31f);


void updateScenelight(Shader& shader) {
    shader.setVec3("dirLight.direction", defDirDirection);
    shader.setVec3("dirLight.ambient", defDirAmbient);
    shader.setVec3("dirLight.diffuse", defDirDiffuse);
    shader.setVec3("dirLight.specular", defDirSpecular);
}

void UserMenu() {
    // Convert currentTime to 24-hour format
    ImGui::Begin("Scene Info");  
    ImGui::Text("Frame Rate: %f", framerate);
    ImGui::Text("Position (%.2f, %.2f, %.2f)", globCamera->getPosition().x, globCamera->getPosition().y, globCamera->getPosition().z);
    ImGui::Text("Body Velocity:");
    ImGui::SliderFloat(" ", &dtSpeed, 0.0001f, 0.5f, "%.4f");


    ImGui::End();
}


void setBuffers() {
    hPosCol = new float[NUM_PARTICLES*6];
    hVel = new float[NUM_PARTICLES*3];
    init_values(pos_x_limit, pos_y_limit, pos_z_limit, hPosCol, NUM_PARTICLES);
    for (int i = 0; i < NUM_PARTICLES*3; i+=3) {
        hVel[i] = (float) rand()/RAND_MAX;
        hVel[i+1] = (float) rand()/RAND_MAX;
        hVel[i+2] = (float) rand()/RAND_MAX;
    }

    glGenBuffers(1, &posColVbo);
    glBindBuffer(GL_ARRAY_BUFFER, posColVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * NUM_PARTICLES, hPosCol, GL_STATIC_DRAW);
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

void renderBuffers() {
    glBindBuffer(GL_ARRAY_BUFFER, posColVbo);

    // pos coords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(float)*6, 0);
    glVertexAttribDivisor(2, 1);

    // col coords
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(float)*6, (void*)(3 * sizeof(float)));
    glVertexAttribDivisor(3, 1);

    // glBindBuffer(GL_ARRAY_BUFFER, 0);
    // glDisableVertexAttribArray(2);
    // glDisableVertexAttribArray(3);
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
    if(mode==1){
        kernel.setArg(4,radius);
    }
    cl::NDRange GlobalWorkSize(GROUP_SIZE, 1, 1);
    cl::NDRange LocalWorkSize(LOCAL_SIZE, 1, 1);

    queue.enqueueNDRangeKernel(kernel, cl::NullRange, GlobalWorkSize, LocalWorkSize);
    queue.enqueueReadBuffer(posColBuff, CL_TRUE, 0, sizeof(float)*6*NUM_PARTICLES, hPosCol);
    res = queue.enqueueReleaseGLObjects(&glObjects);
    if (res!=CL_SUCCESS) {
        std::cout<<"Failed releasing GL object: "<<res<<std::endl;
        exit(247);
    }

    queue.finish();
}

void animate(GLFWwindow* window, float dt, int numParticles) {
    
    body.bindBodyBuffers();
    body.RenderBody(dt);
    renderBuffers();
    GLsizei elem=500;
    // for(int i=0;500*i<(numParticles+500);i++){
    //     const void* offset = (const void*)(i * elem * sizeof(GLuint));
    //     glDrawElementsInstanced(GL_TRIANGLES, body.getIndicesSize()*3, GL_UNSIGNED_INT, offset, std::min(500.0f,(float)(numParticles-i*500)));
    // }
    glDrawElementsInstanced(GL_TRIANGLES, body.getIndicesSize()*3, GL_UNSIGNED_INT, 0, numParticles);
    body.unbindBodyBuffers();

    glfwSwapBuffers(window);
    glfwPollEvents();
}

int main(int argc, char* argv[]) {
    using std::chrono::milliseconds;

    NUM_PARTICLES = std::atoi(argv[1]);
    LOCAL_SIZE = std::atoi(argv[2]);
    GROUP_SIZE = std::atoi(argv[3]);
    radius = std::stof(argv[4]);
    int subdivision = std::stoi(argv[5]);
    vel_limit = std::stof(argv[6]);
    SCR_WIDTH = std::atoi(argv[7]);
    SCR_HEIGHT = std::atoi(argv[8]);
    pos_x_limit = std::atoi(argv[9]);
    pos_y_limit = std::atoi(argv[9]);
    pos_z_limit = std::atoi(argv[9]);
    mode= std::atoi(argv[10]);
    body.setParameters(radius, subdivision);

    std::cout << "NUM_PARTICLES " << NUM_PARTICLES << std::endl;
    std::cout << "LOCAL_SIZE " << LOCAL_SIZE << std::endl;
    std::cout << "GROUP_SIZE " << GROUP_SIZE << std::endl;
    std::cout << "radius " << radius << std::endl;
    std::cout << "subdivision " << subdivision << std::endl;
    std::cout << "vel_limit " << vel_limit << std::endl;
    std::cout << "SCR_WIDTH " << SCR_WIDTH << std::endl;
    std::cout << "SCR_HEIGHT " << SCR_HEIGHT << std::endl;
    GLFWwindow* window;
    // char *windowName = "N-Body Problem";
    initOpenGL(&window, SCR_WIDTH, SCR_HEIGHT, "N-Body Problem");

    // interface with imgui
    Interface gui(window);
    gui.initContext();

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    gui.initWithOpenGL();

    initOpenCL(&device, &context, &platform);
    std::string src_code;
    if(mode==1){
        src_code = load_from_file("collisionKernel.cl");
    }
    else{
        src_code = load_from_file("kernel.cl");
    }
    initProgram(&program, &kernel, src_code, &device, &queue, &context);


    // set shaders
    Shader mainShader("vertexSh.txt", "fragmentSh.txt");
    mainShader.use();
    
    // set buffers
    body.CreateBodyOnGPU();
    setBuffers();

    // set camera
    Camera camera(SCR_WIDTH, SCR_HEIGHT);
    camera.SetPosition(0.0f, 5.0f, 0.0f);
    camera.SetFront(0.0f, -1.0f, 0.0f);
    camera.SetUp(0.0f, 0.0f, 1.0f);
    globCamera = &camera;

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.2, 0.3, 0.3, 1.0);

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

        gui.newFrame();

        mainShader.use();
        mainShader.setMat4("model", globCamera->getModel());
        mainShader.setMat4("projection", globCamera->getProjection());
        mainShader.setMat4("view", globCamera->getView());

        updateScenelight(mainShader);
        mainShader.setVec3("objectColor", objectColor.x, objectColor.y, objectColor.z);
        mainShader.setVec3("lightColor", lightColor.x, lightColor.y, lightColor.z);
        mainShader.setVec3("viewPos", globCamera->getPosition());

        updatePos(deltaTime*dtSpeed);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        UserMenu();
        gui.drawData();
        animate(window, deltaTime*dtSpeed, NUM_PARTICLES);
        framerate = 1.0f/deltaTime;
        // std::cout<<"framerate: "<<1.0/deltaTime<<std::endl;
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