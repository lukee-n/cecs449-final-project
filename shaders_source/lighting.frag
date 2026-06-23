#version 330
// A fragment shader for rendering fragments in the Phong reflection model.
// 
// Referenced "Textures and Lighting" slides mostly,
// as well as https://learnopengl.com/PBR/Lighting, https://learnopengl.com/Lighting/Lighting-maps
layout (location=0) out vec4 FragColor;

// Inputs: the texture coordinates, world-space normal, and world-space position
// of this fragment, interpolated between its vertices.
in vec2 TexCoord;
in vec3 Normal;
in vec3 FragWorldPos;

// Textures
uniform sampler2D baseTexture;      // The mesh's base (diffuse) texture.
uniform sampler2D emissiveTexture;  // glowing
uniform sampler2D specMap;          // shininess

// Material parameters: k_a, k_d, k_s, shininess
uniform vec4 material; 

// Lighting
uniform vec3 ambientColor;

uniform vec3 directionalLight; // this is the "I" vector, not the "L" vector.
uniform vec3 directionalColor;

const int MAX_POINT_LIGHTS = 8;  // may need to change this later idk if ill add more lights
uniform int numPointLights;
uniform vec3 pointLightPos[MAX_POINT_LIGHTS];
uniform vec3 pointLightColor[MAX_POINT_LIGHTS];

// Flags
uniform int hasEmission;
uniform int hasSpecMap;

// Emission
uniform float emissiveStrength;

// Camera
uniform vec3 cameraPos;  // viewPos


void main() {
    vec4 baseColor = texture(baseTexture, TexCoord);
    if (baseColor.a < 0.02)  // alpha cutoff (tall grass fix)
        discard;
    
    // Common vectors
    vec3 N = normalize(Normal);                    // normal
    vec3 V = normalize(cameraPos - FragWorldPos);  // view/eye direction

    // Shininess
    float specStrength = (hasSpecMap == 1) 
        ? material.z * texture(specMap, TexCoord).r   // controlled by specular map
        : 0.05;                                       // default value
    
    // --------------------
    // Ambient light
    //---------------------
    vec3 ambientIntensity = material.x * ambientColor;

    // --------------------
    // Directional light
    //---------------------
    vec3 dirLightDir = normalize(-directionalLight); 

    // Diffuse
    float dirLambertFactor = max(dot(N, dirLightDir), 0.0);
    vec3 dirDiffuseIntensity = material.y * directionalColor * dirLambertFactor;

    // Specular
    vec3 dirSpecularIntensity = vec3(0.0);  

    if (dirLambertFactor > 0) {
        vec3 dirReflect = normalize(reflect(-dirLightDir, N));
        float dirSpecularFactor = max(dot(dirReflect, V), 0.0);

        dirSpecularIntensity = specStrength * directionalColor * pow(dirSpecularFactor, material.w);
    }

    // --------------------
    // Point light
    //---------------------
    vec3 totalPointDiffuseIntensity = vec3(0.0);
    vec3 totalPointSpecularIntensity = vec3(0.0);

    int lightCount = min(numPointLights, MAX_POINT_LIGHTS);

    // Iterate light sources
    for (int i = 0; i < lightCount; ++i) {
        // Diffuse
        vec3 pointLightDir = normalize(pointLightPos[i] - FragWorldPos);
        float pointLambertFactor = max(dot(N, pointLightDir), 0.0);
        vec3 pointDiffuseIntensity = material.y * pointLightColor[i] * pointLambertFactor;

        // Specular
        vec3 pointSpecularIntensity = vec3(0.0);

        if (pointLambertFactor > 0) {
            vec3 pointReflect = normalize(reflect(-pointLightDir, N));
            float pointSpecularFactor = max(dot(pointReflect, V), 0.0);

            pointSpecularIntensity = specStrength * pointLightColor[i] * pow(pointSpecularFactor, material.w);
        }
        // Attenuation
        float distance = length(pointLightPos[i] - FragWorldPos);
    
        float constant = 1.0;   // tune these
        float linear = 1.4;     //
        float quadratic = 2.0;  //

        float attenuation = 1.0 / (
            constant + 
            linear * distance + 
            quadratic * distance * distance
        );

        totalPointDiffuseIntensity += pointDiffuseIntensity * attenuation;
        totalPointSpecularIntensity += pointSpecularIntensity * attenuation;
    }

    
    // --------------------
    // Emission
    //---------------------
    vec3 emissiveColor = (hasEmission == 1) 
        ? texture(emissiveTexture, TexCoord).rgb * emissiveStrength
        : vec3(0.0);

    // --------------------
    // Final color
    //---------------------
    vec3 diffuseLighting = ambientIntensity + dirDiffuseIntensity + totalPointDiffuseIntensity;
    vec3 specularLighting = dirSpecularIntensity + totalPointSpecularIntensity;

    vec3 finalColor = 
        baseColor.rgb * diffuseLighting +
        specularLighting +
        emissiveColor;

    FragColor = vec4(finalColor, baseColor.a);
}

