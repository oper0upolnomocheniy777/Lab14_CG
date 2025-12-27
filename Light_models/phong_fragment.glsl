#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

// Текстуры
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

// Освещение - УПРОЩЕННЫЕ СТРУКТУРЫ
struct Light {
    vec3 position;
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float cutOff;
    float outerCutOff;
    bool enabled;
};

uniform Light dirLight;
uniform Light pointLight;
uniform Light spotLight;

// Материал
uniform float material_shininess;
uniform bool material_useTextures;

uniform vec3 viewPos;

void main()
{
    // Цвет из текстуры или по умолчанию
    vec3 diffuseColor = material_useTextures ? 
        texture(texture_diffuse1, TexCoords).rgb : vec3(0.8, 0.8, 0.8);
    vec3 specularColor = material_useTextures ? 
        texture(texture_specular1, TexCoords).rgb : vec3(0.5, 0.5, 0.5);
    
    // Нормализация
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    vec3 result = vec3(0.0);
    
    // 1. НАПРАВЛЕННЫЙ СВЕТ (Phong)
    if (dirLight.enabled) {
        vec3 lightDir = normalize(-dirLight.direction);
        
        // Диффузное
        float diff = max(dot(norm, lightDir), 0.0);
        
        // Отраженное (Phong)
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), material_shininess);
        
        vec3 ambient = dirLight.ambient * diffuseColor;
        vec3 diffuse = dirLight.diffuse * diff * diffuseColor;
        vec3 specular = dirLight.specular * spec * specularColor;
        
        result += ambient + diffuse + specular;
    }
    
    // 2. ТОЧЕЧНЫЙ СВЕТ
    if (pointLight.enabled) {
        vec3 lightDir = normalize(pointLight.position - FragPos);
        
        // Диффузное
        float diff = max(dot(norm, lightDir), 0.0);
        
        // Отраженное
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), material_shininess);
        
        // Затухание (упрощенное)
        float distance = length(pointLight.position - FragPos);
        float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
        
        vec3 ambient = pointLight.ambient * diffuseColor;
        vec3 diffuse = pointLight.diffuse * diff * diffuseColor;
        vec3 specular = pointLight.specular * spec * specularColor;
        
        ambient *= attenuation;
        diffuse *= attenuation;
        specular *= attenuation;
        
        result += ambient + diffuse + specular;
    }
    
    // 3. ПРОЖЕКТОР
    if (spotLight.enabled) {
        vec3 lightDir = normalize(spotLight.position - FragPos);
        
        // Угол конуса
        float theta = dot(lightDir, normalize(-spotLight.direction));
        float epsilon = spotLight.cutOff - spotLight.outerCutOff;
        float intensity = clamp((theta - spotLight.outerCutOff) / epsilon, 0.0, 1.0);
        
        // Диффузное
        float diff = max(dot(norm, lightDir), 0.0);
        
        // Отраженное
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), material_shininess);
        
        // Затухание
        float distance = length(spotLight.position - FragPos);
        float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
        
        vec3 ambient = spotLight.ambient * diffuseColor;
        vec3 diffuse = spotLight.diffuse * diff * diffuseColor;
        vec3 specular = spotLight.specular * spec * specularColor;
        
        ambient *= attenuation * intensity;
        diffuse *= attenuation * intensity;
        specular *= attenuation * intensity;
        
        result += ambient + diffuse + specular;
    }
    
    FragColor = vec4(result, 1.0);
}