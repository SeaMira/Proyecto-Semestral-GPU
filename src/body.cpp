#include <body.h>
#include <iostream>

// empty constructor
Body::Body() {
    this->r = 1.0f;
    this->subdivisions = 3;
    generateIcosphere();
}

// full constructor
Body::Body(float r, int subdivisions) {
    this->r = r * subdivisions;
    this->subdivisions = subdivisions;
    generateIcosphere();
}


// Función para normalizar un vector
void Body::normalize(float& x, float& y, float& z) {
    float length = std::sqrt(x * x + y * y + z * z);
    x /= length;
    y /= length;
    z /= length;
}

// Función para obtener la clave de un vector (para evitar duplicados)
size_t Body::getMidPointIndex(size_t index1, size_t index2, std::unordered_map<uint64_t, size_t>& midpointCache) {
    uint64_t smallerIndex = std::min(index1, index2);
    uint64_t greaterIndex = std::max(index1, index2);
    uint64_t key = (smallerIndex << 32) + greaterIndex;

    auto it = midpointCache.find(key);
    if (it != midpointCache.end()) {
        return it->second;
    }

    Vertex& v1 = vertices[index1];
    Vertex& v2 = vertices[index2];
    Vertex midpoint = {
        (v1.x + v2.x) / 2.0f, (v1.y + v2.y) / 2.0f, (v1.z + v2.z) / 2.0f,
        0.0f, 0.0f, 0.0f
    };
    normalize(midpoint.x, midpoint.y, midpoint.z);
    midpoint.nx = midpoint.x;
    midpoint.ny = midpoint.y;
    midpoint.nz = midpoint.z;

    vertices.push_back(midpoint);
    size_t index = vertices.size() - 1;
    midpointCache[key] = index;

    return index;
}

// Función para generar una icosfera
void Body::generateIcosphere() {
    vertices.clear();
    indices.clear();

    // Vértices iniciales del icosaedro
    float t = (1.0 + std::sqrt(5.0)) / 2.0;

    vertices = {
        {-1,  t,  0, 0, 0, 0},
        { 1,  t,  0, 0, 0, 0},
        {-1, -t,  0, 0, 0, 0},
        { 1, -t,  0, 0, 0, 0},
        { 0, -1,  t, 0, 0, 0},
        { 0,  1,  t, 0, 0, 0},
        { 0, -1, -t, 0, 0, 0},
        { 0,  1, -t, 0, 0, 0},
        { t,  0, -1, 0, 0, 0},
        { t,  0,  1, 0, 0, 0},
        {-t,  0, -1, 0, 0, 0},
        {-t,  0,  1, 0, 0, 0}
    };

    for (auto& v : vertices) {
        normalize(v.x, v.y, v.z);
        v.nx = v.x;
        v.ny = v.y;
        v.nz = v.z;
    }

    // Caras iniciales del icosaedro
    indices = {
        {0, 11, 5},
        {0, 5, 1},
        {0, 1, 7},
        {0, 7, 10}, 
        {0, 10, 11},
        {1, 5, 9},
        {5, 11, 4}, 
        {11, 10, 2},  
        {10, 7, 6}, 
        {7, 1, 8},
        {3, 9, 4},
        {3, 4, 2},
        {3, 2, 6},
        {3, 6, 8},
        {3, 8, 9},
        {4, 9, 5},
        {2, 4, 11}, 
        {6, 2, 10}, 
        {8, 6, 7},
        {9, 8, 1}
    };

    // Subdivisiones
    std::unordered_map<uint64_t, size_t> midpointCache;
    for (int i = 0; i < subdivisions; ++i) {
        std::vector<Triangle> newIndices;
        for (size_t j = 0; j < indices.size(); j ++) {
            size_t v1 = indices[j].v1;
            size_t v2 = indices[j].v2;
            size_t v3 = indices[j].v3;

            size_t a = getMidPointIndex(v1, v2, midpointCache);
            size_t b = getMidPointIndex(v2, v3, midpointCache);
            size_t c = getMidPointIndex(v3, v1, midpointCache);


            newIndices.push_back(Triangle(v1, a, c));

            newIndices.push_back(Triangle(v2, b, a));

            newIndices.push_back(Triangle(v3, c, b));

            newIndices.push_back(Triangle(a, b, c));
        }
        indices = newIndices;
    }

    // Ajustar el radio y calcular las coordenadas de textura
    for (auto& v : vertices) {
        v.x *= r;
        v.y *= r;
        v.z *= r;
    }
}

// setters
void Body::setRadius(float r) {
    this->r = r;
}

void Body::setSubdivisions(int n) {
    this->subdivisions = n;
}

void Body::setParameters(float r, int n) {
    setRadius(r);
    setSubdivisions(n);
    generateIcosphere();
}

// getters
float Body::getRadius() {
    return this->r;
}

int Body::getSubdivisions() {
    return this->subdivisions;
}

int Body::getVertexSize() {
    return (this->vertices).size();
} 

Vertex* Body::getVertexData() {
    return &vertices[0];
} 

int Body::getIndicesSize() {
    return (this->indices).size();
} 

Triangle* Body::getIndicesData() {
    return &indices[0];
} 


// gl buffers
void Body::CreateBodyOnGPU() {
    std::cout << "Creating VAO" << std::endl;
    std::cout << "Vertices count: " << getVertexSize() << std::endl;
    std::cout << "Size of Vertex: " << sizeof(Vertex) << std::endl;
    std::cout << "Indexes count: " << getIndicesSize() << std::endl;
    std::cout << "Size of Triangle: " << sizeof(Triangle) << std::endl;

    glGenVertexArrays(1, &bodyVao);
    glBindVertexArray(bodyVao);

    glGenBuffers(1, &bodyVbo);
    glBindBuffer(GL_ARRAY_BUFFER, bodyVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*getVertexSize(), getVertexData(), GL_STATIC_DRAW);

    
    glGenBuffers(1, &bodyIbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bodyIbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Triangle)*getIndicesSize(), getIndicesData(), GL_STATIC_DRAW);
    
    // glBindVertexArray(0);
    // glBindBuffer(GL_ARRAY_BUFFER, 0);
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Body::bindBodyBuffers() {
    glBindVertexArray(bodyVao);
    glBindBuffer(GL_ARRAY_BUFFER, bodyVbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bodyIbo);
}

void Body::bindBodyVAO() {
    glBindVertexArray(bodyVao);
}

void Body::unbindBodyBuffers() {
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    // glDisableVertexAttribArray(0);
    // glDisableVertexAttribArray(1);
}


// render 
void Body::RenderBody(float dt) {        

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));

}