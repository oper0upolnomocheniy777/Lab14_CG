#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

// Структуры материалов и света
struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
    float shininess;
    bool useTextures;
};

struct DirLight {
    bool enabled;
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    bool enabled;
    vec3 position;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    
    float constant;
    float linear;
    float quadratic;
};

struct SpotLight {
    bool enabled;
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    
    float constant;
    float linear;
    float quadratic;
};

uniform vec3 viewPos;
uniform Material material;

// Все три типа источников света
uniform DirLight dirLight;
uniform PointLight pointLight;
uniform SpotLight spotLight;

// Прототипы функций
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

void main()
{
    // Свойства
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    vec3 result = vec3(0.0, 0.0, 0.0);
    
    // Направленный свет (солнце/луна)
    if (dirLight.enabled) {
        result += CalcDirLight(dirLight, norm, viewDir);
    }
    
    // Точечный свет (лампочка)
    if (pointLight.enabled) {
        result += CalcPointLight(pointLight, norm, FragPos, viewDir);
    }
    
    // Прожекторный свет (фонарик)
    if (spotLight.enabled) {
        result += CalcSpotLight(spotLight, norm, FragPos, viewDir);
    }
    
    FragColor = vec4(result, 1.0);
}

// Расчет направленного света
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    
    // Диффузное затенение
    float diff = max(dot(normal, lightDir), 0.0);
    
    // Отраженное затенение
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    
    // Объединяем результаты
    vec3 ambient, diffuse, specular;
    
    if (material.useTextures) {
        ambient = light.ambient * vec3(texture(material.texture_diffuse1, TexCoords));
        diffuse = light.diffuse * diff * vec3(texture(material.texture_diffuse1, TexCoords));
        specular = light.specular * spec * vec3(texture(material.texture_specular1, TexCoords));
    } else {
        ambient = light.ambient * vec3(0.5, 0.5, 0.5);
        diffuse = light.diffuse * diff * vec3(0.8, 0.8, 0.8);
        specular = light.specular * spec * vec3(1.0, 1.0, 1.0);
    }
    
    return (ambient + diffuse + specular);
}

// Расчет точечного света
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    
    // Диффузное затенение
    float diff = max(dot(normal, lightDir), 0.0);
    
    // Отраженное затенение
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    
    // Затухание
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    
    // Объединяем результаты
    vec3 ambient, diffuse, specular;
    
    if (material.useTextures) {
        ambient = light.ambient * vec3(texture(material.texture_diffuse1, TexCoords));
        diffuse = light.diffuse * diff * vec3(texture(material.texture_diffuse1, TexCoords));
        specular = light.specular * spec * vec3(texture(material.texture_specular1, TexCoords));
    } else {
        ambient = light.ambient * vec3(0.5, 0.5, 0.5);
        diffuse = light.diffuse * diff * vec3(0.8, 0.8, 0.8);
        specular = light.specular * spec * vec3(1.0, 1.0, 1.0);
    }
    
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    
    return (ambient + diffuse + specular);
}

// Расчет прожекторного света
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    
    // Проверка угла конуса
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    
    // Диффузное затенение
    float diff = max(dot(normal, lightDir), 0.0);
    
    // Отраженное затенение
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    
    // Затухание
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    
    // Объединяем результаты
    vec3 ambient, diffuse, specular;
    
    if (material.useTextures) {
        ambient = light.ambient * vec3(texture(material.texture_diffuse1, TexCoords));
        diffuse = light.diffuse * diff * vec3(texture(material.texture_diffuse1, TexCoords));
        specular = light.specular * spec * vec3(texture(material.texture_specular1, TexCoords));
    } else {
        ambient = light.ambient * vec3(0.5, 0.5, 0.5);
        diffuse = light.diffuse * diff * vec3(0.8, 0.8, 0.8);
        specular = light.specular * spec * vec3(1.0, 1.0, 1.0);
    }
    
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    
    return (ambient + diffuse + specular);
}