/*
 * A C++ framework for OpenGL programming in TNM061 for MT1 2014.
 *
 * This is a small, limited framework, designed to be easy to use
 * for students in an introductory computer graphics course in
 * the second year of a M Sc curriculum. It uses custom code
 * for some things that are better solved by external libraries
 * like GLEW and GLM, but the emphasis is on simplicity and
 * readability, not generality.
 * For the window management, GLFW 3.0 is used for convenience.
 * The framework should work in Windows, MacOS X and Linux.
 * Some Windows-specific stuff for extension loading is still
 * here. GLEW could have been used instead, but for clarity
 * and minimal dependence on other code, we rolled our own extension
 * loading for the things we need. That code is short-circuited on
 * platforms other than Windows. This code is dependent only on
 * the GLFW and OpenGL libraries. OpenGL 3.3 or higher is required.
 *
 * Author: Stefan Gustavson (stegu@itn.liu.se) 2013-2014
 * updated 2021 by Martin Falk (martin.falk@liu.se)
 *
 * This code is in the public domain.
 */
#if defined(WIN32) && !defined(_USE_MATH_DEFINES)
#define _USE_MATH_DEFINES
#endif

// System utilities
#include <iostream>
#include <cstdlib>
#include <cmath>

#include <GL/glew.h>
// GLFW 3.x, to handle the OpenGL window
#include <GLFW/glfw3.h>

// Headers for the other source files that make up this program.
#include "util/tnm061.hpp"
#include "util/Shader.hpp"
#include "util/Texture.hpp"
#include "util/TriangleSoup.hpp"
#include "util/Rotator.hpp"
#include "util/MatrixStack.hpp"

/*
 * setupViewport() - set up the OpenGL viewport.
 * This should be done for each frame, to handle window resizing.
 * The "proper" way would be to set a "resize callback function"
 * using glfwSetWindowSizeCallback() and do these few operations
 * only when something changes, but let's keep it simple.
 * Besides, we want to change P when the aspect ratio changes.
 * A callback function would require P to be changed indirectly
 * in some manner, which is somewhat awkward in this case.
 */
void setupViewport(GLFWwindow* window, GLfloat* P) {

    int width, height;

    // Get window size. It may start out different from the requested
    // size, and will change if the user resizes the window.
    glfwGetWindowSize(window, &width, &height);

    // adjust perspective matrix for non-square aspect ratios
    P[0] = P[5] * height / width;

    // Set viewport. This is the pixel rectangle we want to draw into.
    glViewport(0, 0, width, height);  // The entire window
}


// Egen hjälp funktion
void renderPlanet(MatrixStack& stack, Texture& texture, float orbitRadius, float orbitSpeed,
                  float selfRotationSpeed, float scale, float time, GLint location_MV,
                  TriangleSoup& sphere) {
    stack.push();
    stack.rotY(orbitSpeed * time);             // Planetens omloppsbana runt solen
    stack.translate(orbitRadius, 0.0f, 0.0f);  // Radie för omloppsbana
    stack.push();
    stack.rotY(selfRotationSpeed * time);  // Planetens rotation runt sin egen axel
    stack.scale(scale);                    // Skalning av planeten
    glUniformMatrix4fv(location_MV, 1, GL_FALSE, stack.getCurrentMatrix());
    glBindTexture(GL_TEXTURE_2D, texture.texID);
    sphere.render();  // Rendera planeten
    stack.pop();      // Återställ egen rotationskontext
    stack.pop();      // Återställ solens kontext
}


int main() {
    // Initialise GLFW
    glfwInit();

    // GLFW struct to hold information about the display
    const GLFWvidmode* vidmode = glfwGetVideoMode(glfwGetPrimaryMonitor());

    // Make sure we are getting a GL context of at least version 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    // GLFW struct to hold information about the window
    // Open a square window (aspect ratio 1:1) to fill half the screen height
    GLFWwindow* window = glfwCreateWindow(vidmode->height / 2, vidmode->height / 2,
                                          "TNM061 Lab1: Hierarchical Transformations", NULL, NULL);
    if (!window) {
        glfwTerminate();  // No window was opened, so we can't continue in any useful way
        return -1;
    }

    // Make the newly created window the "current context" for OpenGL
    // (This step is strictly required, or things will simply not work)
    glfwMakeContextCurrent(window);

    GLenum err = glewInit();
    if (GLEW_OK != err) {
        std::cerr << "Error: " << glewGetErrorString(err) << "\n";
        glfwTerminate();
        return -1;
    }

    MouseRotator rotator;
    rotator.init(window);

    std::cout << "GL vendor:       " << glGetString(GL_VENDOR)
              << "\nGL renderer:     " << glGetString(GL_RENDERER)
              << "\nGL version:      " << glGetString(GL_VERSION)
              << "\nDesktop size:    " << vidmode->width << " x " << vidmode->height << "\n";

    glfwSwapInterval(0);  // Do not wait for screen refresh between frames

    // Perspective projection matrix
    // This is the standard gluPerspective() form of the
    // matrix, with d=4, near=3, far=7 and aspect=1.
    /*GLfloat P[16] = {4.0f, 0.0f, 0.0f,  0.0f,  0.0f, 4.0f, 0.0f,   0.0f,
                     0.0f, 0.0f, -2.5f, -1.0f, 0.0f, 0.0f, -10.5f, 0.0f};*/

    // Lekte runt lite med perspektivet för att få fram ett bra perspektiv med flera planeter
    GLfloat P[16] = {4.0f, 0.0f, 0.0f,  0.0f,  // Bredda synfältet
                     0.0f, 4.0f, 0.0f,  0.0f,  // Gör vy högre
                     0.0f, 0.0f, -2.5f, -1.0f, 0.0f, 0.0f, -15.5f, 0.0f};

    // The matrix stack we are going to use to set MV
    MatrixStack modelViewStack;
    // Intialize the matrix to an identity transformation
    modelViewStack.init();

    // Create geometry for rendering
    TriangleSoup earthSphere;
    earthSphere.createSphere(1.0, 30);
    // soupReadOBJ(&myShape, MESHFILENAME);
    earthSphere.printInfo();

    // Create a shader program object from GLSL code in two files
    Shader shader;
    shader.createShader("shaders/vertexshader.glsl", "shaders/fragmentshader.glsl");

    glEnable(GL_TEXTURE_2D);
    // Read the texture data from file and upload it to the GPU
    Texture earthTexture;
    earthTexture.createTexture("textures/earth.tga");

    // Lägger in texturerna för det andra planeter jag behöver ******
    Texture moonTexture;
    moonTexture.createTexture("textures/moon.tga");

    Texture sunTexture;
    sunTexture.createTexture("textures/sun.tga");

    Texture planet1Texture;
    planet1Texture.createTexture("textures/neptune.tga");

    Texture planet2Texture;
    planet2Texture.createTexture("textures/mercury.tga");
    //************************************************************'

    // Shader uniform locations
    GLint location_MV = glGetUniformLocation(shader.programID, "MV");
    GLint location_P = glGetUniformLocation(shader.programID, "P");
    GLint location_time = glGetUniformLocation(shader.programID, "time");
    GLint location_tex = glGetUniformLocation(shader.programID, "tex");

    // Activate our shader program.
    glUseProgram(shader.programID);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Calculate and update the frames per second (FPS) display
        tnm061::displayFPS(window);

        // Set the clear color and depth, and clear the buffers for drawing
        glClearColor(0.3f, 0.3f, 0.3f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);  // Use the Z buffer
        glEnable(GL_CULL_FACE);   // Use back face culling
        glCullFace(GL_BACK);

        // Set up the viewport
        setupViewport(window, P);

        // Handle mouse input
        rotator.poll(window);
        // std::cout << "phi = " << rotator.phi << ", theta = " << rotator.theta << "\n";

        // Copy the projection matrix P into the shader.
        glUniformMatrix4fv(location_P, 1, GL_FALSE, P);

        // Tell the shader to use texture unit 0.
        glUniform1i(location_tex, 0);

        // Update the uniform time variable.
        const float time = (float)glfwGetTime();  // Needed later as well
        glUniform1f(location_time, time);

        // Draw the scene
        modelViewStack.push();  // Save the initial, untouched matrix

        // Modify MV according to user input
        // First, do the view transformations ("camera motion")
        modelViewStack.translate(0.0f, 0.0f,
                                 -7.0f);  // Ändra värden på denna för att ändra kameraposition
        modelViewStack.rotX(rotator.theta);
        modelViewStack.rotY(rotator.phi);

        // Then, do the model transformations ("object motion")
        modelViewStack.push();  // Save the current matrix on the stack

        // Sun
        modelViewStack.rotY(time);
        modelViewStack.rotX(
            static_cast<float>(-M_PI_2));  // Orient the poles along Y axis instead of Z
        modelViewStack.scale(0.5f);        // Scale unit sphere to radius 0.5
        // Update the transformation matrix in the shader
        glUniformMatrix4fv(location_MV, 1, GL_FALSE, modelViewStack.getCurrentMatrix());
        // Render the geometry to draw the sun
        glBindTexture(GL_TEXTURE_2D, sunTexture.texID);  // ändra till sol
        earthSphere.render();

        modelViewStack.pop();  // Restore the matrix we saved above

        // Earth
        modelViewStack.rotY(0.2f * time);            // Earth orbit rotation
        modelViewStack.translate(1.0f, 0.0f, 0.0f);  // Earth orbit radius (1.0 ändrades till 1.5)
        modelViewStack.push();                       // Save the matrix before the Earth's rotation

        modelViewStack.rotY(10.0f * time);  // Earth's rotation around its axis
        modelViewStack.rotX(
            static_cast<float>(-M_PI_2));  // Orient the poles along Y axis instead of Z
        modelViewStack.scale(0.2f);        // Scale unit sphere to radius 0.1
        // Update the transformation matrix in the shader
        glUniformMatrix4fv(location_MV, 1, GL_FALSE, modelViewStack.getCurrentMatrix());
        // Render the geometry to draw the earth
        glBindTexture(GL_TEXTURE_2D, earthTexture.texID);
        earthSphere.render();
        modelViewStack.pop();  // Restore the matrix we saved above

        // Lägger in Moon ****************************
        modelViewStack.push();
        modelViewStack.rotY(0.5f * time);            // Moon's orbit around Earth
        modelViewStack.translate(0.4f, 0.0f, 0.0f);  // Moon's orbit radius

        modelViewStack.scale(0.1f);  // Scale unit sphere to radius 0.1
        glUniformMatrix4fv(location_MV, 1, GL_FALSE, modelViewStack.getCurrentMatrix());
        glBindTexture(GL_TEXTURE_2D, moonTexture.texID);  // Use Moon texture
        earthSphere.render();
        modelViewStack.pop();  // Restore the matrix we saved above

        //*****************************************

        // Planet 1
        modelViewStack.push();
        modelViewStack.rotY(0.15f * time);           // Planet1's orbit around the Sun
        modelViewStack.translate(2.0f, 0.0f, 0.0f);  // Planet1's orbit radius

        //modelViewStack.push();             // Save the orbit transformation
        //modelViewStack.rotY(5.0f * time);  // Planet 1's rotation around its own axis
        

        modelViewStack.scale(0.2f);  // Scale Planet1
        glUniformMatrix4fv(location_MV, 1, GL_FALSE, modelViewStack.getCurrentMatrix());
        glBindTexture(GL_TEXTURE_2D, planet1Texture.texID);
        earthSphere.render();
        modelViewStack.pop();

        // Planet 2
        modelViewStack.push();
        modelViewStack.rotY(0.1f * time);  // Planet2's orbit around the Sun
        modelViewStack.translate(2.0f, 0.0f, 0.0f);  // Planet2's orbit radius

        //modelViewStack.push();             // Save the orbit transformation
        //modelViewStack.rotY(3.0f * time);  // Planet 2's rotation around its own axis

        modelViewStack.scale(0.2f);                  // Scale Planet2
        glUniformMatrix4fv(location_MV, 1, GL_FALSE, modelViewStack.getCurrentMatrix());
        glBindTexture(GL_TEXTURE_2D, planet2Texture.texID);
        earthSphere.render();
        modelViewStack.pop(); // Restore orbit transformation

        modelViewStack.pop();  // Restore initial matrix

        // Swap buffers, i.e. display the image and prepare for next frame.
        glfwSwapBuffers(window);

        // Poll events (read keyboard and mouse input)
        glfwPollEvents();

        // Exit if the ESC key is pressed (and also if the window is closed).
        if (glfwGetKey(window, GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }
    }

    // Play nice and deactivate the shader program
    glUseProgram(0);

    // Close the OpenGL window and terminate GLFW.
    glfwDestroyWindow(window);
    glfwTerminate();
}
