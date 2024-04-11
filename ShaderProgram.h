#pragma once
#include "ShaderData.h"

class ShaderProgram {
    public:
        // constructor reads and builds the shader
        ShaderProgram(ShaderData& data);
		~ShaderProgram();
		void use();

        bool wasError() { return errorFlag; }
		unsigned int getID() { return ID; }

    private:
		unsigned int ID;
        bool errorFlag;
};