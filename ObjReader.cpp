#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <set>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "ObjReader.h"


// Parse Wavefront .obj file
// https://en.wikipedia.org/wiki/Wavefront_.obj_file
void ObjReader::readObjAsIndexed(std::string objName, ObjData& outData, bool breakIntoTris){
	

	std::string targetFile("../data/objects/" + objName + ".obj");
	std::ifstream inStream(targetFile);

	std::string currentLine;
	std::string token;
	std::string partialToken;
	std::string delimiter("/");

	int verticesPerFaceCount;
	while (getline(inStream, currentLine)) {
		// Output the text from the file
  	 	
		std::stringstream currentLineStream(currentLine);
		currentLineStream >> token;

		if(token.compare("v") == 0){
			glm::vec3 vertex;
			currentLineStream >> token;
			vertex.x = std::stof(token);
			currentLineStream >> token;
			vertex.y = std::stof(token);
			currentLineStream >> token;
			vertex.z = std::stof(token);
			outData.vertices.push_back(vertex);
			
		} else if(token.compare("vt") == 0){
			glm::vec2 uv;
			currentLineStream >> token;
			uv.x = std::stof(token);
			currentLineStream >> token;
			uv.y = std::stof(token);
			outData.uvs.push_back(uv);

		} else if(token.compare("vn") == 0){
			glm::vec3 normal;
			currentLineStream >> token;
			normal.x = std::stof(token);
			currentLineStream >> token;
			normal.y = std::stof(token);
			currentLineStream >> token;
			normal.z = std::stof(token);
			outData.normals.push_back(normal);

		} else if(token.compare("f") == 0){
			std::vector<Face> allFaces;

			Face readInFace;
			while(currentLineStream >> token){
				Attribute attribute;
				parseVertexAttribute(token, attribute);
				readInFace.attributes.push_back(attribute);
				
				
			}
			
			

			if(breakIntoTris){
				breakFaceIntoTris(readInFace, allFaces);
			} else{
				allFaces.push_back(readInFace);
			}

			// Store
			for(Face& currentFace : allFaces){
				

				for(Attribute& currentAttribute : currentFace.attributes){
					// Subtract 1 from final since obj is 1 indexed and arrays are 0 indexed. 
					// OpenGL matches arrays starting from index 0.
					outData.vertexIndices.push_back(currentAttribute.vertexIndex-1);
					outData.uvIndices.push_back(currentAttribute.uvIndex-1);
					outData.normalIndices.push_back(currentAttribute.normalIndex-1);
					
				}

				outData.verticesPerFaceCounts.push_back(currentFace.attributes.size());
				
			}
		}
	}

	// Close the file
	inStream.close(); 
	

	return;
}

void ObjReader::indexedToSeparateTriangles(const ObjData& inData, ObjData& outData){

	
	for(int i = 0; i < inData.vertexIndices.size(); i++){
		unsigned int vertexIndex = inData.vertexIndices[i];
		const glm::vec3& vert = inData.vertices[vertexIndex];
		outData.vertices.push_back(vert);	
	}

	
	/**/for (int i = 0; i < inData.normalIndices.size(); i++) {
		try {
			unsigned int normalIndex = inData.normalIndices.at(i);
			const glm::vec3& normal = inData.normals.at(normalIndex);
			outData.normals.push_back(normal);	
		}
		catch(...) {
			std::cerr << "Error: Reading Obj File" << std::endl;
			exit(1);
		}
	}
}




// Scale down object to [-1,-1] and [1,1] bounds
void ObjReader::scaleToClipCoords(ObjData& data){
	// Find vertices with min and max distance from (0,0)
	// Scale down model by ratio of distances
	// Ex. dist(min coord (1,1,1)) = 1, dist(max coord (6,6,6)) = 10.3, so scale by 1/10.3 where max coord is now at max (1,1,1)
	if(data.vertices.size() == 0){
		return;
	}

	// Find min and max distance
	float maxDistance = glm::length(data.vertices[0]);
	for(glm::vec3& vert : data.vertices){
		float distance = glm::length(vert);
		if(distance > maxDistance){
			maxDistance = distance;
		}
	}

	// Manual CPU clip coord scale
	glm::mat4 ortho = glm::ortho(-maxDistance, maxDistance, -maxDistance, maxDistance, -maxDistance, maxDistance);
	for(glm::vec3& vert : data.vertices){
		vert = ortho * glm::vec4(vert, 1);
	}
}

// Parse #vertex_index/#texture_index/#normal_index
void ObjReader::parseVertexAttribute(std::string& token, Attribute& outAttribute){
	size_t pos = 0;
	int i = 0;
	
	std::stringstream tokenStream(token);
	std::string partialToken;
	while(getline(tokenStream, partialToken, '/')){
		if(partialToken.length() > 0){
			unsigned int value = stoi(partialToken);

			if(i == 0){ // Vertex
				outAttribute.vertexIndex = value;
			} else if(i == 1){ // Texture/UV
				outAttribute.uvIndex = value;
			} else if(i == 2){ // Normal
				outAttribute.normalIndex = value;
			}
		} 

		// Next attribute in vertex
		i++;
	}
}

void ObjReader::breakFaceIntoTris(const Face& face, std::vector<Face>& outFaces){
	if(face.attributes.size() == 3){
		// Triangles have 3 vertices. Just return that.
		outFaces.push_back(face);
	} else if(face.attributes.size() == 4) {
		// Quads have 4 vertices
		// Break into 2 triangles.
		// Points 1, 2, 3 in first triangle
		// Points 1, 3, 4 in second triangle.  
		Face face1;
		Face face2;

		face1.attributes.push_back(face.attributes[0]);
		face1.attributes.push_back(face.attributes[2]);
		face1.attributes.push_back(face.attributes[3]);
		
		face2.attributes.push_back(face.attributes[0]);
		face2.attributes.push_back(face.attributes[1]);
		face2.attributes.push_back(face.attributes[2]);

		outFaces.push_back(face1);
		outFaces.push_back(face2);
	}
}
