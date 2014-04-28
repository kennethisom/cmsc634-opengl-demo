// data shared across entire scene
// expected to change up to once per frame
#ifndef Scene_hpp
#define Scene_hpp

#include <glm/glm.hpp>

class Marker;
struct GLFWwindow;

class Scene {
// private data
private:
    // GL uniform buffer IDs
    enum {UNIFORM_BUFFER, NUM_BUFFERS};
    unsigned int bufferIDs[NUM_BUFFERS];

// public data
public:
    struct ShaderData {
        //MatPair4f viewmat, projection; // viewing matrices
		glm::mat4 viewMat, viewInverse;
		glm::mat4 projectionMat, projectionInverse;
        glm::vec3 lightpos;		       // light position
        int fog;		       // 1 = display fog, 0 = don't
    } sdata;

    int width, height;         // current window dimensions

    glm::vec3 viewSph;          // view position in spherical coordinates
    glm::vec3 lightSph;         // light position in spherical coordinates

// public methods
public:
    // create with initial window size and orbit location
    Scene(GLFWwindow *win, Marker &lightMarker);

    // set up new window viewport and projection
    void viewport(GLFWwindow *win);

    // set view using orbitAngle
    void view();

    // update light
    void light(Marker &lightMarker);

    // update shader uniform state each frame
    void update() const;
};

#endif
