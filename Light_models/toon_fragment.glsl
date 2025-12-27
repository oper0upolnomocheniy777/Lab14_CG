#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

// Текстуры
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

// Освещение - ТАКИЕ ЖЕ СТРУКТУРЫ КАК В PHONG
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
    // Цвет из текстуры
    vec3 color = material_useTextures ? 
        texture(texture_diffuse1, TexCoords).rgb : vec3(0.8, 0.8, 0.8);
    
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    vec3 result = vec3(0.0);
    
    // 1. НАПРАВЛЕННЫЙ СВЕТ (Toon)
    if (dirLight.enabled) {
        vec3 lightDir = normalize(-dirLight.direction);
        float diff = max(dot(norm, lightDir), 0.0);
        
        // Toon эффект - дискретные уровни
        if (diff > 0.75) diff = 1.0;
        else if (diff > 0.5) diff = 0.75;
        else if (diff > 0.25) diff = 0.5;
        else diff = 0.25;
        
        // Обводка (rim lighting)
        float rim = 1.0 - max(dot(viewDir, norm), 0.0);
        if (rim > 0.7) rim = 1.0;
        else if (rim > 0.4) rim = 0.6;
        else rim = 0.0;
        
        vec3 ambient = dirLight.ambient * color;
        vec3 diffuse = dirLight.diffuse * diff * color;
        vec3 rimColor = vec3(0.2, 0.2, 0.3) * rim;
        
        result += ambient + diffuse + rimColor;
    }
    
    // 2. ТОЧЕЧНЫЙ СВЕТ
    if (pointLight.enabled) {
        vec3 lightDir = normalize(pointLight.position - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        
        // Toon эффект
        if (diff > 0.75) diff = 1.0;
        else if (diff > 0.5) diff = 0.75;
        else if (diff > 0.25) diff = 0.5;
        else diff = 0.25;
        
        // Затухание
        float distance = length(pointLight.position - FragPos);
        float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
        
        vec3 ambient = pointLight.ambient * color;
        vec3 diffuse = pointLight.diffuse * diff * color;
        
        ambient *= attenuation;
        diffuse *= attenuation;
        
        result += ambient + diffuse;
    }
    
    // 3. ПРОЖЕКТОР
    if (spotLight.enabled) {
        vec3 lightDir = normalize(spotLight.position - FragPos);
        
        // Угол конуса
        float theta = dot(lightDir, normalize(-spotLight.direction));
        float epsilon = spotLight.cutOff - spotLight.outerCutOff;
        float intensity = clamp((theta - spotLight.outerCutOff) / epsilon, 0.0, 1.0);
        
        float diff = max(dot(norm, lightDir), 0.0) * intensity;
        
        // Toon эффект
        if (diff > 0.75) diff = 1.0;
        else if (diff > 0.5) diff = 0.75;
        else if (diff > 0.25) diff = 0.5;
        else diff = 0.25;
        
        // Затухание
        float distance = length(spotLight.position - FragPos);
        float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
        
        vec3 ambient = spotLight.ambient * color;
        vec3 diffuse = spotLight.diffuse * diff * color;
        
        ambient *= attenuation * intensity;
        diffuse *= attenuation * intensity;
        
        result += ambient + diffuse;
    }
    
    FragColor = vec4(result, 1.0);
}