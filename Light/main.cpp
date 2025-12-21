#include <iostream>
#include <vector>
#include <filesystem>

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
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Magical Scene with Correct Textures", NULL, NULL);
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

    // Компиляция шейдера
    Shader shader("shaders/vertex.glsl", "shaders/fragment.glsl");

    // Загрузка моделей с текстурами
    std::cout << "\n=== Loading Models with Textures ===" << std::endl;

    // 1. Руины дворца
    std::cout << "\n1. Loading ruins (CASTLE)..." << std::endl;
    Model ruins("models/model.obj");

    // 2. Фея
    std::cout << "\n2. Loading fairy..." << std::endl;
    Model fairy("models/fb_abWishFairy.obj");

    // 3. Собака
    std::cout << "\n3. Loading dog..." << std::endl;
    Model dog("models/dog day.obj");

    // 4. Пони
    std::cout << "\n4. Loading pony..." << std::endl;
    Model pony("models/fg_funkoFluttershy.obj");

    // 5. Sweet
    std::cout << "\n5. Loading Sweet..." << std::endl;
    Model sweet("models/SweeFinal.obj");

    std::cout << "\n=== All models loaded! Setting up scene... ===" << std::endl;

    // Уровень земли
    float groundLevel = -1.0f;

    // НОВАЯ КОМПОЗИЦИЯ: Фея, собака и пони рядом с замком
    // ПОЗИЦИИ ОБЪЕКТОВ:
    glm::vec3 ruinsPos(0.0f, groundLevel, 0.0f);               // Руины в центре
    glm::vec3 fairyPos(2.0f, groundLevel + 0.5f, 0.0f);       // Фея справа от замка
    glm::vec3 dogPos(2.2f, groundLevel, 0.5f);                // Собака ВПЕРЕДИ (большая координата Z), на уровне земли
    glm::vec3 ponyPos(2.4f, groundLevel + 0.5f, 0.0f);        // Пони справа от феи

    // МАСШТАБ ОБЪЕКТОВ (увеличены):
    // ЗАМОК - увеличен в 5 раз
    glm::vec3 ruinsScale(0.5f, 0.5f, 0.5f);

    // Фея - УВЕЛИЧЕНА В 8 РАЗ (очень большая)
    glm::vec3 fairyScale(3.2f, 3.2f, 3.2f);

    // Собака - УМЕНЬШЕНА в 2 раза (было 1.0f)
    glm::vec3 dogScale(0.5f, 0.5f, 0.5f);

    // Пони - увеличен в 4 раза
    glm::vec3 ponyScale(1.4f, 1.4f, 1.4f);

    // Sweet - увеличен в 3 раза
    glm::vec3 sweetScale(0.45f, 0.45f, 0.45f);

    // Создаем зеленую текстуру для земли
    unsigned int groundTexture;
    glGenTextures(1, &groundTexture);
    glBindTexture(GL_TEXTURE_2D, groundTexture);

    unsigned char grassData[] = {
        100, 160, 60, 255,
        120, 180, 80, 255,
        120, 180, 80, 255,
        100, 160, 60, 255
    };

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, grassData);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Создаем VAO для земли
    unsigned int groundVAO, groundVBO, groundEBO;
    float groundVertices[] = {
        -2.0f, 0.0f, -2.0f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
         2.0f, 0.0f, -2.0f,  0.0f, 1.0f, 0.0f,  40.0f, 0.0f,
         2.0f, 0.0f,  2.0f,  0.0f, 1.0f, 0.0f,  40.0f, 40.0f,
        -2.0f, 0.0f,  2.0f,  0.0f, 1.0f, 0.0f,  0.0f, 40.0f
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

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    std::cout << "\n=== Scene setup complete! Starting render loop... ===" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  W/A/S/D - Move camera" << std::endl;
    std::cout << "  Space/Shift - Move up/down" << std::endl;
    std::cout << "  Up/Down arrows - Adjust light intensity" << std::endl;
    std::cout << "  L - Toggle light movement" << std::endl;
    std::cout << "  ESC - Exit" << std::endl;
    std::cout << "\nScene composition:" << std::endl;
    std::cout << "  - Castle in the center" << std::endl;
    std::cout << "  - Large fairy floating to the right of castle" << std::endl;
    std::cout << "  - Small dog in front of fairy" << std::endl;
    std::cout << "  - Pony to the right of fairy" << std::endl;
    std::cout << "  - Sweet flying above" << std::endl;

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

        glClearColor(0.5f, 0.7f, 0.9f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        // Матрицы вида и проекции
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
            (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        shader.setVec3("viewPos", camera.Position);

        // ОСВЕЩЕНИЕ
        glm::vec3 lightPos = glm::vec3(
            10.0f * cos(lightAngle),
            15.0f,
            10.0f * sin(lightAngle)
        );
        shader.setVec3("light.position", lightPos);
        shader.setVec3("light.ambient", 0.5f * lightIntensity, 0.5f * lightIntensity, 0.5f * lightIntensity);
        shader.setVec3("light.diffuse", 0.9f * lightIntensity, 0.9f * lightIntensity, 0.9f * lightIntensity);
        shader.setVec3("light.specular", 1.0f * lightIntensity, 1.0f * lightIntensity, 1.0f * lightIntensity);
        shader.setFloat("light.constant", 1.0f);
        shader.setFloat("light.linear", 0.007f);
        shader.setFloat("light.quadratic", 0.0002f);

        shader.setFloat("material.shininess", 32.0f);
        shader.setBool("material.useTextures", true);

        // 1. РИСУЕМ ЗЕМЛЮ
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, groundLevel - 0.1f, 0.0f));
        model = glm::scale(model, glm::vec3(40.0f, 0.1f, 40.0f));
        shader.setMat4("model", model);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, groundTexture);
        shader.setInt("material.texture_diffuse1", 0);
        shader.setInt("material.texture_specular1", 0);

        glBindVertexArray(groundVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // 2. РИСУЕМ ЗАМОК
        model = glm::mat4(1.0f);
        model = glm::translate(model, ruinsPos);
        model = glm::scale(model, ruinsScale);
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        shader.setMat4("model", model);
        ruins.Draw(shader);

        // 3. РИСУЕМ ФЕЮ (ОЧЕНЬ БОЛЬШАЯ, ПОДНЯТА ВЫШЕ, РЯДОМ С ЗАМКОМ)
        model = glm::mat4(1.0f);
        model = glm::translate(model, fairyPos);
        // Фея слегка парит (легкое покачивание)
        model = glm::translate(model, glm::vec3(0.0f, sin(currentFrame * 0.5f) * 0.2f, 0.0f));
        // Фея слегка поворачивается, глядя на замок
        model = glm::rotate(model, glm::radians(-45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, fairyScale);
        shader.setMat4("model", model);
        fairy.Draw(shader);

        // 4. РИСУЕМ СОБАКУ (УМЕНЬШЕНА И ПЕРЕДВИНУТА ВПЕРЕД)
        model = glm::mat4(1.0f);
        model = glm::translate(model, dogPos);
        // Собака смотрит назад (на фею и замок)
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, dogScale);
        shader.setMat4("model", model);
        dog.Draw(shader);

        // 5. РИСУЕМ ПОНИ (СПРАВА ОТ ФЕИ)
        model = glm::mat4(1.0f);
        model = glm::translate(model, ponyPos);
        // Пони смотрит на центр сцены (замок и фею)
        model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, ponyScale);
        shader.setMat4("model", model);
        pony.Draw(shader);

        // 6. РИСУЕМ SWEET (летает над всей сценой)
        model = glm::mat4(1.0f);
        glm::vec3 sweetOrbit = glm::vec3(
            sin(currentFrame * 0.5f) * 8.0f,
            5.0f + sin(currentFrame * 0.7f) * 2.0f,
            cos(currentFrame * 0.5f) * 8.0f
        );
        model = glm::translate(model, sweetOrbit);
        model = glm::rotate(model, currentFrame * 2.0f, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, sweetScale);
        shader.setMat4("model", model);
        sweet.Draw(shader);

        // Обновление заголовка
        std::string title = "Magical Scene - Fairy with Small Dog - Light: " +
            std::to_string(lightIntensity).substr(0, 3) +
            " - Light Moving: " + (lightMoving ? "ON" : "OFF");
        glfwSetWindowTitle(window, title.c_str());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    std::cout << "\n=== Program finished successfully! ===" << std::endl;
    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

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

    // Регулировка света
    static float lastLightChange = 0.0f;
    float currentTime = static_cast<float>(glfwGetTime());

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