cmake_minimum_required(VERSION 3.10)
project(OpenGLLighting)

set(CMAKE_CXX_STANDARD 17)

# Поиск GLFW
find_package(glfw3 3.3 REQUIRED)

# Поиск OpenGL
find_package(OpenGL REQUIRED)

# Настройки компиляции
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

# Включаем поддержку filesystem
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Исходные файлы
set(SOURCES
    main.cpp
    Shader.cpp
    Camera.cpp
    Model.cpp
    glad.c
)

# Создание исполняемого файла
add_executable(${PROJECT_NAME} ${SOURCES})

# Подключение библиотек
target_link_libraries(${PROJECT_NAME} glfw)
target_link_libraries(${PROJECT_NAME} OpenGL::GL)

# Копирование ресурсов
file(COPY models DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY textures DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY shaders DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Добавляем пути для include файлов
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders
    ${CMAKE_CURRENT_SOURCE_DIR}/models
    ${CMAKE_CURRENT_SOURCE_DIR}/textures
)