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

// Три типа освещения
bool dirLightEnabled = true;      // Направленный свет (солнце)
bool pointLightEnabled = true;    // Точечный свет (лампочка)
bool spotLightEnabled = true;     // Прожекторный свет (фонарик)

// Параметры прожектора
float spotLightCutOff = glm::cos(glm::radians(12.5f));      // Внутренний угол конуса
float spotLightOuterCutOff = glm::cos(glm::radians(17.5f)); // Внешний угол конуса

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

        // НАСТРОЙКА ОСВЕЩЕНИЯ
        shader.use();

        // 1. НАПРАВЛЕННЫЙ СВЕТ (Солнце/Луна) - бесконечно далеко, параллельные лучи
        shader.setBool("dirLight.enabled", dirLightEnabled);
        shader.setVec3("dirLight.direction", glm::vec3(-0.2f, -1.0f, -0.3f));
        shader.setVec3("dirLight.ambient", 0.2f * lightIntensity, 0.2f * lightIntensity, 0.2f * lightIntensity);
        shader.setVec3("dirLight.diffuse", 0.5f * lightIntensity, 0.5f * lightIntensity, 0.5f * lightIntensity);
        shader.setVec3("dirLight.specular", 0.5f * lightIntensity, 0.5f * lightIntensity, 0.5f * lightIntensity);

        // 2. ТОЧЕЧНЫЙ СВЕТ (Лампочка) - светит из точки во все стороны
        shader.setBool("pointLight.enabled", pointLightEnabled);
        glm::vec3 pointLightPos = glm::vec3(
            5.0f * cos(currentFrame * 0.5f),
            8.0f,
            5.0f * sin(currentFrame * 0.5f)
        );
        shader.setVec3("pointLight.position", pointLightPos);
        shader.setVec3("pointLight.ambient", 0.1f * lightIntensity, 0.1f * lightIntensity, 0.1f * lightIntensity);
        shader.setVec3("pointLight.diffuse", 0.8f * lightIntensity, 0.8f * lightIntensity, 0.8f * lightIntensity);
        shader.setVec3("pointLight.specular", 1.0f * lightIntensity, 1.0f * lightIntensity, 1.0f * lightIntensity);
        shader.setFloat("pointLight.constant", 1.0f);
        shader.setFloat("pointLight.linear", 0.09f);
        shader.setFloat("pointLight.quadratic", 0.032f);

        // 3. ПРОЖЕКТОРНЫЙ СВЕТ (Фонарик) - конус света, следует за камерой
        shader.setBool("spotLight.enabled", spotLightEnabled);
        shader.setVec3("spotLight.position", camera.Position);
        shader.setVec3("spotLight.direction", camera.Front);
        shader.setFloat("spotLight.cutOff", spotLightCutOff);
        shader.setFloat("spotLight.outerCutOff", spotLightOuterCutOff);
        shader.setVec3("spotLight.ambient", 0.1f * lightIntensity, 0.1f * lightIntensity, 0.1f * lightIntensity);
        shader.setVec3("spotLight.diffuse", 1.0f * lightIntensity, 1.0f * lightIntensity, 1.0f * lightIntensity);
        shader.setVec3("spotLight.specular", 1.0f * lightIntensity, 1.0f * lightIntensity, 1.0f * lightIntensity);
        shader.setFloat("spotLight.constant", 1.0f);
        shader.setFloat("spotLight.linear", 0.09f);
        shader.setFloat("spotLight.quadratic", 0.032f);

        // Материал
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
        std::string title = "3 Light Types - Dir[" + std::to_string(dirLightEnabled) +
            "] Point[" + std::to_string(pointLightEnabled) +
            "] Spot[" + std::to_string(spotLightEnabled) +
            "] - CutOff: " + std::to_string(spotLightCutOff).substr(0, 4);
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

    // Управление тремя типами освещения (с задержкой 0.3 секунды)
    if (currentTime - lastKeyPress > 0.3f) {
        // 1. Направленный свет (Солнце/Луна) - клавиша 1
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
            dirLightEnabled = !dirLightEnabled;
            std::cout << "=== Directional Light ===" << std::endl;
            std::cout << "Status: " << (dirLightEnabled ? "ENABLED" : "DISABLED") << std::endl;
            std::cout << "Type: Sun/Moon (infinite distance, parallel rays)" << std::endl;
            lastKeyPress = currentTime;
        }

        // 2. Точечный свет (Лампочка) - клавиша 2
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
            pointLightEnabled = !pointLightEnabled;
            std::cout << "=== Point Light ===" << std::endl;
            std::cout << "Status: " << (pointLightEnabled ? "ENABLED" : "DISABLED") << std::endl;
            std::cout << "Type: Light Bulb (emits in all directions)" << std::endl;
            lastKeyPress = currentTime;
        }

        // 3. Прожекторный свет (Фонарик) - клавиша 3
        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
            spotLightEnabled = !spotLightEnabled;
            std::cout << "=== Spot Light ===" << std::endl;
            std::cout << "Status: " << (spotLightEnabled ? "ENABLED" : "DISABLED") << std::endl;
            std::cout << "Type: Flashlight (cone-shaped, follows camera)" << std::endl;
            lastKeyPress = currentTime;
        }

        // 4. Увеличить угол прожектора - клавиша 4
        if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) {
            spotLightCutOff += 0.02f;
            spotLightCutOff = std::min(spotLightCutOff, 0.99f);
            float angle = glm::degrees(glm::acos(spotLightCutOff));
            std::cout << "Spotlight inner angle: " << angle << " degrees" << std::endl;
            lastKeyPress = currentTime;
        }

        // 5. Уменьшить угол прожектора - клавиша 5
        if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS) {
            spotLightCutOff -= 0.02f;
            spotLightCutOff = std::max(spotLightCutOff, 0.1f);
            float angle = glm::degrees(glm::acos(spotLightCutOff));
            std::cout << "Spotlight inner angle: " << angle << " degrees" << std::endl;
            lastKeyPress = currentTime;
        }

        // 6. Увеличить внешний угол прожектора - клавиша 6
        if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS) {
            spotLightOuterCutOff += 0.02f;
            spotLightOuterCutOff = std::min(spotLightOuterCutOff, 0.99f);
            float angle = glm::degrees(glm::acos(spotLightOuterCutOff));
            std::cout << "Spotlight outer angle: " << angle << " degrees" << std::endl;
            lastKeyPress = currentTime;
        }

        // 7. Уменьшить внешний угол прожектора - клавиша 7
        if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS) {
            spotLightOuterCutOff -= 0.02f;
            spotLightOuterCutOff = std::max(spotLightOuterCutOff, 0.1f);
            float angle = glm::degrees(glm::acos(spotLightOuterCutOff));
            std::cout << "Spotlight outer angle: " << angle << " degrees" << std::endl;
            lastKeyPress = currentTime;
        }

        // 8. Все источники света ВКЛ/ВЫКЛ - клавиша 0
        if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS) {
            bool allEnabled = !(dirLightEnabled && pointLightEnabled && spotLightEnabled);
            dirLightEnabled = allEnabled;
            pointLightEnabled = allEnabled;
            spotLightEnabled = allEnabled;
            std::cout << "All lights: " << (allEnabled ? "ENABLED" : "DISABLED") << std::endl;
            lastKeyPress = currentTime;
        }

        // 9. Цикл режимов освещения - клавиша 8
        static int lightMode = 0;
        if (glfwGetKey(window, GLFW_KEY_8) == GLFW_PRESS) {
            lightMode = (lightMode + 1) % 4;

            switch (lightMode) {
            case 0: // Все включены
                dirLightEnabled = true;
                pointLightEnabled = true;
                spotLightEnabled = true;
                std::cout << "Light mode: ALL LIGHTS" << std::endl;
                break;
            case 1: // Только дневной свет
                dirLightEnabled = true;
                pointLightEnabled = false;
                spotLightEnabled = false;
                std::cout << "Light mode: DAYLIGHT ONLY" << std::endl;
                break;
            case 2: // Только искусственный свет
                dirLightEnabled = false;
                pointLightEnabled = true;
                spotLightEnabled = false;
                std::cout << "Light mode: ARTIFICIAL LIGHT ONLY" << std::endl;
                break;
            case 3: // Только фонарик
                dirLightEnabled = false;
                pointLightEnabled = false;
                spotLightEnabled = true;
                std::cout << "Light mode: FLASHLIGHT ONLY" << std::endl;
                break;
            }
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

            camera.Position = glm::vec3(0.0f, 8.0f, 15.0f);
            camera.Yaw = -90.0f;
            camera.Pitch = 0.0f;
            camera.ProcessMouseMovement(0, 0);

            std::cout << "=== ALL SETTINGS RESET ===" << std::endl;
            std::cout << "- Lights reset to default" << std::endl;
            std::cout << "- Camera reset to initial position" << std::endl;
            lastKeyPress = currentTime;
        }

        // T. Включить/выключить текстуры - клавиша T
        static bool texturesEnabled = true;
        if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
            texturesEnabled = !texturesEnabled;
            std::cout << "Textures: " << (texturesEnabled ? "ENABLED" : "DISABLED") << std::endl;
            lastKeyPress = currentTime;
        }

        // P. Пауза/возобновление движения света - клавиша P
        if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
            lightMoving = !lightMoving;
            std::cout << "Light movement: " << (lightMoving ? "RESUMED" : "PAUSED") << std::endl;
            lastKeyPress = currentTime;
        }

        // M. Показать справку по управлению - клавиша M
        if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
            std::cout << "\n=== CONTROLS ===" << std::endl;
            std::cout << "WASD + Space/Shift: Move camera" << std::endl;
            std::cout << "Mouse: Look around" << std::endl;
            std::cout << "Scroll: Zoom in/out" << std::endl;
            std::cout << "\n=== LIGHT CONTROLS ===" << std::endl;
            std::cout << "1: Toggle directional light (sun)" << std::endl;
            std::cout << "2: Toggle point light (bulb)" << std::endl;
            std::cout << "3: Toggle spot light (flashlight)" << std::endl;
            std::cout << "4/5: Adjust spot inner angle" << std::endl;
            std::cout << "6/7: Adjust spot outer angle" << std::endl;
            std::cout << "8: Cycle light modes" << std::endl;
            std::cout << "0: Toggle all lights" << std::endl;
            std::cout << "Up/Down: Adjust light intensity" << std::endl;
            std::cout << "L: Toggle light movement" << std::endl;
            std::cout << "T: Toggle textures" << std::endl;
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