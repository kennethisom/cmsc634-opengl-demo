// terrain data and drawing
#ifndef Terrain_hpp
#define Terrain_hpp

#define GLM_SWIZZLE

#include "Shader.hpp"
#include <glm/glm.hpp>

// terrain data and rendering methods
class Terrain {
// private data
private:
    glm::vec3 gridSize;             // elevation grid size
    glm::vec3 mapSize;              // size of terrain in world space

    unsigned int numvert;       // total vertices
    glm::vec3 *vert;                // per-vertex position
    glm::vec3 *dPdu, *dPdv;         // per-vertex tangents
    glm::vec3 *norm;                // per-vertex normal
    glm::vec2 *texcoord;            // per-vertex texture coordinate
    
    unsigned int numtri;        // total triangles
    glm::uvec3 *indices; // 3 vertex indices per triangle

    // GL vertex array object IDs
    unsigned int varrayID;

    // GL texture IDs
    enum {COLOR_TEXTURE, NORMAL_TEXTURE, GLOSS_TEXTURE, NUM_TEXTURES};
    unsigned int textureIDs[NUM_TEXTURES];

    // GL buffer object IDs
    enum {POSITION_BUFFER, TANGENT_BUFFER, BITANGENT_BUFFER, NORMAL_BUFFER, 
          UV_BUFFER, INDEX_BUFFER, NUM_BUFFERS};
    unsigned int bufferIDs[NUM_BUFFERS];

    // GL shaders
    unsigned int shaderID;      // ID for shader program
    ShaderInfo shaderParts[2];  // vertex & fragment shader info

// public methods
public:
    // load terrain, given elevation image and surface texture
    Terrain(const char *elevationPPM, const char *texturePPM,
            const char *normalPPM, const char *glossPPM);

    // clean up allocated memory
    ~Terrain();

    // load/reload a texture
    void updateTexture(const char *ppm, unsigned int textureID);

    // load/reload shaders
    void updateShaders();

    // draw this terrain object
    void draw() const;
};

#endif
