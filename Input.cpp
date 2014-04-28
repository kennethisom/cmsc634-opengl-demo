// set up and maintain view as window sizes change

#include "Input.hpp"
#include "AppContext.hpp"
#include "Scene.hpp"
#include "Terrain.hpp"
#include "Marker.hpp"

// using core modern OpenGL
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <math.h>

#ifndef F_PI
#define F_PI 3.1415926f
#endif

//
// called when a mouse button is pressed. 
// Remember where we were, and what mouse button it was.
//
void Input::mousePress(GLFWwindow *win, int b, int action)
{
    if (action == GLFW_PRESS) {
        // hide cursor, record button
        glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        button = b;
    }
    else {
        // display cursor, update button state
        glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        button = -1;       // no button
    }
}

//
// called when the mouse moves
// use difference between oldX,oldY and x,y to define a rotation
//
void Input::mouseMove(GLFWwindow *win, Scene *scene, double x, double y)
{
    // only update view after at least one old position is stored
    if (button == GLFW_MOUSE_BUTTON_LEFT && button == oldButton) {
        // record differences & update last position
        float dx = float(x - oldX);
        float dy = float(y - oldY);

        // rotation angle, scaled so across the window = one rotation
        scene->viewSph.x += 180 * dx / float(scene->width);
        scene->viewSph.y += 90 * dy / float(scene->height);
        scene->view();

        // tell GLFW that something has changed and we must redraw
        redraw = true;
    }

    // update prior mouse state
    oldButton = button;
    oldX = x;
    oldY = y;
}

//
// called when any key is pressed
//
void Input::keyPress(GLFWwindow *win, int key, AppContext *appctx)
{
    switch (key) { // Light position is in Radians
    case 'A':                   // rotate left
        panRate = -F_PI; // half a rotation/sec
        updateTime = glfwGetTime();
        redraw = true;          // need to redraw
        break;

    case 'D':                   // rotate right
        panRate = F_PI;  // half a rotation/sec
        updateTime = glfwGetTime();
        redraw = true;          // need to redraw
        break;

    case 'W':                   // rotate up
        tiltRate = 0.5f * F_PI; // 1/4 rotation/sec
        updateTime = glfwGetTime();
        redraw = true;          // need to redraw
        break;

    case 'S':                   // rotate down
        tiltRate = -0.5f * F_PI; // 1/4 rotation/sec
        updateTime = glfwGetTime();
        redraw = true;          // need to redraw
        break;

    case 'R':                   // reload shaders
        appctx->terrain->updateShaders();
        appctx->lightmarker->updateShaders();
        redraw = true;          // need to redraw
        break;

    case 'F':                   // toggle fog on or off
        appctx->scene->sdata.fog = 1 - appctx->scene->sdata.fog;
        redraw = true;          // need to redraw
        break;

    case GLFW_KEY_ESCAPE:                    // Escape: exit
        glfwSetWindowShouldClose(win, true);
        break;
    }
}

//
// called when any key is released
//
void Input::keyRelease(GLFWwindow *win, int key)
{
    switch (key) {
    case 'A': case 'D':         // stop panning
        panRate = 0;
        break;
    case 'W': case 'S':         // stop tilting
        tiltRate = 0;
        break;
    }
}

//
// update view if necessary based on a/d keys
//
void Input::keyUpdate(AppContext *appctx)
{
    if (panRate != 0 || tiltRate != 0) {
        double now = glfwGetTime();
        double dt = (now - updateTime);

        // update pan based on time elapsed since last update
        // ensures uniform rate of change
        appctx->scene->lightSph.x += float(panRate * dt);
        appctx->scene->lightSph.y += float(tiltRate * dt);
        appctx->scene->light(*appctx->lightmarker);

        // remember time for next update
        updateTime = now;

        // changing, so will need to start another draw
        redraw = true;
    }
}
