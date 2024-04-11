#include <string>
#include "ShaderReader.h"

ShaderReader::ShaderReader(const char* vertexPath, const char* fragmentPath) {
    this->vertexPath = vertexPath;
    this->fragmentPath = fragmentPath;
}

// From https://learnopengl.com/Getting-started/Shaders
// Reads shader from file
void ShaderReader::read(ShaderData& data) {
    // 1. retrieve the vertex/fragment source code from filePath
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;

    // ensure ifstream objects can throw exceptions:
    vShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
	
    try {
        // open files
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);

        // read file's buffer contents into streams
        std::stringstream vShaderStream, fShaderStream;
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();	

        // close file handlers
        vShaderFile.close();
        fShaderFile.close();

        // convert stream into string
        data.vertexShaderCode = vShaderStream.str();
        data.fragmentShaderCode = fShaderStream.str(); 
        errorFlag = false;
    }
    catch(std::ifstream::failure e) {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
        errorFlag = true;
    }
}