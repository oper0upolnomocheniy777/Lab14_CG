#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

// Текстуры
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

// Освещение
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
    float minnaertPower; // Параметр Minnaert
};

uniform vec3 viewPos;
uniform DirLight dirLight;
uniform PointLight pointLight;
uniform SpotLight spotLight;
uniform Material material;

// Функции освещения Minnaert
vec3 CalcDirLightMinnaert(DirLight light, vec3 normal, vec3 viewDir, vec3 diffuseColor);
vec3 CalcPointLightMinnaert(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffuseColor);
vec3 CalcSpotLightMinnaert(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffuseColor);

void main()
{
    vec3 diffuseColor = material.useTextures ? 
        texture(material.texture_diffuse1, TexCoords).rgb : vec3(0.8, 0.8, 0.8);
    
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    vec3 result = vec3(0.0);
    
    if (dirLight.enabled)
        result += CalcDirLightMinnaert(dirLight, norm, viewDir, diffuseColor);
    
    if (pointLight.enabled)
        result += CalcPointLightMinnaert(pointLight, norm, FragPos, viewDir, diffuseColor);
    
    if (spotLight.enabled)
        result += CalcSpotLightMinnaert(spotLight, norm, FragPos, viewDir, diffuseColor);
    
    FragColor = vec4(result, 1.0);
}

// Модель Minnaert (для матовых поверхностей)
vec3 CalcDirLightMinnaert(DirLight light, vec3 normal, vec3 viewDir, vec3 diffuseColor)
{
    vec3 lightDir = normalize(-light.direction);
    float NdotL = max(dot(normal, lightDir), 0.0);
    float NdotV = max(dot(normal, viewDir), 0.0);
    
    // Формула Minnaert: (N·L)^k * (N·V)^(k-1)
    float k = material.minnaertPower;
    float minnaert = pow(NdotL, k) * pow(NdotV, k - 1.0);
    minnaert = clamp(minnaert, 0.0, 1.0);
    
    vec3 ambient = light.ambient * diffuseColor;
    vec3 diffuse = light.diffuse * minnaert * diffuseColor;
    
    return (ambient + diffuse);
}

vec3 CalcPointLightMinnaert(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffuseColor)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float NdotL = max(dot(normal, lightDir), 0.0);
    float NdotV = max(dot(normal, viewDir), 0.0);
    
    float k = material.minnaertPower;
    float minnaert = pow(NdotL, k) * pow(NdotV, k - 1.0);
    minnaert = clamp(minnaert, 0.0, 1.0);
    
    // Затухание
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + 
                               light.quadratic * (distance * distance));
    
    vec3 ambient = light.ambient * diffuseColor;
    vec3 diffuse = light.diffuse * minnaert * diffuseColor;
    
    ambient *= attenuation;
    diffuse *= attenuation;
    
    return (ambient + diffuse);
}

vec3 CalcSpotLightMinnaert(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffuseColor)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    
    float NdotL = max(dot(normal, lightDir), 0.0);
    float NdotV = max(dot(normal, viewDir), 0.0);
    
    float k = material.minnaertPower;
    float minnaert = pow(NdotL, k) * pow(NdotV, k - 1.0);
    minnaert = clamp(minnaert, 0.0, 1.0);
    
    // Затухание
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + 
                               light.quadratic * (distance * distance));
    
    vec3 ambient = light.ambient * diffuseColor;
    vec3 diffuse = light.diffuse * minnaert * diffuseColor;
    
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    
    return (ambient + diffuse);
}