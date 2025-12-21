#version 330 core
out vec4 FragColor;

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
    float shininess;
    bool useTextures;
}; 

struct Light {
    vec3 position;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    
    float constant;
    float linear;
    float quadratic;
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 viewPos;
uniform Material material;
uniform Light light;

void main()
{
    // ambient
    vec3 ambient;
    if (material.useTextures) {
        ambient = light.ambient * vec3(texture(material.texture_diffuse1, TexCoords));
    } else {
        ambient = light.ambient * vec3(0.5, 0.5, 0.5);
    }
    
    // diffuse 
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse;
    if (material.useTextures) {
        diffuse = light.diffuse * diff * vec3(texture(material.texture_diffuse1, TexCoords));
    } else {
        diffuse = light.diffuse * diff * vec3(0.8, 0.8, 0.8);
    }
    
    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular;
    if (material.useTextures) {
        specular = light.specular * spec * vec3(texture(material.texture_specular1, TexCoords));
    } else {
        specular = light.specular * spec * vec3(1.0, 1.0, 1.0);
    }
    
    // attenuation
    float distance = length(light.position - FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    
    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}