#include <iostream>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "Camera.h"
#include "Model.h"

// Настройки окна
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 800;

// Камера
Camera camera(glm::vec3(0.0f, 8.0f, 15.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// Тайминги
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Обработка ввода
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

// Глобальные переменные для управления освещением
float lightIntensity = 1.2f;
float lightAngle = 0.0f;
bool lightMoving = false;

// Три типа освещения
bool dirLightEnabled = true;      // Направленный свет (солнце)
bool pointLightEnabled = true;    // Точечный свет (лампочка)
bool spotLightEnabled = true;     // Прожекторный свет (фонарик)

// Параметры прожектора
float spotLightCutOff = glm::cos(glm::radians(12.5f));      // Внутренний угол конуса
float spotLightOuterCutOff = glm::cos(glm::radians(17.5f)); // Внешний угол конуса

// Перечисление для моделей освещения
enum LightingModel {
    PHONG,
    TOON,
    MINNAERT
};

// Структура для хранения информации об объекте
struct SceneObject {
    Model* model;
    glm::vec3 position;
    glm::vec3 scale;
    float rotation;
    LightingModel lightingModel;
    std::string name;
};

// Глобальные переменные для управления шейдерами
int currentShaderMode = 0; // 0 = индивидуально, 1 = все Phong, 2 = все Toon, 3 = все Minnaert

// Функция для установки uniform'ов в шейдер
void setupShaderUniforms(Shader& shader, glm::mat4& projection, glm::mat4& view, Camera& camera,
    bool dirLightEnabled, bool pointLightEnabled, bool spotLightEnabled,
    float lightIntensity, float currentFrame, float spotLightCutOff,
    float spotLightOuterCutOff, bool isMinnaert = false) {

    // Общие uniform'ы
    shader.setMat4("projection", projection);
    shader.setMat4("view", view);
    shader.setVec3("viewPos", camera.Position);

    // Направленный свет
    shader.setBool("dirLight.enabled", dirLightEnabled);
    shader.setVec3("dirLight.direction", glm::vec3(-0.2f, -1.0f, -0.3f));
    shader.setVec3("dirLight.ambient", 0.2f * lightIntensity, 0.2f * lightIntensity, 0.2f * lightIntensity);
    shader.setVec3("dirLight.diffuse", 0.5f * lightIntensity, 0.5f * lightIntensity, 0.5f * lightIntensity);
    shader.setVec3("dirLight.specular", 0.5f * lightIntensity, 0.5f * lightIntensity, 0.5f * lightIntensity);

    // Параметры затухания для направленного света (не используются, но должны быть)
    shader.setFloat("dirLight.constant", 1.0f);
    shader.setFloat("dirLight.linear", 0.0f);
    shader.setFloat("dirLight.quadratic", 0.0f);

    // Точечный свет (движется по кругу)
    glm::vec3 pointLightPos = glm::vec3(
        5.0f * cos(currentFrame * 0.5f),
        8.0f,
        5.0f * sin(currentFrame * 0.5f)
    );

    shader.setBool("pointLight.enabled", pointLightEnabled);
    shader.setVec3("pointLight.position", pointLightPos);
    shader.setVec3("pointLight.ambient", 0.1f * lightIntensity, 0.1f * lightIntensity, 0.1f * lightIntensity);
    shader.setVec3("pointLight.diffuse", 0.8f * lightIntensity, 0.8f * lightIntensity, 0.8f * lightIntensity);
    shader.setVec3("pointLight.specular", 1.0f * lightIntensity, 1.0f * lightIntensity, 1.0f * lightIntensity);

    // Параметры затухания для точечного света
    shader.setFloat("pointLight.constant", 1.0f);
    shader.setFloat("pointLight.linear", 0.09f);
    shader.setFloat("pointLight.quadratic", 0.032f);

    // Прожектор (следует за камерой)
    shader.setBool("spotLight.enabled", spotLightEnabled);
    shader.setVec3("spotLight.position", camera.Position);
    shader.setVec3("spotLight.direction", camera.Front);
    shader.setFloat("spotLight.cutOff", spotLightCutOff);
    shader.setFloat("spotLight.outerCutOff", spotLightOuterCutOff);
    shader.setVec3("spotLight.ambient", 0.1f * lightIntensity, 0.1f * lightIntensity, 0.1f * lightIntensity);
    shader.setVec3("spotLight.diffuse", 1.0f * lightIntensity, 1.0f * lightIntensity, 1.0f * lightIntensity);
    shader.setVec3("spotLight.specular", 1.0f * lightIntensity, 1.0f * lightIntensity, 1.0f * lightIntensity);

    // Параметры затухания для прожектора
    shader.setFloat("spotLight.constant", 1.0f);
    shader.setFloat("spotLight.linear", 0.09f);
    shader.setFloat("spotLight.quadratic", 0.032f);

    // Материал - ВАЖНО: используем правильные имена uniform'ов
    shader.setFloat("material.shininess", 32.0f);
    shader.setBool("material.useTextures", true);

    // Для Minnaert устанавливаем дополнительный параметр
    if (isMinnaert) {
        shader.setFloat("material.minnaertPower", 1.2f);
    }
}

// Функция для установки uniform'ов материала и текстур для земли
void setupGroundShaderUniforms(Shader& shader, unsigned int groundTexture) {
    shader.use();

    // Активируем и привязываем текстуру
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, groundTexture);

    // Устанавливаем uniform'ы для текстур материала
    shader.setInt("material.texture_diffuse1", 0);
    shader.setInt("material.texture_specular1", 0);

    // Также устанавливаем старые uniform'ы для совместимости
    shader.setInt("texture_diffuse1", 0);
    shader.setInt("texture_specular1", 0);
}

int main() {
    // Устанавливаем скорость камеры
    camera.MovementSpeed = 10.0f;

    // Инициализация GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Создание окна
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Magical Scene with Multiple Lighting Models", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Инициализация GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Настройка OpenGL
    glEnable(GL_DEPTH_TEST);

    // Компиляция шейдеров
    std::cout << "\n=== Compiling Shaders ===" << std::endl;

    // ИСПОЛЬЗУЕМ ЕДИНЫЙ ВЕРШИННЫЙ ШЕЙДЕР ДЛЯ ВСЕХ МОДЕЛЕЙ ОСВЕЩЕНИЯ
    const char* vertexShaderPath = "shaders/vertex.glsl";

    // Загружаем шейдеры с использованием одного вершинного шейдера
    Shader phongShader(vertexShaderPath, "shaders/phong_fragment.glsl");
    std::cout << "1. Phong shader compiled. ID: " << phongShader.ID << std::endl;

    Shader toonShader(vertexShaderPath, "shaders/toon_fragment.glsl");
    std::cout << "2. Toon shader compiled. ID: " << toonShader.ID << std::endl;

    Shader minnaertShader(vertexShaderPath, "shaders/minnaert_fragment.glsl");
    std::cout << "3. Minnaert shader compiled. ID: " << minnaertShader.ID << std::endl;

    // Проверка успешности компиляции
    if (phongShader.ID == 0 || toonShader.ID == 0 || minnaertShader.ID == 0) {
        std::cout << "ERROR: Failed to compile one or more shaders!" << std::endl;
        std::cout << "Please check the shader files in the 'shaders' folder." << std::endl;
        return -1;
    }

    // Загрузка моделей с текстурами
    std::cout << "\n=== Loading Models with Textures ===" << std::endl;

    // 1. Руины дворца
    std::cout << "\n1. Loading ruins (CASTLE)..." << std::endl;
    Model ruins("models/model.obj");

    // 2. Фея
    std::cout << "\n2. Loading fairy..." << std::endl;
    //Model fairy("models/fb_abWishFairy.obj");

    // 3. Собака
    std::cout << "\n3. Loading dog..." << std::endl;
    //Model dog("models/dog day.obj");

    // 4. Пони
    std::cout << "\n4. Loading pony..." << std::endl;
    //Model pony("models/fg_funkoFluttershy.obj");

    // 5. Sweet
    std::cout << "\n5. Loading Sweet..." << std::endl;
    //Model sweet("models/SweeFinal.obj");

    std::cout << "\n=== All models loaded! Setting up scene... ===" << std::endl;

    // Уровень земли
    float groundLevel = -1.0f;

    // Создаем сцену с объектами
    std::vector<SceneObject> sceneObjects;

    // Замок - Phong
    sceneObjects.push_back({
        &ruins,
        glm::vec3(0.0f, groundLevel, 0.0f),
        glm::vec3(0.5f, 0.5f, 0.5f),
        90.0f,
        PHONG,
        "Castle"
        });

    // Для остальных объектов (раскомментируйте когда будут загружены):
    /*
    sceneObjects.push_back({
        &fairy,
        glm::vec3(2.0f, groundLevel + 0.5f, 0.0f),
        glm::vec3(3.2f, 3.2f, 3.2f),
        -45.0f,
        TOON,
        "Fairy"
    });

    sceneObjects.push_back({
        &dog,
        glm::vec3(2.2f, groundLevel, 0.5f),
        glm::vec3(0.5f, 0.5f, 0.5f),
        -90.0f,
        MINNAERT,
        "Dog"
    });

    sceneObjects.push_back({
        &pony,
        glm::vec3(2.4f, groundLevel + 0.5f, 0.0f),
        glm::vec3(1.4f, 1.4f, 1.4f),
        45.0f,
        PHONG,
        "Pony"
    });

    sceneObjects.push_back({
        &sweet,
        glm::vec3(0.0f, 5.0f, 0.0f),
        glm::vec3(0.45f, 0.45f, 0.45f),
        0.0f,
        TOON,
        "Sweet"
    });
    */

    // Создаем зеленую текстуру для земли
    unsigned int groundTexture;
    glGenTextures(1, &groundTexture);
    glBindTexture(GL_TEXTURE_2D, groundTexture);

    // Создаем простую зеленую текстуру 2x2 пикселя
    unsigned char grassData[] = {
        100, 160, 60, 255,     // темно-зеленый
        120, 180, 80, 255,     // средне-зеленый
        120, 180, 80, 255,     // средне-зеленый
        100, 160, 60, 255      // темно-зеленый
    };

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, grassData);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    std::cout << "Ground texture created. ID: " << groundTexture << std::endl;

    // Создаем VAO для земли
    unsigned int groundVAO, groundVBO, groundEBO;
    float groundVertices[] = {
        // Позиции           // Нормали         // Текстурные координаты
        -20.0f, 0.0f, -20.0f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
         20.0f, 0.0f, -20.0f,  0.0f, 1.0f, 0.0f,  10.0f, 0.0f,
         20.0f, 0.0f,  20.0f,  0.0f, 1.0f, 0.0f,  10.0f, 10.0f,
        -20.0f, 0.0f,  20.0f,  0.0f, 1.0f, 0.0f,  0.0f, 10.0f
    };

    unsigned int groundIndices[] = { 0, 1, 2, 2, 3, 0 };

    glGenVertexArrays(1, &groundVAO);
    glGenBuffers(1, &groundVBO);
    glGenBuffers(1, &groundEBO);

    glBindVertexArray(groundVAO);
    glBindBuffer(GL_ARRAY_BUFFER, groundVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(groundVertices), groundVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, groundEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(groundIndices), groundIndices, GL_STATIC_DRAW);

    // Атрибуты вершин
    // Позиция
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Нормаль
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // Текстурные координаты
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    std::cout << "Ground VAO created. ID: " << groundVAO << std::endl;

    std::cout << "\n=== Scene setup complete! Starting render loop... ===" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  W/A/S/D - Move camera" << std::endl;
    std::cout << "  Space/Shift - Move up/down" << std::endl;
    std::cout << "  Up/Down arrows - Adjust light intensity" << std::endl;
    std::cout << "  L - Toggle light movement" << std::endl;
    std::cout << "  F1 - Individual shaders per object" << std::endl;
    std::cout << "  F2 - All objects use Phong" << std::endl;
    std::cout << "  F3 - All objects use Toon" << std::endl;
    std::cout << "  F4 - All objects use Minnaert" << std::endl;
    std::cout << "  ESC - Exit" << std::endl;
    std::cout << "\nCurrent mode: Individual shaders per object" << std::endl;

    // Цикл рендеринга
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // Обновление угла света, если включено движение
        if (lightMoving) {
            lightAngle += 0.5f * deltaTime;
        }

        // Очистка экрана
        glClearColor(0.5f, 0.7f, 0.9f, 1.0f);  // Голубой цвет неба
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Матрицы вида и проекции
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
            (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();

        // ===========================================
        // 1. РИСУЕМ ЗЕМЛЮ (всегда используем Phong)
        // ===========================================
        phongShader.use();

        // Устанавливаем uniform'ы для земли
        setupShaderUniforms(phongShader, projection, view, camera,
            dirLightEnabled, pointLightEnabled, spotLightEnabled,
            lightIntensity, currentFrame, spotLightCutOff, spotLightOuterCutOff);

        // Настраиваем текстуры для земли
        setupGroundShaderUniforms(phongShader, groundTexture);

        // Матрица модели для земли
        glm::mat4 groundModel = glm::mat4(1.0f);
        groundModel = glm::translate(groundModel, glm::vec3(0.0f, groundLevel - 0.1f, 0.0f));
        groundModel = glm::scale(groundModel, glm::vec3(1.0f, 1.0f, 1.0f));
        phongShader.setMat4("model", groundModel);

        // Рисуем землю
        glBindVertexArray(groundVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // ===========================================
        // 2. РИСУЕМ ОБЪЕКТЫ СЦЕНЫ
        // ===========================================
        for (auto& obj : sceneObjects) {
            Shader* shaderToUse = &phongShader;
            bool isMinnaert = false;

            // Выбираем шейдер в зависимости от режима
            if (currentShaderMode == 0) {
                // Индивидуальные шейдеры
                switch (obj.lightingModel) {
                case PHONG: shaderToUse = &phongShader; break;
                case TOON: shaderToUse = &toonShader; break;
                case MINNAERT:
                    shaderToUse = &minnaertShader;
                    isMinnaert = true;
                    break;
                }
            }
            else if (currentShaderMode == 1) {
                // Все Phong
                shaderToUse = &phongShader;
            }
            else if (currentShaderMode == 2) {
                // Все Toon
                shaderToUse = &toonShader;
            }
            else if (currentShaderMode == 3) {
                // Все Minnaert
                shaderToUse = &minnaertShader;
                isMinnaert = true;
            }

            shaderToUse->use();

            // Устанавливаем uniform'ы
            setupShaderUniforms(*shaderToUse, projection, view, camera,
                dirLightEnabled, pointLightEnabled, spotLightEnabled,
                lightIntensity, currentFrame, spotLightCutOff, spotLightOuterCutOff,
                isMinnaert);

            // Создаем матрицу модели для объекта
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, obj.position);
            model = glm::rotate(model, glm::radians(obj.rotation), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, obj.scale);

            // Добавляем анимацию для некоторых объектов
            if (obj.name == "Sweet") {
                // Анимация полета Sweet
                glm::vec3 sweetOrbit = glm::vec3(
                    sin(currentFrame * 0.5f) * 5.0f,
                    5.0f + sin(currentFrame * 0.7f) * 1.0f,
                    cos(currentFrame * 0.5f) * 5.0f
                );
                model = glm::translate(glm::mat4(1.0f), sweetOrbit);
                model = glm::rotate(model, currentFrame * 2.0f, glm::vec3(0.0f, 1.0f, 0.0f));
                model = glm::scale(model, obj.scale);
            }
            else if (obj.name == "Fairy") {
                // Легкое парение феи
                model = glm::translate(model, glm::vec3(0.0f, sin(currentFrame * 0.5f) * 0.1f, 0.0f));
            }

            shaderToUse->setMat4("model", model);

            // Отрисовываем модель
            obj.model->Draw(*shaderToUse);
        }

        // Обновление заголовка окна
        std::string title = "3D Scene | Lighting: ";
        switch (currentShaderMode) {
        case 0: title += "Individual"; break;
        case 1: title += "All Phong"; break;
        case 2: title += "All Toon"; break;
        case 3: title += "All Minnaert"; break;
        }
        title += " | Intensity: " + std::to_string(lightIntensity).substr(0, 3);
        glfwSetWindowTitle(window, title.c_str());

        // Обмен буферов и обработка событий
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    std::cout << "\n=== Program finished successfully! ===" << std::endl;

    // Очистка ресурсов
    glDeleteVertexArrays(1, &groundVAO);
    glDeleteBuffers(1, &groundVBO);
    glDeleteBuffers(1, &groundEBO);
    glDeleteTextures(1, &groundTexture);

    glfwTerminate();
    return 0;
}

// ===========================================
// ОБРАБОТКА ВВОДА (остается без изменений)
// ===========================================
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Управление камерой
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime);

    static float lastLightChange = 0.0f;
    static float lastKeyPress = 0.0f;
    float currentTime = static_cast<float>(glfwGetTime());

    // Управление общей интенсивностью света
    if (currentTime - lastLightChange > 0.1f) {
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
            lightIntensity += 0.1f;
            lightIntensity = std::min(lightIntensity, 3.0f);
            std::cout << "Light intensity: " << lightIntensity << std::endl;
            lastLightChange = currentTime;
        }
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
            lightIntensity -= 0.1f;
            lightIntensity = std::max(lightIntensity, 0.1f);
            std::cout << "Light intensity: " << lightIntensity << std::endl;
            lastLightChange = currentTime;
        }
        if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
            lightMoving = !lightMoving;
            std::cout << "Light movement: " << (lightMoving ? "ON" : "OFF") << std::endl;
            lastLightChange = currentTime;
        }
    }

    // Управление тремя типами освещения
    if (currentTime - lastKeyPress > 0.3f) {
        // 1. Направленный свет - клавиша 1
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
            dirLightEnabled = !dirLightEnabled;
            std::cout << "Directional Light: " << (dirLightEnabled ? "ENABLED" : "DISABLED") << std::endl;
            lastKeyPress = currentTime;
        }

        // 2. Точечный свет - клавиша 2
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
            pointLightEnabled = !pointLightEnabled;
            std::cout << "Point Light: " << (pointLightEnabled ? "ENABLED" : "DISABLED") << std::endl;
            lastKeyPress = currentTime;
        }

        // 3. Прожекторный свет - клавиша 3
        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
            spotLightEnabled = !spotLightEnabled;
            std::cout << "Spot Light: " << (spotLightEnabled ? "ENABLED" : "DISABLED") << std::endl;
            lastKeyPress = currentTime;
        }

        // F1-F4: Управление моделями освещения
        if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS) {
            currentShaderMode = 0;
            std::cout << "Lighting mode: INDIVIDUAL per object" << std::endl;
            lastKeyPress = currentTime;
        }

        if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS) {
            currentShaderMode = 1;
            std::cout << "Lighting mode: ALL PHONG" << std::endl;
            lastKeyPress = currentTime;
        }

        if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS) {
            currentShaderMode = 2;
            std::cout << "Lighting mode: ALL TOON" << std::endl;
            lastKeyPress = currentTime;
        }

        if (glfwGetKey(window, GLFW_KEY_F4) == GLFW_PRESS) {
            currentShaderMode = 3;
            std::cout << "Lighting mode: ALL MINNAERT" << std::endl;
            lastKeyPress = currentTime;
        }

        // R. Сброс всех настроек - клавиша R
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
            dirLightEnabled = true;
            pointLightEnabled = true;
            spotLightEnabled = true;
            spotLightCutOff = glm::cos(glm::radians(12.5f));
            spotLightOuterCutOff = glm::cos(glm::radians(17.5f));
            lightIntensity = 1.2f;
            lightMoving = false;
            currentShaderMode = 0;

            camera.Position = glm::vec3(0.0f, 8.0f, 15.0f);
            camera.Yaw = -90.0f;
            camera.Pitch = 0.0f;
            camera.ProcessMouseMovement(0, 0);

            std::cout << "=== ALL SETTINGS RESET ===" << std::endl;
            lastKeyPress = currentTime;
        }

        // M. Показать справку - клавиша M
        if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
            std::cout << "\n=== CONTROLS ===" << std::endl;
            std::cout << "WASD + Space/Shift: Move camera" << std::endl;
            std::cout << "Mouse: Look around" << std::endl;
            std::cout << "Scroll: Zoom in/out" << std::endl;
            std::cout << "\n=== LIGHT CONTROLS ===" << std::endl;
            std::cout << "1/2/3: Toggle directional/point/spot lights" << std::endl;
            std::cout << "Up/Down: Adjust light intensity" << std::endl;
            std::cout << "L: Toggle light movement" << std::endl;
            std::cout << "\n=== SHADER CONTROLS ===" << std::endl;
            std::cout << "F1: Individual shaders per object" << std::endl;
            std::cout << "F2: All objects use Phong" << std::endl;
            std::cout << "F3: All objects use Toon" << std::endl;
            std::cout << "F4: All objects use Minnaert" << std::endl;
            std::cout << "\n=== OTHER CONTROLS ===" << std::endl;
            std::cout << "R: Reset all settings" << std::endl;
            std::cout << "M: Show this help" << std::endl;
            std::cout << "ESC: Exit" << std::endl;
            lastKeyPress = currentTime;
        }
    }
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}