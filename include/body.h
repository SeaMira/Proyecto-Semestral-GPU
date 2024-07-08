#ifndef _BODY_H_
#define _BODY_H_

#include <vector>
#include <unordered_map>
#include <cmath>
#include <glad/glad.h>

struct Vertex {
    float x, y, z;
    float nx, ny, nz;
};

struct Triangle {
    unsigned int v1, v2, v3;
    Triangle(unsigned int v1, unsigned int v2, unsigned int v3) : v1(v1), v2(v2), v3(v3) {}
};

class Body {
    public:
        void setRadius(float r);
        void setSubdivisions(int n);

        float getRadius();
        int getSubdivisions();
        Vertex* getVertexData();
        int getVertexSize();
        Triangle* getIndicesData();
        int getIndicesSize();

        void normalize(float& x, float& y, float& z);
        size_t getMidPointIndex(size_t index1, size_t index2, std::unordered_map<uint64_t, size_t>& midpointCache);
        void generateIcosphere();

        void CreateBodyOnGPU();
        void bindBodyBuffers();
        void RenderBody(float dt);
        void unbindBodyBuffers();

        Body();
        Body(float r, int subdivisions);
        ~Body() {}

    private:
        float r;
        int subdivisions;
        std::vector<Vertex> vertices;
        std::vector<Triangle> indices;

        GLuint bodyVao = -1;
        GLuint bodyVbo = -1;
        GLuint bodyIbo = -1;

};

#endif // _BODY_H_