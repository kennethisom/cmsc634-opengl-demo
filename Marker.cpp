// draw a simple tetrahedral light indicator

#include "Marker.hpp"
#include "AppContext.hpp"

// using core modern OpenGL
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


//
// load the geometry data
//
Marker::Marker()
{
    // buffer objects to be used later
    glGenBuffers(NUM_BUFFERS, bufferIDs);
    glGenVertexArrays(NUM_VARRAYS, varrayIDs);

    // build vertex array = corners of an octahedron
    numvert = sizeof(vert)/sizeof(*vert);
    vert[0] = glm::vec3( 10.f,  0.f,  0.f);
    vert[1] = glm::vec3(-10.f,  0.f,  0.f);
    vert[2] = glm::vec3(  0.f, 10.f,  0.f);
    vert[3] = glm::vec3(  0.f,-10.f,  0.f);
    vert[4] = glm::vec3(  0.f,  0.f, 10.f);
    vert[5] = glm::vec3(  0.f,  0.f,-10.f);

    // build index array linking sets of three vertices into triangles
    numtri = sizeof(indices)/sizeof(*indices);
    indices[0] = glm::uvec3(0, 2, 4);
    indices[1] = glm::uvec3(0, 4, 3);
    indices[2] = glm::uvec3(0, 3, 5);
    indices[3] = glm::uvec3(0, 5, 2);
    indices[4] = glm::uvec3(1, 4, 2);
    indices[5] = glm::uvec3(1, 2, 5);
    indices[6] = glm::uvec3(1, 5, 3);
    indices[7] = glm::uvec3(1, 3, 4);

    // load vertex and index array to GPU
    glBindBuffer(GL_ARRAY_BUFFER, bufferIDs[POSITION_BUFFER]);
    glBufferData(GL_ARRAY_BUFFER, numvert*sizeof(glm::vec3), vert, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferIDs[INDEX_BUFFER]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
                 numtri*sizeof(glm::uvec3), indices, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // create uniform buffer objects
    glBindBuffer(GL_UNIFORM_BUFFER, bufferIDs[UNIFORM_BUFFER]);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(ModelData), 0, GL_STREAM_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, AppContext::MODEL_UNIFORMS,
                     bufferIDs[UNIFORM_BUFFER]);

    // initial shader load
    shaderParts[0].id = glCreateShader(GL_VERTEX_SHADER);
    shaderParts[0].file = "marker.vert";
    shaderParts[1].id = glCreateShader(GL_FRAGMENT_SHADER);
    shaderParts[1].file = "marker.frag";
    shaderID = glCreateProgram();
    updateShaders();
}

//
// Delete terrain data
//
Marker::~Marker()
{
    glDeleteProgram(shaderID);
    glDeleteBuffers(NUM_BUFFERS, bufferIDs);
}

//
// load (or replace) terrain shaders
//
void Marker::updateShaders()
{
    loadShaders(shaderID, sizeof(shaderParts)/sizeof(*shaderParts), 
                shaderParts);
    glUseProgram(shaderID);

    // (re)connect uniform shader parameter blocks
    glUniformBlockBinding(shaderID, 
                          glGetUniformBlockIndex(shaderID,"SceneData"),
                          AppContext::SCENE_UNIFORMS);

    glUniformBlockBinding(shaderID, 
                          glGetUniformBlockIndex(shaderID,"ModelData"),
                          AppContext::MODEL_UNIFORMS);

    // re-connect attribute arrays
    glBindVertexArray(varrayIDs[TERRAIN_VARRAY]);

    GLint positionAttrib = glGetAttribLocation(shaderID, "vPosition");
    glBindBuffer(GL_ARRAY_BUFFER, bufferIDs[POSITION_BUFFER]);
    glVertexAttribPointer(positionAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(positionAttrib);

    // turn off everything we enabled
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

//
// update marker position
void Marker::updatePosition(const glm::vec3 &center)
{
    mdata.viewMat = glm::translate(glm::mat4(), center);
	mdata.viewInverse = glm::inverse(mdata.viewMat);
}

//
// this is called every time the terrain needs to be redrawn 
//
void Marker::draw() const
{
    // enable shaders
    glUseProgram(shaderID);

    // update uniform model-parameter block
    glBindBuffer(GL_UNIFORM_BUFFER, bufferIDs[UNIFORM_BUFFER]);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ModelData), &mdata);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // enable vertex arrays
    glBindVertexArray(varrayIDs[TERRAIN_VARRAY]);

    // draw the triangles for each three indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferIDs[INDEX_BUFFER]);
    glDrawElements(GL_TRIANGLES, 3*numtri, GL_UNSIGNED_INT, 0);

    // turn of whatever we turned on
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

