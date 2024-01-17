#pragma once

#include<iostream>
#include<vector>
#include<sstream>
#include <tuple>

#include"Vertex.h"
#include"Shader.h"
#include"Material.h"
#include "Primitives.h"
#include "Octree.h"
#include "MultiOctree.h"
#include"FlattenedNode.h"
#include"FlattenedMultiNode.h"
#include"Point.h"
#include "Loader.h"

class MultiPointCloud
{
private:
	//Id del pointCloud
	GLuint id;

	//Variables de puntos
	std::vector<Point> vertices;
	std::vector<Point> verticesError;
	std::vector<Point> verticesExtra;
	Vertex* vertexArray;

	int octreeMaxDepth;
	int octreeMaxPointsPerNode;
	bool childContainingAllPoints;
	int ntotalPoints;

	//Imagen 3D final.
	std::vector<std::vector<std::vector<std::vector<GLubyte>>>> image;
	unsigned nrOfVertices;

	//Variables para el shader
	float alphaError;
	float alphaGround;
	float alphaExtra;
	float radiusExtra;
	float radiusError;
	float radius;

	//Variables para la voxelicacion
	int totalSize;
	ivec3 size_grid;
	glm::vec3 uVoxelDimensions;

	//Variables minima i maxima de los vertices
	glm::vec3 uMinVertex;
	glm::vec3 uMaxVertex;

	//Para intentar hacer el rayo que intersecta para analizar el punto.
	GLuint ssbo;
	Point* point;

	//Gluint para la conexion con el shader.
	GLuint flattenedMultiNodesBufferFloats;
	GLuint flattenedMultiNodesBufferInts;
	GLuint flattenedMultiPoints1Buffer;
	GLuint flattenedMultiPoints2Buffer;
	GLuint flattenedMultiPoints3Buffer;
	GLuint flattenedMultiNodesTextureInts;
	GLuint flattenedMultiNodesTextureFloats;
	GLuint flattenedMultiPoints1Texture;
	GLuint flattenedMultiPoints2Texture;
	GLuint flattenedMultiPoints3Texture;

	std::vector<FlattenedMultiNodeIntsSend3> flattenedNodesIntsToSend;
	std::vector<Point> flattenedPoints1;
	std::vector<Point> flattenedPoints2;
	std::vector<Point> flattenedPoints3;

	//Variables de los 3 planos que rodean al MultiPointCloud
	glm::vec3 planeOrigin;
	glm::vec3 range;
	float planeSize;
	float scalePlaneFactor;

	//MultiOctree
	MultiOctree* multiOctree;
	std::string filePath;

	//Metriques
	float accuracy;
	float IoU;
	int numberOfMisclasifications;

	void initMetrics() {
		int total = vertices.size() + verticesError.size();
		int correct = vertices.size();
		int misclassifications = verticesError.size();

		this->accuracy = static_cast<float>(correct) / static_cast<float>(total);
		this->numberOfMisclasifications = misclassifications;
		int intersection = 0;
		for (auto& point : verticesError)
		{
			if (point.labels[2] == 1.0)
			{
				intersection++;
			}
		}

		// IoU is the ratio of intersection to union of ground truth and predicted labels
		IoU = static_cast<float>(intersection) / static_cast<float>(total - intersection);

	}

	/**
	 * Initialize variables needed for scaling and positioning the model in the voxel grid
	 * @param totalSize - Total size of the model
	 * @param scale - Scale factor of the model
	 */
	void initVariables(int totalSize, float scale, int octreeMaxDepth,	int octreeMaxPointsPerNode,	bool childContainingAllPoints)
	{
		this->octreeMaxDepth = octreeMaxDepth;
		this->octreeMaxPointsPerNode = octreeMaxPointsPerNode;
		this->childContainingAllPoints = childContainingAllPoints;
		this->ntotalPoints = vertices.size();
		
		// Initialize the radius
		this->radius = 5;
		this->radiusExtra = 1.5f;
		this->radiusError = 1;

		// Initialize the alpha error
		this->alphaError = 0.1;

		// Initialize the ground alpha
		this->alphaGround = 0.3;

		// Set common variables
		this->totalSize = totalSize;
		this->scalePlaneFactor = 3.f;

		// Compute the size of the object
		glm::vec3 objectSize = this->uMaxVertex - this->uMinVertex;

		// Get the maximum size dimension of the object
		float maxObjectSize = glm::max(glm::max(objectSize.x, objectSize.y), objectSize.z);

		// Compute the scaling factor for the model
		float scalingFactor = totalSize * scale / maxObjectSize;

		// Scale each vertex of the model
		for (Point& vertex : vertices)
		{
			vertex.position *= scalingFactor;
		}

		// Scale the bounding box of the model
		this->uMinVertex *= scalingFactor;
		this->uMaxVertex *= scalingFactor;

		// Recalculate the object size after scaling
		objectSize = this->uMaxVertex - this->uMinVertex;

		// Set the voxel dimensions
		this->uVoxelDimensions = vec3(scale);

		// Calculate the grid size based on the object size and scale
		this->size_grid = glm::ivec3(glm::ceil(objectSize / scale));
	}

	/**
	 * Populate an image representation of the voxel grid, with each voxel color coded based on the vertices it contains.
	 */
	void populateImage()
	{
		// Initialize the 3D image as black (4 channels: R, G, B, A with each initialized to 0)
		std::vector<std::vector<std::vector<std::vector<GLubyte>>>> image(
			this->size_grid[2],
			std::vector<std::vector<std::vector<GLubyte>>>(
				this->size_grid[1],
				std::vector<std::vector<GLubyte>>(
					this->size_grid[0],
					std::vector<GLubyte>(4, 0)
				)
			)
		);

		// Initialize a mask image
		std::vector<std::vector<std::vector<GLubyte>>> maskImage(
			size_grid[2],
			std::vector<std::vector<GLubyte>>(
				size_grid[1],
				std::vector<GLubyte>(size_grid[0], 0)
			)
		);

		// Assign red color to voxels where there's a vertex
		for (const Point& vertex : this->vertices)
		{
			// Calculate voxel coordinates for each vertex, taking into account voxel dimensions
			glm::ivec3 voxelCoords = glm::ivec3((vertex.position - this->uMinVertex) / this->uVoxelDimensions);

			// Clamp voxel coordinates to ensure they're within the grid
			voxelCoords = glm::clamp(voxelCoords, glm::ivec3(0), glm::ivec3(this->totalSize - 1));

			// Scale the vertex color from [0,1] to [0,255]
			ivec3 colorInt = ivec3(vertex.color * 255.0f + 0.5f);

			// Assign vertex color to corresponding voxel in the image
			image[voxelCoords.z][voxelCoords.y][voxelCoords.x][0] = colorInt.x; // Red
			image[voxelCoords.z][voxelCoords.y][voxelCoords.x][1] = colorInt.y; // Green
			image[voxelCoords.z][voxelCoords.y][voxelCoords.x][2] = colorInt.z; // Blue

			// Assign opacity (alpha value). Here it's constant (50), could be based on other criteria
			image[voxelCoords.z][voxelCoords.y][voxelCoords.x][3] = 50;
		}

		// Update the member variable to hold the populated image
		this->image = image;
	}

	void initSSBO()
	{
		glGenBuffers(1, &this->ssbo);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->ssbo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Point), this->point, GL_DYNAMIC_COPY);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, this->ssbo);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	/**
 * Initialize a 3D texture from multiple point clouds
 */
	void initMultiPointCloudAs3DTexture()
	{
		// Generate and bind a new texture ID
		glGenTextures(1, &this->id);
		glBindTexture(GL_TEXTURE_3D, this->id);

		// Set the texture parameters
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		// Create a flat image from the 3D texture data
		std::vector<GLubyte> flatImage(this->size_grid[0] * this->size_grid[1] * this->size_grid[2] * 4);
		for (int z = 0; z < this->size_grid[2]; z++)
		{
			for (int y = 0; y < this->size_grid[1]; y++)
			{
				for (int x = 0; x < this->size_grid[0]; x++)
				{
					for (int c = 0; c < 4; c++)
					{
						flatImage[((z * this->size_grid[1] + y) * this->size_grid[0] + x) * 4 + c] = this->image[z][y][x][c];
					}
				}
			}
		}

		// Create the 3D texture using the flat image data
		glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, this->size_grid[0], this->size_grid[1], this->size_grid[2], 0, GL_RGBA, GL_UNSIGNED_BYTE, flatImage.data());

		// Unbind the texture
		glActiveTexture(0);
		glBindTexture(GL_TEXTURE_3D, 0);
	}

	/**
	 * Initialize a multi octree from multiple point clouds
	 */
	void initMultiPointCloudAsMultiOctree()
	{
		// Flatten nodes and points from the multiOctree
		std::vector<FlattenedMultiNodeFloats> flattenedNodesFloats;
		std::vector<FlattenedMultiNodeInts> flattenedNodesInts;
		std::tie(flattenedNodesFloats, flattenedNodesInts) = this->multiOctree->getFlattenedNodesMultiOctree(3);
		flattenedPoints1 = this->multiOctree->getFlattenedPoints(0);
		
		flattenedPoints2= this->multiOctree->getFlattenedPoints(1);
		flattenedPoints3 = this->multiOctree->getFlattenedPoints(2);

		// Convert FlattenedMultiNodeInts to FlattenedMultiNodeIntsSend2 format for sending
		for (int i = 0; i < flattenedNodesInts.size(); i++) {
			FlattenedMultiNodeInts flattenedMultiNodeInts = flattenedNodesInts[i];
			FlattenedMultiNodeIntsSend3 flattenedMultiNodeIntsSend3;

			// Copiar la información de pointIndex, pointCount, depends, leaf a las variables pointInfo
			for (int j = 0; j < 3; ++j) {
				glm::ivec4& pointInfo = j == 0 ? flattenedMultiNodeIntsSend3.pointInfo1 :
					j == 1 ? flattenedMultiNodeIntsSend3.pointInfo2 :
					flattenedMultiNodeIntsSend3.pointInfo3;
				pointInfo.x = flattenedMultiNodeInts.octreeInfo[j].pointIndex;
				pointInfo.y = flattenedMultiNodeInts.octreeInfo[j].pointCount;
				pointInfo.z = flattenedMultiNodeInts.octreeInfo[j].depends ? 1 : 0;
				pointInfo.w = flattenedMultiNodeInts.octreeInfo[j].leaf ? 1 : 0;
			}

			// Copiar la información de los índices de los hijos a las variables childrenInfo1 y childrenInfo2
			flattenedMultiNodeIntsSend3.childrenInfo1 = glm::ivec4(flattenedMultiNodeInts.childrenIndices[0], flattenedMultiNodeInts.childrenIndices[1], flattenedMultiNodeInts.childrenIndices[2], flattenedMultiNodeInts.childrenIndices[3]);
			flattenedMultiNodeIntsSend3.childrenInfo2 = glm::ivec4(flattenedMultiNodeInts.childrenIndices[4], flattenedMultiNodeInts.childrenIndices[5], flattenedMultiNodeInts.childrenIndices[6], flattenedMultiNodeInts.childrenIndices[7]);

			// Añadir la estructura convertida al vector flattenedNodesIntsToSend2
			flattenedNodesIntsToSend.push_back(flattenedMultiNodeIntsSend3);
		}

		// Create and configure the buffer for flattened float nodes
		glGenBuffers(1, &this->flattenedMultiNodesBufferFloats);
		glBindBuffer(GL_TEXTURE_BUFFER, this->flattenedMultiNodesBufferFloats);
		glBufferData(GL_TEXTURE_BUFFER, flattenedNodesFloats.size() * sizeof(FlattenedMultiNodeFloats), flattenedNodesFloats.data(), GL_STATIC_DRAW);

		// Create and configure the buffer for flattened integer nodes
		glGenBuffers(1, &this->flattenedMultiNodesBufferInts);
		glBindBuffer(GL_TEXTURE_BUFFER, this->flattenedMultiNodesBufferInts);
		glBufferData(GL_TEXTURE_BUFFER, flattenedNodesIntsToSend.size() * sizeof(FlattenedMultiNodeIntsSend3), flattenedNodesIntsToSend.data(), GL_STATIC_DRAW);

		// Create and configure the buffer for flattened points // their positions
		glGenBuffers(1, &this->flattenedMultiPoints1Buffer);
		glBindBuffer(GL_TEXTURE_BUFFER, this->flattenedMultiPoints1Buffer);
		glBufferData(GL_TEXTURE_BUFFER, flattenedPoints1.size() * sizeof(Point), flattenedPoints1.data(), GL_STATIC_DRAW);

		glGenBuffers(1, &this->flattenedMultiPoints2Buffer);
		glBindBuffer(GL_TEXTURE_BUFFER, this->flattenedMultiPoints2Buffer);
		glBufferData(GL_TEXTURE_BUFFER, flattenedPoints2.size() * sizeof(Point), flattenedPoints2.data(), GL_STATIC_DRAW);

		glGenBuffers(1, &this->flattenedMultiPoints3Buffer);
		glBindBuffer(GL_TEXTURE_BUFFER, this->flattenedMultiPoints3Buffer);
		glBufferData(GL_TEXTURE_BUFFER, flattenedPoints3.size() * sizeof(Point), flattenedPoints3.data(), GL_STATIC_DRAW);

		// Create and configure the texture buffer for flattened nodes

		// Create and configure the texture buffer for flattened float nodes
		glGenTextures(1, &this->flattenedMultiNodesTextureFloats);
		glBindTexture(GL_TEXTURE_BUFFER, this->flattenedMultiNodesTextureFloats);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, this->flattenedMultiNodesBufferFloats);

		// Create and configure the texture buffer for flattened integer nodes
		glGenTextures(1, &this->flattenedMultiNodesTextureInts);
		glBindTexture(GL_TEXTURE_BUFFER, this->flattenedMultiNodesTextureInts);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32I, this->flattenedMultiNodesBufferInts);

		// Create and configure the texture buffer for flattened points

		glGenTextures(1, &this->flattenedMultiPoints1Texture);
		glBindTexture(GL_TEXTURE_BUFFER, this->flattenedMultiPoints1Texture);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, this->flattenedMultiPoints1Buffer);

		glGenTextures(1, &this->flattenedMultiPoints2Texture);
		glBindTexture(GL_TEXTURE_BUFFER, this->flattenedMultiPoints2Texture);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, this->flattenedMultiPoints2Buffer);

		glGenTextures(1, &this->flattenedMultiPoints3Texture);
		glBindTexture(GL_TEXTURE_BUFFER, this->flattenedMultiPoints3Texture);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, this->flattenedMultiPoints3Buffer);

		glBindTexture(GL_TEXTURE_BUFFER, 0);
	}


	// updateUniforms: This method updates the uniforms in the shader program.
	void updateUniforms(Shader* shader)
	{
		// Set the uniforms in the shader
		shader->set1f(this->radius, "radius");
		shader->set1f(this->radiusError, "radiusError");
		shader->set1f(this->radius, "radiusExtra");
		shader->set1f(this->alphaGround, "alphaGround");
		shader->set1f(this->alphaError, "alphaError");
		shader->setVec3f(this->uVoxelDimensions, "uVoxelDimensions");
		shader->setVec3i(this->size_grid, "grid_dim");
		shader->setVec3f(this->uMinVertex, "uMinVertex");
		shader->setVec3f(this->uMaxVertex, "uMaxVertex");
		updatePlane();
		shader->setVec3f(this->planeOrigin, "planeOrigin");
		shader->set1f(this->planeSize, "planeSize");
		shader->setVec3f(this->range, "range");
	}


	// loadFile: This method loads a 3D model from a file.
	void loadFile(std::string filePath) {
		glm::vec3 minVertex, maxVertex;

		// Determine file extension
		std::string extension = filePath.substr(filePath.find_last_of(".") + 1);

		if (extension == "obj")
		{
			if (!Loader::loadFromOBJFile(filePath, this->vertices, minVertex, maxVertex))
			{
				throw std::runtime_error("Error: Could not load OBJ file.");
			}
		}
		else if (extension == "ply")
		{
			if (!Loader::loadFromPLYFile(filePath, this->vertices, minVertex, maxVertex))
			{
				throw std::runtime_error("Error: Could not load PLY file.");
			}
		}
		else if (extension == "txt")
		{
			if (!Loader::loadFromtxtFile(filePath, this->vertices, minVertex, maxVertex))
			{
				throw std::runtime_error("Error: Could not load txt file.");
			}
		}
		else
		{
			throw std::runtime_error("Error: Unsupported file type.");
		}
		this->filePath = filePath;
		this->uMinVertex = minVertex;
		this->uMaxVertex = maxVertex;
	}


	// Initializes voxel grid by populating image and initializing multi-point cloud as 3D texture.
	void initVoxelGrid() {
		this->populateImage();
		this->initMultiPointCloudAs3DTexture();
	}

	// Initializes octree and merges octree based on the number of Octrees.
	void initOctree(int nOctrees) {
		// Initialize MultiOctree
		if (nOctrees  == 1) {
			this->multiOctree = new MultiOctree(this->uMinVertex, this->uMaxVertex, this->vertices, this->octreeMaxDepth, this->octreeMaxPointsPerNode, this->childContainingAllPoints, 0);
		}
		// Checks and processes if number of octrees is more than or equal to 2
		if (nOctrees == 2) {
			// Loop through vertices
			for (int i = 0; i < vertices.size(); i++) {
				// Check if the vertex has a zero in the third component of 'labels'
				if (vertices[i].labels.z == 0.0f) {
					// Add the vertex to 'verticesError'
					verticesError.push_back(vertices[i]);
					// Remove the vertex from 'vertices'
					vertices.erase(vertices.begin() + i);
					i--; // Decrement the position because the vector shortens
				}
			}
			this->multiOctree = new MultiOctree(this->uMinVertex, this->uMaxVertex, this->vertices, this->octreeMaxDepth, this->octreeMaxPointsPerNode, this->childContainingAllPoints, 0);
			// Merge the Octree
			this->multiOctree->mergeOctree(this->uMinVertex, this->uMaxVertex, this->verticesError, this->octreeMaxDepth, this->octreeMaxPointsPerNode, this->childContainingAllPoints, 0);
		}

		// Checks and processes if number of octrees is more than or equal to 3
		if (nOctrees == 3) {
			// Loop through vertices
			for (int i = 0; i < vertices.size(); i++) {
				// Check if the vertex has a zero in the third component of 'labels'
				if (vertices[i].labels.z == 0.0f) {
					// Add the vertex to 'verticesError'
					verticesError.push_back(vertices[i]);
					// Remove the vertex from 'vertices'
					vertices.erase(vertices.begin() + i);
					i--; // Decrement the position because the vector shortens
				}
			}
			for (int i = 0; i < vertices.size(); i++) {
				// Check if the vertex has a one in the first component of 'labels'
				if (vertices[i].labels.x == 1.0f) {
					// Add the vertex to 'verticesExtra'
					verticesExtra.push_back(vertices[i]);
					// Remove the vertex from 'vertices'
					vertices.erase(vertices.begin() + i);
					i--; // Decrement the position because the vector shortens
				}
			}
			
			this->multiOctree = new MultiOctree(this->uMinVertex, this->uMaxVertex, this->vertices, this->octreeMaxDepth, this->octreeMaxPointsPerNode, this->childContainingAllPoints, 0);
			// Merge the Octree
			this->multiOctree->mergeOctree(this->uMinVertex, this->uMaxVertex, this->verticesError, this->octreeMaxDepth, this->octreeMaxPointsPerNode, this->childContainingAllPoints, 0);
			// Merge the Octree
			this->multiOctree->mergeOctree(this->uMinVertex, this->uMaxVertex, this->verticesExtra, this->octreeMaxDepth, this->octreeMaxPointsPerNode, this->childContainingAllPoints, 0);
		}

		// Initialize MultiPointCloud as MultiOctree
		bool a = this->multiOctree->checkNodeIntegrity();
		this->initMultiPointCloudAsMultiOctree();
	}

public:

	// Constructor of MultiPointCloud. Initializes variables, voxel grid and octree based on the parameters.
	MultiPointCloud(int id, const std::string& filePath, 
		int nOctrees = 3, 
		int octreeMaxDepth = 7,
		int octreeMaxPointsPerNode = 2,
		bool childContainingAllPoints = false,
		int totalSize = 150, 
		float scale = 0.5f,
		bool voxelize = true, 
		bool octree = true)
	{
		this->id = id;
		this->loadFile(filePath);
		this->initVariables(totalSize, scale, octreeMaxDepth, octreeMaxPointsPerNode, childContainingAllPoints);
		if (voxelize)
			this->initVoxelGrid();

		if (octree)
			this->initOctree(nOctrees);

		this->initMetrics();
	}

	// Returns the id of MultiPointCloud.
	GLuint getID() const
	{
		return this->id;
	};

	// Initializes planes. Updates plane and sets plane variables in the shader.
	void initPlanes(Shader* shader, float scaleFactor = 3.2f)
	{
		this->scalePlaneFactor = scaleFactor;
		this->updatePlane();
		shader->setVec3f(this->planeOrigin, "planeOrigin");
		shader->set1f(this->planeSize, "planeSize");
		shader->setVec3f(this->range, "range");
	}

	// Updates plane. Scales plane factor, sets plane size and calculates range.
	void updatePlane()
	{
		glm::vec3 center = (uMaxVertex + uMinVertex) / 2.0f;
		glm::vec3 objectSize = uMaxVertex - uMinVertex;
		float maxSize = glm::max(glm::max(objectSize.x, objectSize.y), objectSize.z);
		this->planeSize = maxSize * this->scalePlaneFactor;
		glm::vec3 halfPlaneSize = glm::vec3(planeSize * 0.5f);
		this->planeOrigin = center - halfPlaneSize;
		this->range = glm::vec3(planeSize);
	}

	float* getScalePlaneFactor() {
		return &this->scalePlaneFactor;
	}


	

	// Binds 3D texture to GL_TEXTURE7.
	void bind3DTexture()
	{
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_3D, this->id);
	};

	// Unbinds 3D texture from GL_TEXTURE7.
	void unbind()
	{
		glActiveTexture(0);
		glBindTexture(GL_TEXTURE_3D, this->id);
	}

	// Binds buffer and maps it to access data.
	void bindBuffer()
	{
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
		Point* p = (Point*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(Point), GL_MAP_READ_BIT);

		if (p)
		{
			std::cout << "Point Position: " << p->position.x << ", " << p->position.y << ", " << p->position.z << std::endl;
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		}

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	};

	// Binds buffers for MultiOctree.
	void bindBuffersMultiOctree(int n = 1)
	{
		this->bindBufferMultiNodesFloat();
		this->bindBufferMultiNodesInt();
		this->bindBufferMultiPoints1();
		if (n >= 2)
			this->bindBufferMultiPoints2();
		if (n >= 3)
			this->bindBufferMultiPoints3();
	}

	// Bind functions for each buffer.
	void bindBufferMultiPoints1()
	{
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_BUFFER, this->flattenedMultiPoints1Texture);
	};

	void bindBufferMultiPoints2()
	{
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_BUFFER, this->flattenedMultiPoints2Texture);
	};

	void bindBufferMultiPoints3()
	{
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_BUFFER, this->flattenedMultiPoints3Texture);
	};

	void bindBufferMultiNodesFloat()
	{
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_BUFFER, this->flattenedMultiNodesTextureFloats);
	};

	void bindBufferMultiNodesInt()
	{
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_BUFFER, this->flattenedMultiNodesTextureInts);
	};

	int getOctreeDepth()
	{
		return this->multiOctree->getMaxDepth();
	}

	int getOctreeNumberOctrees()
	{
		return this->multiOctree->getOctreeNumberOctrees();
	}

	int getOctreeSize()
	{
		return this->flattenedNodesIntsToSend.size();
	}


	void update()
	{

	}

	void render(Shader* shader)
	{

		//Update uniforms
		this->updateUniforms(shader);

		shader->use();

		//Cleanup
		glBindVertexArray(0);
		glUseProgram(0);
		glActiveTexture(0);
		glBindTexture(GL_TEXTURE_BUFFER, 0);
	}


	~MultiPointCloud()
	{

		delete[] this->vertexArray;

		//delete image


		glDeleteTextures(1, &this->id);

		glDeleteTextures(1, &this->flattenedMultiNodesTextureFloats);
		glDeleteTextures(1, &this->flattenedMultiNodesTextureInts);
		glDeleteTextures(1, &this->flattenedMultiPoints1Texture);
		glDeleteTextures(1, &this->flattenedMultiPoints2Texture);
		glDeleteTextures(1, &this->flattenedMultiPoints3Texture);
		delete this->multiOctree;

	}



	float* getRadius()
	{
		return &this->radius;
	}

	float* getRadiusError()
	{
		return &this->radiusError;
	}

	float* getAlphaError()
	{
		return &this->alphaError;
	}

	float* getAlphaGround()
	{
		return &this->alphaGround;
	}

	void addRadius(const float& dt)
	{
		float newRad = this->radius + dt;
		newRad = max(0.01f, min(newRad, 10.0f));
		this->radius = newRad;

	}

	int getNodesPerOctree(int i) {
		return this->multiOctree->getNodesPerOctree(i);
	}

	vec3 getCenter() const
	{
		return (this->uMaxVertex + this->uMinVertex) / 2.0f;
	}


	void SetMinVertex(const glm::vec3& minVertex) {
		uMinVertex = minVertex;
	}

	void SetMaxVertex(const glm::vec3& maxVertex) {
		uMaxVertex = maxVertex;
	}

	const glm::vec3& GetMinVertex() const {
		return uMinVertex;
	}

	const glm::vec3& GetMaxVertex() const {
		return uMaxVertex;
	}

	const std::string& GetFilePath() const {
		return filePath;
	}


	int getOctreeMaxDepth() {
		return this->octreeMaxDepth;
	}

	int getOctreeMaxPointsPerNode() {
		return this->octreeMaxPointsPerNode;
	}

	bool getChildContainingAllPoints() {
		return this->childContainingAllPoints;
	}

	int getOctreePointsSize() {
		return this->ntotalPoints;
	}

	int getOctree1PointsSize() {
		return this->flattenedPoints1.size();
	}

	int getOctree2PointsSize() {
		return this->flattenedPoints2.size();
	}

	int getOctree3PointsSize() {
		return this->flattenedPoints3.size();
	}


};