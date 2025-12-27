#ifndef MODEL_H
#define MODEL_H

#include <glm/glm.hpp>
#include <string>
#include <vector>

class Shader;  // Forward declaration

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct Texture {
    unsigned int id;
    std::string type;
    std::string path;
};

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures);
    void Draw(Shader& shader);

private:
    unsigned int VAO, VBO, EBO;
    void setupMesh();
};

class Model {
public:
    Model(const std::string& modelPath);
    void Draw(Shader& shader);

private:
    std::vector<Mesh> meshes;
    std::string directory;
    std::vector<Texture> loadedTextures;

    void loadOBJ(const std::string& path);
    void loadTexturesForModel(const std::string& modelName);
    unsigned int loadTexture(const std::string& path);
    unsigned int createColorTexture(float r, float g, float b);
    void createFallbackModel(const std::string& modelName);
};

#endif