#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

// Текстуры
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

// Освещение (те же структуры)
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

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
    float shininess;
    bool useTextures;
};

uniform vec3 viewPos;
uniform DirLight dirLight;
uniform PointLight pointLight;
uniform SpotLight spotLight;
uniform Material material;

// Функции освещения для Toon
vec3 CalcDirLightToon(DirLight light, vec3 normal, vec3 viewDir, vec3 diffuseColor);
vec3 CalcPointLightToon(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffuseColor);
vec3 CalcSpotLightToon(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffuseColor);

void main()
{
    vec3 diffuseColor = material.useTextures ? 
        texture(material.texture_diffuse1, TexCoords).rgb : vec3(0.8, 0.8, 0.8);
    
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    vec3 result = vec3(0.0);
    
    if (dirLight.enabled)
        result += CalcDirLightToon(dirLight, norm, viewDir, diffuseColor);
    
    if (pointLight.enabled)
        result += CalcPointLightToon(pointLight, norm, FragPos, viewDir, diffuseColor);
    
    if (spotLight.enabled)
        result += CalcSpotLightToon(spotLight, norm, FragPos, viewDir, diffuseColor);
    
    FragColor = vec4(result, 1.0);
}

// Toon освещение - дискретные уровни
vec3 CalcDirLightToon(DirLight light, vec3 normal, vec3 viewDir, vec3 diffuseColor)
{
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    
    // Дискретизация (4 уровня)
    if (diff > 0.75) diff = 1.0;
    else if (diff > 0.5) diff = 0.75;
    else if (diff > 0.25) diff = 0.5;
    else diff = 0.25;
    
    // Обводка (rim lighting)
    float rim = 1.0 - max(dot(viewDir, normal), 0.0);
    if (rim > 0.7) rim = 1.0;
    else if (rim > 0.4) rim = 0.6;
    else rim = 0.0;
    
    vec3 ambient = light.ambient * diffuseColor;
    vec3 diffuse = light.diffuse * diff * diffuseColor;
    vec3 rimColor = vec3(0.2, 0.2, 0.3) * rim;
    
    return (ambient + diffuse + rimColor);
}

vec3 CalcPointLightToon(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffuseColor)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    
    // Дискретизация
    if (diff > 0.75) diff = 1.0;
    else if (diff > 0.5) diff = 0.75;
    else if (diff > 0.25) diff = 0.5;
    else diff = 0.25;
    
    // Затухание
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + 
                               light.quadratic * (distance * distance));
    
    // Обводка
    float rim = 1.0 - max(dot(viewDir, normal), 0.0);
    if (rim > 0.7) rim = 1.0;
    else if (rim > 0.4) rim = 0.6;
    else rim = 0.0;
    
    vec3 ambient = light.ambient * diffuseColor;
    vec3 diffuse = light.diffuse * diff * diffuseColor;
    vec3 rimColor = vec3(0.2, 0.2, 0.3) * rim;
    
    ambient *= attenuation;
    diffuse *= attenuation;
    rimColor *= attenuation;
    
    return (ambient + diffuse + rimColor);
}

vec3 CalcSpotLightToon(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffuseColor)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    
    float diff = max(dot(normal, lightDir), 0.0);
    
    // Дискретизация
    if (diff > 0.75) diff = 1.0;
    else if (diff > 0.5) diff = 0.75;
    else if (diff > 0.25) diff = 0.5;
    else diff = 0.25;
    
    // Затухание
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + 
                               light.quadratic * (distance * distance));
    
    vec3 ambient = light.ambient * diffuseColor;
    vec3 diffuse = light.diffuse * diff * diffuseColor;
    
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    
    return (ambient + diffuse);
}