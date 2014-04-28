// data shared across entire scene
// expected to change up to once per frame
// primarily view information

#include "Scene.hpp"
#include "AppContext.hpp"
#include "Marker.hpp"

// using core modern OpenGL
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/matrix.hpp>
#include <glm/gtc/matrix_transform.hpp>

// for offsetof
#include <cstddef>

#ifndef F_PI
#define F_PI 3.1415926f
#endif

//
// create and initialize view
//
Scene::Scene(GLFWwindow *win, Marker &lightmarker) : 
    viewSph(glm::vec3(0.f, -80.5f, 500.f)),
    lightSph(glm::vec3(F_PI/2.f, F_PI/4.f, 300.f)) // Light position is in radians.
{
    // create uniform buffer objects
    glGenBuffers(NUM_BUFFERS, bufferIDs);
    glBindBuffer(GL_UNIFORM_BUFFER, bufferIDs[UNIFORM_BUFFER]);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(ShaderData), 0, GL_STREAM_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, AppContext::SCENE_UNIFORMS,
                     bufferIDs[UNIFORM_BUFFER]);

    // initialize scene data
    viewport(win);
    view();
    light(lightmarker);
    sdata.fog = 0;                    // fog off
}

//
// New view, pointing to origin, at specified angle
//
void Scene::view()
{
    // update view matrix
	sdata.viewMat = glm::mat4();
	sdata.viewMat = glm::translate(sdata.viewMat, glm::vec3(0,0,-viewSph.z)); //translate4fp(vec3<float>(0,0,-viewSph.z)) 
	sdata.viewMat = glm::rotate(sdata.viewMat, viewSph.y, glm::vec3(1.f, 0.f, 0.f));
	sdata.viewMat = glm::rotate(sdata.viewMat, viewSph.x, glm::vec3(0.f, 0.f, 1.f));
        //* xrotate4fp(viewSph.y)
        //* zrotate4fp(viewSph.x);
	sdata.viewInverse = glm::inverse(sdata.viewMat);
}

//
// This is called when window is created or resized
// Adjust projection accordingly.
//
void Scene::viewport(GLFWwindow *win)
{
    // get window dimensions
    glfwGetFramebufferSize(win, &width, &height);

    // this viewport makes a 1 to 1 mapping of physical pixels to GL
    // "logical" pixels
    glViewport(0, 0, width, height);

    // adjust 3D projection into this window
    sdata.projectionMat = glm::perspective(45.f, (float)width/height, 1.f, 10000.f);
	sdata.projectionInverse = glm::inverse(sdata.projectionMat);
}


//
// Call to update light position
//
void Scene::light(Marker &lightmarker)
{
    // update position from spherical coordinates
    float cx = cos(lightSph.x), sx = sin(lightSph.x);
    float cy = cos(lightSph.y), sy = sin(lightSph.y);
    sdata.lightpos = lightSph.z * glm::vec3(cx*cy, sx*cy, sy);

    // update marker position
	glm::vec3 lpos(sdata.lightpos.x, sdata.lightpos.y, sdata.lightpos.z);
    lightmarker.updatePosition(lpos);
}


//
// call before drawing each frame to update per-frame scene state
//
void Scene::update() const
{
    // update uniform block
    glBindBuffer(GL_UNIFORM_BUFFER, bufferIDs[UNIFORM_BUFFER]);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ShaderData), &sdata);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}
