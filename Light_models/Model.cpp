#include "Model.h"
#include "Shader.h"

// Для filesystem в Visual Studio
#ifdef _MSC_VER
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

// Реализация класса Mesh
Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures) {
    this->vertices = vertices;
    this->indices = indices;
    this->textures = textures;
    setupMesh();
}

void Mesh::setupMesh() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void Mesh::Draw(Shader& shader) {
    // Привязываем текстуры
    for (unsigned int i = 0; i < textures.size(); i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, textures[i].id);

        if (textures[i].type == "texture_diffuse") {
            shader.setInt("material.texture_diffuse1", i);
        }
        else if (textures[i].type == "texture_specular") {
            shader.setInt("material.texture_specular1", i);
        }
    }

    // Если нет текстур, используем стандартный слот
    if (textures.empty()) {
        glActiveTexture(GL_TEXTURE0);
        shader.setInt("material.texture_diffuse1", 0);
    }

    // Отрисовка
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glActiveTexture(GL_TEXTURE0);
}

// Реализация класса Model
Model::Model(const std::string& modelPath) {
    std::cout << "Loading model: " << modelPath << std::endl;
    loadOBJ(modelPath);

    // Определяем имя модели для загрузки текстур
    fs::path p(modelPath);
    std::string modelName = p.stem().string();
    loadTexturesForModel(modelName);
}

void Model::loadOBJ(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cout << "ERROR: Cannot open model file: " << path << std::endl;

        // Определяем имя модели для создания fallback
        fs::path p(path);
        std::string modelName = p.stem().string();
        createFallbackModel(modelName);
        return;
    }

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "v") {
            glm::vec3 pos;
            iss >> pos.x >> pos.y >> pos.z;
            positions.push_back(pos);
        }
        else if (type == "vn") {
            glm::vec3 normal;
            iss >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        }
        else if (type == "vt") {
            glm::vec2 texCoord;
            iss >> texCoord.x >> texCoord.y;
            texCoords.push_back(texCoord);
        }
        else if (type == "f") {
            std::vector<std::string> faceData;
            std::string vertexData;

            while (iss >> vertexData) {
                faceData.push_back(vertexData);
            }

            // Обрабатываем треугольники
            for (size_t i = 1; i < faceData.size() - 1; i++) {
                std::string v1 = faceData[0];
                std::string v2 = faceData[i];
                std::string v3 = faceData[i + 1];

                std::string verticesStr[3] = { v1, v2, v3 };

                for (int j = 0; j < 3; j++) {
                    std::replace(verticesStr[j].begin(), verticesStr[j].end(), '/', ' ');
                    std::istringstream vStream(verticesStr[j]);

                    int posIdx = 0, texIdx = 0, normIdx = 0;
                    vStream >> posIdx;

                    if (!vStream.eof()) vStream >> texIdx;
                    if (!vStream.eof()) vStream >> normIdx;

                    Vertex vertex;

                    if (posIdx > 0 && posIdx <= positions.size())
                        vertex.Position = positions[posIdx - 1];

                    if (texIdx > 0 && texIdx <= texCoords.size())
                        vertex.TexCoords = texCoords[texIdx - 1];
                    else
                        vertex.TexCoords = glm::vec2(0.0f, 0.0f);

                    if (normIdx > 0 && normIdx <= normals.size())
                        vertex.Normal = normals[normIdx - 1];
                    else
                        vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);

                    vertices.push_back(vertex);
                    indices.push_back(vertices.size() - 1);
                }
            }
        }
    }

    file.close();

    std::cout << "Model loaded: " << path
        << " (vertices: " << vertices.size()
        << ", indices: " << indices.size() << ")" << std::endl;

    meshes.push_back(Mesh(vertices, indices, std::vector<Texture>()));
}

void Model::loadTexturesForModel(const std::string& modelName) {
    std::vector<Texture> textures;

    // Загружаем текстуры в зависимости от модели
    if (modelName == "model") {
        std::cout << "Loading textures for ruins..." << std::endl;

        std::string textureFiles[] = {
            "textures/tex_u1_v1_diffuse.jpeg",
            "textures/tex_u1_v2_diffuse.jpeg",
            "textures/tex_u2_v1_diffuse.jpeg",
            "textures/tex_u2_v2_diffuse.jpeg",
            "textures/tex_u3_v1_diffuse.jpeg",
            "textures/tex_u3_v2_diffuse.jpeg"
        };

        for (const auto& texFile : textureFiles) {
            unsigned int texID = loadTexture(texFile);
            if (texID != 0) {
                Texture tex;
                tex.id = texID;
                tex.type = "texture_diffuse";
                tex.path = texFile;
                textures.push_back(tex);
                std::cout << "Loaded texture: " << texFile << std::endl;
                break;
            }
        }

        if (textures.empty()) {
            std::cout << "No textures found for ruins, creating stone color..." << std::endl;
            unsigned int colorTex = createColorTexture(0.7f, 0.65f, 0.6f);
            Texture tex;
            tex.id = colorTex;
            tex.type = "texture_diffuse";
            tex.path = "stone_color";
            textures.push_back(tex);
        }
    }
    else if (modelName == "fb_abWishFairy") {
        std::cout << "Loading textures for fairy..." << std::endl;

        std::string fairyTexture = "textures/fb_abWishFairy_albedo.jpeg";
        unsigned int texID = loadTexture(fairyTexture);

        if (texID == 0) {
            std::string altTextures[] = {
                "textures/fairy_internal_ground_ao_texture.jpeg",
                "textures/fb_abWishFairy_specular.jpeg"
            };

            for (const auto& altTex : altTextures) {
                texID = loadTexture(altTex);
                if (texID != 0) break;
            }
        }

        if (texID != 0) {
            Texture tex;
            tex.id = texID;
            tex.type = "texture_diffuse";
            tex.path = fairyTexture;
            textures.push_back(tex);
            std::cout << "Loaded fairy texture" << std::endl;
        }
        else {
            std::cout << "No textures found for fairy, creating pink color..." << std::endl;
            unsigned int colorTex = createColorTexture(1.0f, 0.9f, 1.0f);
            Texture tex;
            tex.id = colorTex;
            tex.type = "texture_diffuse";
            tex.path = "fairy_color";
            textures.push_back(tex);
        }
    }
    else if (modelName == "dog day") {
        std::cout << "Creating brown color for dog..." << std::endl;
        unsigned int colorTex = createColorTexture(0.5f, 0.35f, 0.2f);
        Texture tex;
        tex.id = colorTex;
        tex.type = "texture_diffuse";
        tex.path = "dog_color";
        textures.push_back(tex);
    }
    else if (modelName == "fg_funkoFluttershy") {
        std::cout << "Loading textures for pony..." << std::endl;

        std::string ponyTexture = "textures/fg_funkoFluttershy_albedo.jpeg";
        unsigned int texID = loadTexture(ponyTexture);

        if (texID != 0) {
            Texture tex;
            tex.id = texID;
            tex.type = "texture_diffuse";
            tex.path = ponyTexture;
            textures.push_back(tex);
            std::cout << "Loaded pony texture" << std::endl;
        }
        else {
            std::cout << "No textures found for pony, creating pink color..." << std::endl;
            unsigned int colorTex = createColorTexture(1.0f, 0.7f, 0.8f);
            Texture tex;
            tex.id = colorTex;
            tex.type = "texture_diffuse";
            tex.path = "pony_color";
            textures.push_back(tex);
        }
    }
    else if (modelName == "SweeFinal") {
        std::cout << "Loading textures for Sweet..." << std::endl;

        std::string sweetTexture = "textures/SweeMainBody_SweeMainBody_BaseColor.png";
        unsigned int texID = loadTexture(sweetTexture);

        if (texID != 0) {
            Texture tex;
            tex.id = texID;
            tex.type = "texture_diffuse";
            tex.path = sweetTexture;
            textures.push_back(tex);
            std::cout << "Loaded Sweet texture" << std::endl;
        }
        else {
            std::cout << "No textures found for Sweet, creating blue color..." << std::endl;
            unsigned int colorTex = createColorTexture(0.8f, 0.8f, 0.9f);
            Texture tex;
            tex.id = colorTex;
            tex.type = "texture_diffuse";
            tex.path = "sweet_color";
            textures.push_back(tex);
        }
    }

    // Добавляем текстуры к мешу
    if (!meshes.empty() && !textures.empty()) {
        meshes[0].textures = textures;
        loadedTextures = textures;
    }
}

unsigned int Model::loadTexture(const std::string& path) {
    std::cout << "Attempting to load texture: " << path << std::endl;

    // Проверяем существование файла
    std::ifstream testFile(path);
    if (!testFile.good()) {
        std::cout << "Texture file not found: " << path << std::endl;
        return 0;
    }
    testFile.close();

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrComponents, 0);

    if (data) {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;
        else {
            std::cout << "Unknown texture format: " << nrComponents << " components" << std::endl;
            stbi_image_free(data);
            return 0;
        }

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);

        std::cout << "Successfully loaded texture: " << path
            << " (" << width << "x" << height << ", " << nrComponents << " components)" << std::endl;
        return textureID;
    }
    else {
        std::cout << "Failed to load texture: " << path << std::endl;
        stbi_image_free(data);
        return 0;
    }
}

unsigned int Model::createColorTexture(float r, float g, float b) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    unsigned char color[] = {
        (unsigned char)(r * 255),
        (unsigned char)(g * 255),
        (unsigned char)(b * 255),
        255
    };

    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, color);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return textureID;
}

void Model::createFallbackModel(const std::string& modelName) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float size = 1.0f;

    Vertex v;

    // Основание пирамиды
    v.Position = glm::vec3(-size, 0.0f, -size);
    v.Normal = glm::vec3(0.0f, -1.0f, 0.0f);
    v.TexCoords = glm::vec2(0.0f, 0.0f);
    vertices.push_back(v);

    v.Position = glm::vec3(size, 0.0f, -size);
    v.Normal = glm::vec3(0.0f, -1.0f, 0.0f);
    v.TexCoords = glm::vec2(1.0f, 0.0f);
    vertices.push_back(v);

    v.Position = glm::vec3(size, 0.0f, size);
    v.Normal = glm::vec3(0.0f, -1.0f, 0.0f);
    v.TexCoords = glm::vec2(1.0f, 1.0f);
    vertices.push_back(v);

    v.Position = glm::vec3(-size, 0.0f, size);
    v.Normal = glm::vec3(0.0f, -1.0f, 0.0f);
    v.TexCoords = glm::vec2(0.0f, 1.0f);
    vertices.push_back(v);

    // Вершина пирамиды
    v.Position = glm::vec3(0.0f, size * 2, 0.0f);
    v.Normal = glm::normalize(glm::vec3(0.0f, 1.0f, 0.5f));
    v.TexCoords = glm::vec2(0.5f, 0.5f);
    vertices.push_back(v);

    // Индексы для основания
    indices.push_back(0); indices.push_back(1); indices.push_back(2);
    indices.push_back(2); indices.push_back(3); indices.push_back(0);

    // Индексы для боковых граней
    indices.push_back(0); indices.push_back(1); indices.push_back(4);
    indices.push_back(1); indices.push_back(2); indices.push_back(4);
    indices.push_back(2); indices.push_back(3); indices.push_back(4);
    indices.push_back(3); indices.push_back(0); indices.push_back(4);

    meshes.push_back(Mesh(vertices, indices, std::vector<Texture>()));
    std::cout << "Created fallback pyramid for: " << modelName << std::endl;
}

void Model::Draw(Shader& shader) {
    for (unsigned int i = 0; i < meshes.size(); i++) {
        meshes[i].Draw(shader);
    }
}