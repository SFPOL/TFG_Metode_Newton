#pragma once

//STD Libs
#include<iostream>
#include<string>
#include<fstream>
#include<vector>
#include<sstream>
#include<algorithm>

//GLEW
#include<glew.h>

//GLFW
#include<glfw3.h>

//OpenGL Math libs
#include<glm.hpp>
#include<vec3.hpp>
#include<vec4.hpp>
#include<mat4x4.hpp>
#include<gtc/matrix_transform.hpp>
#include<gtc/type_ptr.hpp>

//Own libs
#include"Vertex.h"
#include"Point.h"


class Loader
{
public:
	static bool loadFromtxtFile(const std::string& filePath, std::vector<Point>& vertices, glm::vec3& minVertex, glm::vec3& maxVertex);
	static bool loadFromOBJFile(const std::string& filePath, std::vector<Point>& vertices, glm::vec3& minVertex, glm::vec3& maxVertex);
	static bool loadFromBigEndianPLYFile(const std::string& filePath, std::vector<Point>& vertices, glm::vec3& minVertex, glm::vec3& maxVertex);
	static bool isBigEndian();
	static void swapBytes(char* data, int size);
	static bool loadFromPLYFile(const std::string& filePath, std::vector<Point>& vertices, glm::vec3& minVertex, glm::vec3& maxVertex);
	static bool loadFromASCIIPLYFile(const std::string& filePath, std::vector<Point>& vertices, glm::vec3& minVertex, glm::vec3& maxVertex);
	static glm::vec3 getColor(int index);
private:
	// Hacer el constructor privado para que la clase no pueda ser instanciada
	Loader() {}
};



