#ifndef _INTERFACE_H
#define _INTERFACE_H

#include <imgui.h>
#include <GLFW/glfw3.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>



class Interface {
    private:
        GLFWwindow *window;

    public:
        Interface(GLFWwindow *window) : window(window){}
        ~Interface();
        ImGuiIO* io;
        void initContext();
        void initWithOpenGL();
        void newFrame();
        void drawData();
};

#endif // _INTERFACE_H