// template based on material from learnopengl.com
#define GLEW_STATIC

#include <GL/glew.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>
#include <chrono>
#include <glm/gtc/matrix_transform.hpp>
#include "entry.h"
#include "ShaderReader.h"
#include "ShaderProgram.h"
#include "ObjReader.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

float x_position;
float y_position;
float z_position;

float x_rotation;
float y_rotation;
float z_rotation;

float x_scale = 1;
float y_scale = 1;
float z_scale = 1;

int main(int argc, char *argv[])
{
    // Load in program arguments as variables
    std::string targetModel = argc > 1 ? argv[1] : "cube"; // Model in 
    std::string targetProcessor = argc > 2 ? argv[2] : "GPU"; // GPU or CPU
    std::string targetDataStructure = argc > 3 ? argv[3] : "Separate"; // Separate or Indexed

    std::cout << targetProcessor;

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

    // // glew: load all OpenGL function pointers
    glewInit();

    glEnable(GL_DEPTH_TEST);

    // Choose shader    
    std::string vertexShader = "shader_1";
    std::string fragmentShader = "shader_1";
    if(targetProcessor.compare("CPU") == 0){
        // Read shader that doesn't transform vertices. 
        vertexShader = "shader_2";
    }

    //choose and read the shader
    ShaderReader shaderReader(("../data/shaders/" + vertexShader + ".vs").c_str(), ("../data/shaders/" + fragmentShader + ".fs").c_str());
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
    objReader.scaleToClipCoords(objData);

    // .obj files are indexed triangle structures. Need to convert to separate triangles. 
    ObjData currentObjData;
    if(targetDataStructure.compare("Indexed") == 0){
        currentObjData = objData;
        numVertices = currentObjData.vertexIndices.size();
    } else if(targetDataStructure.compare("Separate") == 0){
        // Resolve into currentObj, then count actual number of vertices used. Put here for flexibility
        objReader.indexedToSeparateTriangles(objData, currentObjData);
        numVertices = currentObjData.vertices.size();
    } 
    
    // Funny story, vertex and normals are not 1:1!
    // Why? Because normals apparently map to triangles 
    // Solution: map to separate, then unmap: https://gamedev.stackexchange.com/questions/146853/using-indices-in-obj-files-and-handling-normals
    std::cout << "\n Vertex count: " << objData.vertices.size();
    std::cout << "\n Normal count: " << objData.normals.size() << "\n";

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

    // EBO moves all pointers, vert, color, etc. : https://stackoverflow.com/questions/74791262/opengl-triangles-with-element-buffer-object-ebo-aka-gl-element-array-buffer
    if(targetDataStructure.compare("Indexed") == 0){
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, currentObjData.vertexIndices.size() * sizeof(unsigned int), &currentObjData.vertexIndices[0], GL_STATIC_DRAW);
    }

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
    //delcare below right before loop
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = glm::mat4(1.0f); // This line is repeated and only needs to be declared once outside the loop.
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

        //4/8/2024
        // Perspective Projection parameters
        glm::mat4 model = glm::mat4(1.0f); // Reset model matrix for each frame
        float fov = 45.0f; // Field of View
        float aspectRatio = (float)SCR_WIDTH / (float)SCR_HEIGHT; // Aspect Ratio
        float nearPlane = 0.1f; // Near Clipping Plane
        float farPlane = 100.0f; // Far Clipping Plane

        // Perspective projection matrix
        // Create model transform. May update every frame, so need to set each frame. 
        // ------
        glm::mat4 modelMatrix(1.0f);      
        //added 4.8.2024
        glm::mat4 projection = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
        glm::mat4 view = glm::mat4(1.0f); // Placeholder for actual view matrix setup
        


        float x_rotate = x_rotation * 3.142 / 180;
        float y_rotate = y_rotation * 3.142 / 180;
        float z_rotate = z_rotation * 3.142 / 180;

        modelMatrix = glm::translate(modelMatrix, glm::vec3(x_position, y_position, z_position)) 
            * glm::rotate(modelMatrix, x_rotate, glm::vec3(1.f, 0.f, 0.f)) 
            * glm::rotate(modelMatrix, y_rotate, glm::vec3(0.f, 1.f, 0.f)) 
            * glm::rotate(modelMatrix, z_rotate, glm::vec3(0.f, 0.f, 1.f))
            * glm::scale(modelMatrix, glm::vec3(x_scale, y_scale, z_scale));

        start = std::chrono::high_resolution_clock::now();

        
        

        // Use model transform. Comparing strings is a fraction of time. 
        // ------
        if(targetProcessor.compare("GPU") == 0){
            // Set uniform variable in GPU's existing shader program.
            int modelPosition = glGetUniformLocation(shaderProgram.getID(), "modelView");
            glUniformMatrix4fv(modelPosition, 1, GL_FALSE, &modelMatrix[0][0]);
        } else if(targetProcessor.compare("CPU") == 0){
            // Update each point with CPU. 
            // Note: Must create copy, otherwise transform 'accumulates'. Either create, or iterate again with inverse transform.
            std::vector<glm::vec3> copiedVertices = currentObjData.vertices;
            for(unsigned int i = 0; i < copiedVertices.size(); i++){
                glm::vec4 result = modelMatrix * glm::vec4(copiedVertices[i], 1);
                copiedVertices[i] = glm::vec3(result.x, result.y, result.z);
            }

            // Load all vertice data into existing buffer 
            glNamedBufferSubData(VBO_Verts, 0, copiedVertices.size() * sizeof(glm::vec3), &copiedVertices[0]);
        } else{
            std::cout << "\n Invalid processor. Choose \"GPU\" or \"CPU\" as second program parameter \n";
            break;
        }

        // Draw using GPU buffer data
        // ------
        shaderProgram.use();
        glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized

        //shaderprogramSetMat4 not defined
        shaderProgram.setMat4("projection", projection);
        shaderProgram.setMat4("view", view);
        shaderProgram.setMat4("model", model);

        if(targetDataStructure.compare("Separate") == 0){
            glDrawArrays(GL_TRIANGLES, 0, numVertices);
        } else if(targetDataStructure.compare("Indexed") == 0){
            glDrawElements(GL_TRIANGLES, numVertices, GL_UNSIGNED_INT, 0);
        } else {
            std::cout << "\n Invalid data structure. Choose \"Separate\" or \"Indexed\" as third program parameter \n";
        }
 
        end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsedSeconds = end - start;
        totalDuration += elapsedSeconds.count();

        // Every 3 seconds, print out avg draw time = sum(duration of frame/single frame). 
        //if(totalDuration - lastPrintDuration > 1.){
        std::cout << "[" << targetModel << ", "<< targetProcessor << ", " << targetDataStructure << ", v: " << numVertices << "] " 
                << " Average frame draw time: " << totalDuration / frames << "\n";
        //    lastPrintDuration = totalDuration;
        //}

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

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float positionDelta = 0.0001f;
    float rotationDelta = 0.01f;
    float scaleDelta = 0.0001f;

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

    updateDeltaFromInput(window, x_position, y_position, z_position, positionDelta, positionKeys);
    updateDeltaFromInput(window, x_rotation, y_rotation, z_rotation, rotationDelta, rotationKeys);
    updateDeltaFromInput(window, x_scale, y_scale, z_scale, scaleDelta, scaleKeys);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}