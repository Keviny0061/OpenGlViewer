// template based on material from learnopengl.com
#define GLEW_STATIC

#include <GL/glew.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>
#include <chrono>
#include <glm/gtc/matrix_transform.hpp>
#include "ShaderReader.h"
#include "ShaderProgram.h"
#include "ObjReader.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void set_vec3_uniform(ShaderProgram &shaderProgram, std::string const& uniform_name, glm::vec3 const& v);
void set_boolean_uniform(ShaderProgram &shaderProgram, std::string const& uniform_name, bool v);
void set_float_uniform(ShaderProgram &shaderProgram, std::string const& uniform_name, float v);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

float x_position;
float y_position;
float z_position = -10;

float x_rotation;
float y_rotation;
float z_rotation;

float x_scale = 1;
float y_scale = 1;
float z_scale = 1;
// For perspective projection
float fovy = 45.0;
float near_plane = 0.1;
float far_plane = 1000.0f;

bool showZBuffer = false;
bool useGouraudShading = false;
bool usePhongShading = false;
bool useFlatShading = false;

void reset_variables() {
    x_position = 0.0;
    y_position = 0.0;
    z_position = -5.0;
    x_rotation = 0.0;
    y_rotation = 0.0;
    z_rotation = 0.0;
    x_scale = 1;
    y_scale = 1;
    z_scale = 1;
    fovy = 45.0;
    near_plane = 0.1;
    far_plane = 1000.0f;
}

int main(int argc, char *argv[])
{
    // Load in program arguments as variables
    std::string input;
    std::cout << "Enter the object you want to read: ";

    // Take input as a string
    std::getline(std::cin, input);
    std::string targetModel = input; // choose shape/model

    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "viewGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);

    // // glew: load all OpenGL function pointers
    glewInit();

    glEnable(GL_DEPTH_TEST);

    // Choose shader    
    std::string vertexShader = "shader_1";
    std::string fragmentShader = "shader_1";

    auto vertex = ("../data/shaders/" + vertexShader + ".vs");
    auto frag = ("../data/shaders/" + fragmentShader + ".fs");
    ShaderReader shaderReader(vertex.c_str(), frag.c_str());
    ShaderData shaderData;
    shaderReader.read(shaderData);
    shaderData.print();
    if(shaderReader.wasError()){
        std::cout << "Failed to read shader data. \n";
        return -1;
    }
    
    ShaderProgram shaderProgram(shaderData);
    if(shaderProgram.wasError()){
        std::cout << "Failed to compile and link program \n";
        return -1;
    }

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    ObjReader objReader;
    ObjData objData;
    unsigned int numVertices = 0;
    objReader.readObjAsIndexed(targetModel, objData, true);
    
    // Scale all verts. Done before vertices are potentially duplicated
    // BUG ? LIGHTING NOT WORKING WITH THIS: FIX ME !! 
    // objReader.scaleToClipCoords(objData);

    // .obj files are indexed triangle structures. Need to convert to separate triangles. 
    ObjData currentObjData;
   
        // Resolve into currentObj, then count actual number of vertices used. Put here for flexibility
     objReader.indexedToSeparateTriangles(objData, currentObjData);
        numVertices = currentObjData.vertices.size();
    
    

    unsigned int VAO;
    unsigned int VBO_Verts, VBO_Color; // VAO will contain 2 buffers, use data from both. 
    unsigned int EBO;

    // Request indices from GPU, managed by OS.
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO_Verts);
    glGenBuffers(1, &VBO_Color);
    glGenBuffers(1, &EBO);

    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_Verts);
    glBufferData(GL_ARRAY_BUFFER, currentObjData.vertices.size() * sizeof(glm::vec3), &currentObjData.vertices[0], GL_STATIC_DRAW);

    // Pointer starts at first vertex (0 floats in), and jumps past itself (3 floats) every iteration
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_Color);
    glBufferData(GL_ARRAY_BUFFER, currentObjData.normals.size() * sizeof(glm::vec3), &currentObjData.normals[0], GL_STATIC_DRAW);

    // Pointer starts at first vertex (0 floats in), and jumps past itself (3 floats) every iteration
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0); 

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0); 


    // uncomment this call to draw in wireframe polygons.
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    

    std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
    unsigned int frames = 0;
    double totalDuration;
    double lastPrintDuration;
    bool cpuAppliedTransformLastFrame = false;

    // render loop
    // -----------
    std::cout << "Starting render loop \n";
    while (!glfwWindowShouldClose(window))
    {
        frames++;
        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        // Create model transform. May update every frame, so need to set each frame. 
        // ------
        glm::mat4 modelMatrix(1.0f);        
        float x_rotate = x_rotation * 3.142 / 180;
        float y_rotate = y_rotation * 3.142 / 180;
        float z_rotate = z_rotation * 3.142 / 180;

        modelMatrix = glm::translate(modelMatrix, glm::vec3(x_position, y_position, z_position)) 
            * glm::rotate(modelMatrix, x_rotate, glm::vec3(1.f, 0.f, 0.f)) 
            * glm::rotate(modelMatrix, y_rotate, glm::vec3(0.f, 1.f, 0.f)) 
            * glm::rotate(modelMatrix, z_rotate, glm::vec3(0.f, 0.f, 1.f))
            * glm::scale(modelMatrix, glm::vec3(x_scale, y_scale, z_scale));
                
        // View Matrix
        glm::mat4 viewMatrix(1.0f);
        glm::vec3 viewerPosition = glm::vec3(0.0);
        glm::vec3 viewerCenter = glm::vec3(0.0, 0.0, -1.0);
        glm::vec3 viewerUp = glm::vec3(0.0, 1.0, 0.0);
        viewMatrix = glm::lookAt(viewerPosition, viewerCenter, viewerUp);

        // Perspective Projection Matrix
        glm::mat4 perspectiveMatrix;
        float fov_r = glm::radians(fovy);
        float aspect_ratio = (float)SCR_WIDTH / (float)SCR_HEIGHT;
        float zNear = near_plane;
        float zFar = far_plane;
        perspectiveMatrix = glm::perspective(fov_r, aspect_ratio, zNear, zFar);
        
        // Normal Matrix for lighting calculation
        glm::mat3 normalMatrix;
        normalMatrix = glm::mat3(glm::transpose(glm::inverse(modelMatrix)));

        start = std::chrono::high_resolution_clock::now();

        // Use model transform. Comparing strings is a fraction of time. 
        // ------
        
            // Set uniform variable in GPU's existing shader program.
            int modelPosition = glGetUniformLocation(shaderProgram.getID(), "model");
            glUniformMatrix4fv(modelPosition, 1, GL_FALSE, &modelMatrix[0][0]);

            // Set view matrix
            int viewPosition = glGetUniformLocation(shaderProgram.getID(), "view");
            glUniformMatrix4fv(viewPosition, 1, GL_FALSE, &viewMatrix[0][0]);

            // Set uniform variable for projection matrix in GPU's shader program
            int projectionPosition = glGetUniformLocation(shaderProgram.getID(), "projection");
            glUniformMatrix4fv(projectionPosition, 1, GL_FALSE, &perspectiveMatrix[0][0]);

            // Normal Matrix for Gouraud and Phong shading
            int normalMatrixPosition = glGetUniformLocation(shaderProgram.getID(), "normalMatrix");
            glUniformMatrix3fv(normalMatrixPosition, 1, GL_FALSE, &normalMatrix[0][0]);

        


        int showZBufferPosition = glGetUniformLocation(shaderProgram.getID(), "showZBuffer");
        if (showZBufferPosition == -1) {
            char infoLog[1024];
            glGetProgramInfoLog(shaderProgram.getID(), 512, NULL, infoLog);
            std::cerr << infoLog << std::endl;
            std::cerr << "Error: Cannot find the uniform variable showZBuffer" << std::endl;
            exit(1);
        }
        if (showZBuffer) {
            glUniform1i(showZBufferPosition, 1);
        }
        else {
            glUniform1i(showZBufferPosition, 0);
        }

        // Lighting Calculations
        {
           glm::vec3 lightPosition = glm::vec3(0.0, 3.0, -3.0);
           glm::vec3 lightColor = glm::vec3(1.0);
           glm::vec3 objectColor = glm::vec3(1.0, 0.0, 0.0); // RED COLOR
           float shininess = 32;
           set_vec3_uniform(shaderProgram, "lightPosition", lightPosition);
           set_vec3_uniform(shaderProgram, "viewerPosition", viewerPosition);
           set_vec3_uniform(shaderProgram, "lightColor", lightColor);
           set_vec3_uniform(shaderProgram, "objectColor", objectColor);
           set_boolean_uniform(shaderProgram, "useGouraudShading", useGouraudShading);
           set_boolean_uniform(shaderProgram, "usePhongShading", usePhongShading);
           set_boolean_uniform(shaderProgram, "useFlatShading", useFlatShading);
           set_float_uniform(shaderProgram, "shininess", shininess);
        }

        // Draw using GPU buffer data
        // ------
        shaderProgram.use();
        glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
        glDrawArrays(GL_TRIANGLES, 0, numVertices);
         
        end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsedSeconds = end - start;
        totalDuration += elapsedSeconds.count();

       
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        // Swap buffers should just move pointers, doesn't scale based on number of items.
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO_Verts);
    glDeleteBuffers(1, &VBO_Color);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

struct ButtonBinds {
    const int xInc;
    const int xDec;
    const int yInc;
    const int yDec;
    const int zInc;
    const int zDec;
    const int allInc;
    const int allDec;

    ButtonBinds(int xInc, int xDec, int yInc, int yDec, int zInc, int zDec, int allInc, int allDec):
        xInc(xInc), xDec(xDec), yInc(yInc), yDec(yDec), zInc(zInc), zDec(zDec), allInc(allInc), allDec(allDec) {}
};


void updateDeltaFromInput(GLFWwindow* window, float& x, float& y, float& z, float delta, const ButtonBinds& binds){
    bool allInc = glfwGetKey(window, binds.allInc) == GLFW_PRESS;
    bool allDec = glfwGetKey(window, binds.allDec) == GLFW_PRESS;
    if (glfwGetKey(window, binds.xInc) == GLFW_PRESS || allInc)
        x += delta;
    if (glfwGetKey(window, binds.xDec) == GLFW_PRESS || allDec)
        x -= delta;
    if (glfwGetKey(window, binds.yInc) == GLFW_PRESS || allInc) 
        y += delta;
    if (glfwGetKey(window, binds.yDec) == GLFW_PRESS || allDec)
        y -= delta;
    if (glfwGetKey(window, binds.zInc) == GLFW_PRESS || allInc)
        z += delta;
    if (glfwGetKey(window, binds.zDec) == GLFW_PRESS || allDec)
        z -= delta;
}

void updateIfReset(GLFWwindow* window, int reset_glfw_key)
{
    if (glfwGetKey(window, reset_glfw_key) == GLFW_PRESS) {
        reset_variables();
    }
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float positionDelta = 0.1f;
    float rotationDelta = 0.5f;
    float scaleDelta = 0.1f;

    float projectionDelta = 0.1f;

    const ButtonBinds positionKeys(
        GLFW_KEY_RIGHT, GLFW_KEY_LEFT,
        GLFW_KEY_UP, GLFW_KEY_DOWN, 
        GLFW_KEY_1, GLFW_KEY_2,
        GLFW_KEY_3, GLFW_KEY_4
    );

    const ButtonBinds rotationKeys(
        GLFW_KEY_W, GLFW_KEY_S, 
        GLFW_KEY_A, GLFW_KEY_D,
        GLFW_KEY_Q, GLFW_KEY_E,
        GLFW_KEY_R, GLFW_KEY_F
    );

    const ButtonBinds scaleKeys {
        GLFW_KEY_J, GLFW_KEY_L,
        GLFW_KEY_I, GLFW_KEY_K,
        GLFW_KEY_U, GLFW_KEY_O,
        GLFW_KEY_Y, GLFW_KEY_H
    };

    // Perspective Projection
    const ButtonBinds perspectiveKeys {
        GLFW_KEY_5, GLFW_KEY_6,
        GLFW_KEY_7, GLFW_KEY_8,
        GLFW_KEY_9, GLFW_KEY_0,
        GLFW_KEY_UNKNOWN, GLFW_KEY_UNKNOWN   // SKIP ALL INC/DEC
    };

    updateDeltaFromInput(window, x_position, y_position, z_position, positionDelta, positionKeys);
    updateDeltaFromInput(window, x_rotation, y_rotation, z_rotation, rotationDelta, rotationKeys);
    updateDeltaFromInput(window, x_scale, y_scale, z_scale, scaleDelta, scaleKeys);
    updateDeltaFromInput(window, fovy, near_plane, far_plane, projectionDelta, perspectiveKeys);

    // Reset values to default
    updateIfReset(window, GLFW_KEY_SPACE);

}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
        showZBuffer = !showZBuffer;
    }
    else if (key == GLFW_KEY_G && action == GLFW_PRESS) {
        useGouraudShading = !useGouraudShading;
        usePhongShading = false;
        useFlatShading = false;
    }
    else if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        usePhongShading = !usePhongShading;
        useGouraudShading = false;
        useFlatShading = false;
    }
    else if (key == GLFW_KEY_T && action == GLFW_PRESS) {
        useFlatShading = !useFlatShading;
        useGouraudShading = false;
        usePhongShading = false;
    }
}

void set_vec3_uniform(ShaderProgram &shaderProgram, std::string const& uniform_name, glm::vec3 const& v) {
    int position = glGetUniformLocation(shaderProgram.getID(), uniform_name.c_str());
    if (position == -1) {
        std::cerr << "Error: Cannot find uniform variable: " << uniform_name << std::endl;
        exit(1);
    }
    glUniform3fv(position, 1, &v[0]);
}

void set_boolean_uniform(ShaderProgram &shaderProgram, std::string const& uniform_name, bool v) {
    int position = glGetUniformLocation(shaderProgram.getID(), uniform_name.c_str());
    if (position == -1) {
        std::cerr << "Error: Cannot find uniform variable: " << uniform_name << std::endl;
        exit(1);
    }
    glUniform1i(position, (int)v);
}

void set_float_uniform(ShaderProgram &shaderProgram, std::string const& uniform_name, float v) {
    int position = glGetUniformLocation(shaderProgram.getID(), uniform_name.c_str());
    if (position == -1) {
        std::cerr << "Error: Cannot find uniform variable: " << uniform_name << std::endl;
        exit(1);
    }
    glUniform1f(position, v);
}