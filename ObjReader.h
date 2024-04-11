#pragma once
#include <string>
#include "ObjData.h"

class ObjReader {
	public:
		void readObjAsIndexed(std::string objName, ObjData& outData, bool breakIntoTris);
		void indexedToSeparateTriangles(const ObjData& inData, ObjData& outData);
		void separateTrianglesToIndexed(const ObjData& inData, ObjData& outData);
		void scaleToClipCoords(ObjData& data);

	private:
		class Attribute{
			public:
				unsigned int vertexIndex;
				unsigned int uvIndex;
				unsigned int normalIndex;

		};

		class Face {
			public:
				std::vector<Attribute> attributes;
		};

		void parseVertexAttribute(std::string& token, Attribute& outAttribute);
		void breakFaceIntoTris(const Face& face, std::vector<Face>& outFaces);
};