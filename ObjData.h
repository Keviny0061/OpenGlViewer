#pragma once
#include <vector>
#include <glm/glm.hpp>

class ObjData {
		public:
			std::vector<glm::vec3> vertices;
			std::vector<glm::vec2> uvs;
			std::vector<glm::vec3> normals;

			std::vector<unsigned int> verticesPerFaceCounts;
			std::vector<unsigned int> vertexIndices;
			std::vector<unsigned int> uvIndices;
			std::vector<unsigned int> normalIndices;
};