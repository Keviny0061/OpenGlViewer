#define GLEW_STATIC
#include <GL/glew.h>
#include <iostream>

#include "ShaderProgram.h"


// From https://learnopengl.com/Getting-started/Shaders
// On contruction, links and compiles shader data through OpenGL
ShaderProgram::ShaderProgram(ShaderData& shaderData) { 
	// 2. compile shaders
	errorFlag = false;
	int success;
	char infoLog[512];
	
	// vertex Shader
	const char* vertexShaderCode = shaderData.vertexShaderCode.c_str();
	unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &vertexShaderCode, NULL);
	glCompileShader(vertex);

	// print compile errors if any
	glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
	if(!success) {
		glGetShaderInfoLog(vertex, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
		errorFlag = true;
	};
	
	// similiar for Fragment Shader
	const char* fragmentShaderCode = shaderData.fragmentShaderCode.c_str();
	unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &fragmentShaderCode, NULL);
	glCompileShader(fragment);

	// print compile errors if any
	glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
	if(!success) {
		glGetShaderInfoLog(fragment, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
		errorFlag = true;
	};
	
	// shader Program
	ID = glCreateProgram();
	glAttachShader(ID, vertex);
	glAttachShader(ID, fragment);
	glLinkProgram(ID);

	// print linking errors if any
	glGetProgramiv(ID, GL_LINK_STATUS, &success);
	if(!success)
	{
		glGetProgramInfoLog(ID, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
		errorFlag = true;
	}
	
	// delete the shaders as they're linked into our program now and no longer necessary
	glDeleteShader(vertex);
	glDeleteShader(fragment);
}

ShaderProgram::~ShaderProgram(){
    glDeleteProgram(ID);
}

// One use, OpenGL will use program
void ShaderProgram::use(){
	glUseProgram(ID);
}
