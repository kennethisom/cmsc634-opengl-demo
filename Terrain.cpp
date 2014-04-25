// draw a simple terrain height field

#include "Terrain.hpp"
#include "AppContext.hpp"
#include "ImagePPM.hpp"
#include "Vec.inl"

// using core modern OpenGL
#include <GL/glew.h>
#include <GLFW/glfw3.h>


//
// load the terrain data
//
Terrain::Terrain(const char *elevationPPM, const char *texturePPM,
                 const char *normalPPM, const char *glossPPM)
{
    // buffer objects to be used later
    glGenTextures(NUM_TEXTURES, textureIDs);
    glGenBuffers(NUM_BUFFERS, bufferIDs);
    glGenVertexArrays(1, &varrayID);

    // load albedo, normal & gloss image into a named textures
    ImagePPM(texturePPM).loadTexture(textureIDs[COLOR_TEXTURE]);
    ImagePPM(normalPPM).loadTexture(textureIDs[NORMAL_TEXTURE]);
    ImagePPM(glossPPM).loadTexture(textureIDs[GLOSS_TEXTURE]);

    // load terrain heights
    ImagePPM elevation(elevationPPM);
    unsigned int w = elevation.width, h = elevation.height;
    gridSize = vec3<float>(float(w), float(h), 255.f);

    // world dimensions
    mapSize = vec3<float>(512, 512, 50);

    // build vertex, normal and texture coordinate arrays
    // * x & y are the position in the terrain grid
    // * idx is the linear array index for each vertex
    numvert = (w + 1) * (h + 1);
    vert = new Vec3f[numvert];
    dPdu = new Vec3f[numvert];
    dPdv = new Vec3f[numvert];
    norm = new Vec3f[numvert];
    texcoord = new Vec2f[numvert];

    for(unsigned int y=0, idx=0;  y <= h;  ++y) {
        for(unsigned int x=0;  x <= w;  ++idx, ++x) {
            // 3d vertex location: x,y from grid location, z from terrain data
            vert[idx] = (vec3<float>(float(x), float(y), elevation(x%w, y%h).r)
                         / gridSize - 0.5f) * mapSize;

            // compute normal & tangents from partial derivatives:
            //   position =
            //     (u / gridSize.x - .5) * mapSize.x
            //     (v / gridSize.y - .5) * mapSize.y
            //     (elevation / gridSize.z - .5) * mapSize.z
            //   the u-tangent is the per-component partial derivative by u:
            //      mapSize.x / gridSize.x
            //      0
            //      d(elevation(u,v))/du * mapSize.z / gridSize.z
            //   the v-tangent is the partial derivative by v
            //      0
            //      mapSize.y / gridSize.y
            //      d(elevation(u,v))/du * mapSize.z / gridSize.z
            //   the normal is the cross product of these

            // first approximate du = d(elevation(u,v))/du (and dv)
            // be careful to wrap indices to 0 <= x < w and 0 <= y < h
            float du = (elevation((x+1)%w, y%h).r - elevation((x+w-1)%w, y%h).r)
                * 0.5f * mapSize.z / gridSize.z;
            float dv = (elevation(x%w, (y+1)%h).r - elevation(x%w, (y+h-1)%h).r)
                * 0.5f * mapSize.z / gridSize.z;

            // final tangents and normal using these
            dPdu[idx] = normalize(vec3<float>(mapSize.x/gridSize.x, 0, du));
            dPdv[idx] = normalize(vec3<float>(0, mapSize.y/gridSize.y, dv));
            norm[idx] = normalize(dPdu[idx] ^ dPdv[idx]);

            // 2D texture coordinate for rocks texture, from grid location
            texcoord[idx] = vec2<float>(float(x),float(y)) / gridSize.xy;
        }
    }

    // build index array linking sets of three vertices into triangles
    // two triangles per square in the grid. Each vertex index is
    // essentially its unfolded grid array position. Be careful that
    // each triangle ends up in counter-clockwise order
    numtri = 2*w*h;
    indices = new Vec<unsigned int, 3>[numtri];
    for(unsigned int y=0, idx=0; y<h; ++y) {
        for(unsigned int x=0; x<w; ++x, idx+=2) {
            indices[idx][0] = (w+1)* y    + x;
            indices[idx][1] = (w+1)* y    + x+1;
            indices[idx][2] = (w+1)*(y+1) + x+1;

            indices[idx+1][0] = (w+1)* y    + x;
            indices[idx+1][1] = (w+1)*(y+1) + x+1;
            indices[idx+1][2] = (w+1)*(y+1) + x;
        }
    }

    // load vertex and index array to GPU
    glBindBuffer(GL_ARRAY_BUFFER, bufferIDs[POSITION_BUFFER]);
    glBufferData(GL_ARRAY_BUFFER, numvert*sizeof(Vec3f), vert, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, bufferIDs[TANGENT_BUFFER]);
    glBufferData(GL_ARRAY_BUFFER, numvert*sizeof(Vec3f), dPdu, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, bufferIDs[BITANGENT_BUFFER]);
    glBufferData(GL_ARRAY_BUFFER, numvert*sizeof(Vec3f), dPdv, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, bufferIDs[NORMAL_BUFFER]);
    glBufferData(GL_ARRAY_BUFFER, numvert*sizeof(Vec3f), norm, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, bufferIDs[UV_BUFFER]);
    glBufferData(GL_ARRAY_BUFFER, numvert*sizeof(Vec2f), texcoord, 
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferIDs[INDEX_BUFFER]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
                 numtri*sizeof(unsigned int[3]), indices, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // initial shader load
    shaderParts[0].id = glCreateShader(GL_VERTEX_SHADER);
    shaderParts[0].file = "terrain.vert";
    shaderParts[1].id = glCreateShader(GL_FRAGMENT_SHADER);
    shaderParts[1].file = "terrain.frag";
    shaderID = glCreateProgram();
    updateShaders();
}

//
// Delete terrain data
//
Terrain::~Terrain()
{
    glDeleteShader(shaderParts[0].id);
    glDeleteShader(shaderParts[1].id);
    glDeleteProgram(shaderID);
    glDeleteTextures(NUM_TEXTURES, textureIDs);
    glDeleteBuffers(NUM_BUFFERS, bufferIDs);
    glDeleteVertexArrays(1, &varrayID);

    delete[] indices;
    delete[] texcoord;
    delete[] norm;
    delete[] dPdv;
    delete[] dPdu;
    delete[] vert;
}

//
// load (or replace) texture
//
void Terrain::updateTexture(const char *ppm, unsigned int textureID)
{
    ImagePPM texture(ppm);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 
                 texture.width, texture.height, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, texture.image);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
}

//
// load (or replace) terrain shaders
//
void Terrain::updateShaders()
{
    loadShaders(shaderID, sizeof(shaderParts)/sizeof(*shaderParts), 
                shaderParts);
    glUseProgram(shaderID);

    // (re)connect view and projection matrices
    glUniformBlockBinding(shaderID, 
                          glGetUniformBlockIndex(shaderID,"SceneData"),
                          AppContext::SCENE_UNIFORMS);

    // map shader name for texture to glActiveTexture number used in draw
    glUniform1i(glGetUniformLocation(shaderID, "colorTexture"), COLOR_TEXTURE);
    glUniform1i(glGetUniformLocation(shaderID, "normalTexture"), NORMAL_TEXTURE);
    glUniform1i(glGetUniformLocation(shaderID, "glossTexture"), GLOSS_TEXTURE);

    // re-connect attribute arrays
    glBindVertexArray(varrayID);

    GLint positionAttrib = glGetAttribLocation(shaderID, "vPosition");
    glBindBuffer(GL_ARRAY_BUFFER, bufferIDs[POSITION_BUFFER]);
    glVertexAttribPointer(positionAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(positionAttrib);

    GLint tangentAttrib = glGetAttribLocation(shaderID, "vTangent");
    glBindBuffer(GL_ARRAY_BUFFER, bufferIDs[TANGENT_BUFFER]);
    glVertexAttribPointer(tangentAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(tangentAttrib);

    GLint bitangentAttrib = glGetAttribLocation(shaderID, "vBitangent");
    glBindBuffer(GL_ARRAY_BUFFER, bufferIDs[BITANGENT_BUFFER]);
    glVertexAttribPointer(bitangentAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(bitangentAttrib);

    GLint normalAttrib = glGetAttribLocation(shaderID, "vNormal");
    glBindBuffer(GL_ARRAY_BUFFER, bufferIDs[NORMAL_BUFFER]);
    glVertexAttribPointer(normalAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(normalAttrib);

    GLint uvAttrib = glGetAttribLocation(shaderID, "vUV");
    glBindBuffer(GL_ARRAY_BUFFER, bufferIDs[UV_BUFFER]);
    glVertexAttribPointer(uvAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(uvAttrib);

    // turn off everything we enabled
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

//
// this is called every time the terrain needs to be redrawn 
//
void Terrain::draw() const
{
    // enable shaders
    glUseProgram(shaderID);

    // enable vertex array and textures
    glBindVertexArray(varrayID);
    for(int i=0; i<NUM_TEXTURES; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, textureIDs[i]);
    }

    // draw the triangles for each three indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferIDs[INDEX_BUFFER]);
    glDrawElements(GL_TRIANGLES, 3*numtri, GL_UNSIGNED_INT, 0);

    // turn of whatever we turned on
    for(int i=0; i<NUM_TEXTURES; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

