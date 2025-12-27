#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

// Текстуры
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

// Освещение - ТАКИЕ ЖЕ СТРУКТУРЫ
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
uniform float minnaertPower; // Дополнительный параметр для Minnaert

uniform vec3 viewPos;

void main()
{
    // Цвет из текстуры
    vec3 color = material_useTextures ? 
        texture(texture_diffuse1, TexCoords).rgb : vec3(0.8, 0.8, 0.8);
    
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    vec3 result = vec3(0.0);
    
    // 1. НАПРАВЛЕННЫЙ СВЕТ (Minnaert)
    if (dirLight.enabled) {
        vec3 lightDir = normalize(-dirLight.direction);
        float NdotL = max(dot(norm, lightDir), 0.0);
        float NdotV = max(dot(norm, viewDir), 0.0);
        
        // Формула Minnaert
        float minnaert = pow(NdotL, minnaertPower) * pow(NdotV, minnaertPower - 1.0);
        minnaert = clamp(minnaert, 0.0, 1.0);
        
        vec3 ambient = dirLight.ambient * color;
        vec3 diffuse = dirLight.diffuse * minnaert * color;
        
        result += ambient + diffuse;
    }
    
    // 2. ТОЧЕЧНЫЙ СВЕТ
    if (pointLight.enabled) {
        vec3 lightDir = normalize(pointLight.position - FragPos);
        float NdotL = max(dot(norm, lightDir), 0.0);
        float NdotV = max(dot(norm, viewDir), 0.0);
        
        // Формула Minnaert
        float minnaert = pow(NdotL, minnaertPower) * pow(NdotV, minnaertPower - 1.0);
        minnaert = clamp(minnaert, 0.0, 1.0);
        
        // Затухание
        float distance = length(pointLight.position - FragPos);
        float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
        
        vec3 ambient = pointLight.ambient * color;
        vec3 diffuse = pointLight.diffuse * minnaert * color;
        
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
        
        float NdotL = max(dot(norm, lightDir), 0.0) * intensity;
        float NdotV = max(dot(norm, viewDir), 0.0);
        
        // Формула Minnaert
        float minnaert = pow(NdotL, minnaertPower) * pow(NdotV, minnaertPower - 1.0);
        minnaert = clamp(minnaert, 0.0, 1.0);
        
        // Затухание
        float distance = length(spotLight.position - FragPos);
        float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
        
        vec3 ambient = spotLight.ambient * color;
        vec3 diffuse = spotLight.diffuse * minnaert * color;
        
        ambient *= attenuation * intensity;
        diffuse *= attenuation * intensity;
        
        result += ambient + diffuse;
    }
    
    FragColor = vec4(result, 1.0);
}