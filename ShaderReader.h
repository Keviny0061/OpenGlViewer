#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>  

#include "ShaderData.h"

class ShaderReader {
    public:
        ShaderReader(const char* vertexPath, const char* fragmentPath);
        void read(ShaderData& data);
        bool wasError(){ return errorFlag; }

    private:
        bool errorFlag;
        const char* vertexPath;
        const char* fragmentPath;
};