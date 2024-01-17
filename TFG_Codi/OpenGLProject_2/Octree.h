#pragma once


#include<glm.hpp>
#include <vector>
#include <memory>
#include <array>
#include <algorithm>
#include"FlattenedNode.h"
#include"Point.h"

using namespace glm;
class OctreeNode {
public:
    OctreeNode(const vec3& minBound, const vec3& maxBound, unsigned int depth, const std::vector<Point>& pts)
        : minBound(minBound), maxBound(maxBound), depth(depth), points(pts) {
        for (auto& child : children) {
            child = nullptr;
        }
    }

    OctreeNode(){
    }

    ~OctreeNode() {
        for (auto& child : children) {
            delete child;
        }
    }

    bool hasChildren() const {
        for (const auto& child : children) {
            if (child) {
                return true;
            }
        }
        return false;
    }

    bool hasSameLabelType(int type) {

        if (type == 0) {
            this->typeNode = -1;
            return false;
        }

        if (points.empty()) {
            return true;
        }

        if (type == 1) {
            int firstLabelType = static_cast<int>(points[0].labels.x);
            for (const Point& point : points) {
                if (static_cast<int>(point.labels.x) != firstLabelType) {
                    return false;
                }
            }
            this->typeNode = firstLabelType;
        }

        if (type == 2) {
            int firstLabelType = static_cast<int>(points[0].labels.y);
            for (const Point& point : points) {
                if (static_cast<int>(point.labels.y) != firstLabelType) {
			        return false;
				}
			}
            this->typeNode = firstLabelType;
        }
        std::cout << this->typeNode << "min " << this->minBound.x <<" " << this->minBound.y << " " << this->minBound.z << std::endl;
        std::cout << this->depth << "max " << this->maxBound.x <<" "<< this->maxBound.y << " " << this->maxBound.z << std::endl;
        return true;
    }

    std::vector<Point> points;
    vec3 minBound;
    vec3 maxBound;
    unsigned int pointIndex;
    unsigned int pointCount;
    unsigned int depth;
    unsigned int altura;
    unsigned int typeNode;
    bool leaf;
    std::array<OctreeNode*, 8> children;
    
    //bool hasError;
};

class Octree {
public:
    Octree(const vec3& minBound, const vec3& maxBound, const std::vector<Point>& points, unsigned int maxDepth, unsigned int maxPointsPerNode, bool childContainingAllPoints, unsigned int type = 0)
        : root(new OctreeNode(minBound, maxBound, 0, points)), maxDepth(maxDepth), childContainingAllPointsActivated(childContainingAllPoints), maxPointsPerNode(maxPointsPerNode) {
        root->pointCount = points.size();
        allNodesInOctree(root);
        build(root, this->flattenedPoints,type);
    }

    std::pair<std::vector<FlattenedNodeFloats>, std::vector<FlattenedNodeInts>> getFlattenedNodes(){
        std::vector<FlattenedNodeFloats> flattenedNodesFloats;
        std::vector<FlattenedNodeInts> flattenedNodesInts;
        flattenOctree(this->root, flattenedNodesFloats, flattenedNodesInts);
        return std::make_pair(flattenedNodesFloats, flattenedNodesInts);
    }

    std::vector<Point> getFlattenedPoints()
    {
        return this->flattenedPoints;
    }

    ~Octree() {
        delete root;
    }

    OctreeNode* root;

    std::vector<OctreeNode*> getLeafNodes() {
        std::vector<OctreeNode*> leafNodes;

        // Método recursivo para recorrer el árbol
        getLeafNodesHelper(this->root, leafNodes);

        return leafNodes;
    }

    void getLeafNodesHelper(OctreeNode* node, std::vector<OctreeNode*>& leafNodes) {
        if (node == nullptr) {
            return;
        }

        if (node->leaf) {
            // Este es un nodo hoja, por lo que lo añadimos a la lista
            leafNodes.push_back(node);
        }
        else {
            // Este es un nodo interno, por lo que llamamos a la función recursivamente en sus hijos
            for (auto& child : node->children) {
                getLeafNodesHelper(child, leafNodes);
            }
        }
    }

private:

    std::vector<Point> flattenedPoints;

    unsigned int maxDepth;
    unsigned int maxPointsPerNode;
    bool childContainingAllPointsActivated;

    int flattenOctree(const OctreeNode* node, std::vector<FlattenedNodeFloats>& flattenedNodesFloats, std::vector<FlattenedNodeInts>& flattenedNodesInts) {
        if (!node) {
            return -1;
        }

        FlattenedNodeFloats flatNodeFloat;
        FlattenedNodeInts flatNodeInt;
        flatNodeFloat.minBound = node->minBound;
        flatNodeFloat.maxBound = node->maxBound;
        flatNodeInt.pointIndex = node->pointIndex;
        flatNodeInt.pointCount = node->pointCount;
        flatNodeInt.depth = node->depth;
        flatNodeInt.extra2 = 0;

        int currentNodeIndex = flattenedNodesInts.size();
        flattenedNodesInts.push_back(flatNodeInt);
        flattenedNodesFloats.push_back(flatNodeFloat);

        for (int i = 0; i < 8; ++i) {
            if (node->children[i]) {
                flatNodeInt.childrenIndices[i] = flattenOctree(node->children[i], flattenedNodesFloats, flattenedNodesInts);
            }
            else {
                flatNodeInt.childrenIndices[i] = -1; // Indica que no hay hijo en esta posición
            }
        }

        // Actualizar el nodo aplanado en la lista con los índices de los hijos correctos
        flattenedNodesInts[currentNodeIndex] = flatNodeInt;
        flattenedNodesFloats[currentNodeIndex] = flatNodeFloat;
        
        return currentNodeIndex; 
    }

    void allNodesInOctree(OctreeNode* node)
    { 
        for (const auto& point : node->points) {
            if (!inBounds(point.position, node->minBound, node->maxBound)) {
                std::cout << "Nodo fuera del OCTREE, aumentar precision" << point.position.x << point.position.y << point.position.z << std::endl;
            }
        }
    }


    void build(OctreeNode* node, std::vector<Point>& flattenedPoints, int type) {
        
        //Noda fulla.
        if (node->depth >= maxDepth || node->points.size() <= this->maxPointsPerNode|| node->hasSameLabelType(type)) {
            node->leaf = true;
            return;
        }
        
        int pointsCount = 0;
        vec3 midPoint = (node->minBound + node->maxBound) * 0.5f;
        int childContainingAllPoints = -1;
        int nChilds = 0;
        for (int i = 0; i < 8; ++i) {
            
            vec3 childMinBound = mix(node->minBound, midPoint, vec3((i & 1) > 0, (i & 2) > 0, (i & 4) > 0));
            vec3 childMaxBound = mix(midPoint, node->maxBound, vec3((i & 1) > 0, (i & 2) > 0, (i & 4) > 0));

            std::vector<Point> childPoints;
            for (const auto& point : node->points) {
                if (inBounds(point.position, childMinBound, childMaxBound)) {
                    childPoints.push_back(point);
                }
            }
            if (!childPoints.empty()) {
                if (childPoints.size() == node->points.size()) {
                    if (this->childContainingAllPointsActivated) {
                        node->leaf = true;
                        return;
                    }
                }
                node->children[i] = new OctreeNode(childMinBound, childMaxBound, node->depth + 1, childPoints);

                nChilds++;
                // Guarda el índice del primer punto en el nodo
                node->children[i]->pointIndex = flattenedPoints.size();

                // Guarda la cantidad de puntos en el nodo
                node->children[i]->pointCount = childPoints.size();

                build(node->children[i], flattenedPoints,type);
                // Verifica si el nodo hijo creado es un nodo hoja
                if (!node->children[i]->hasChildren()) {
                    std::sort(node->children[i]->points.begin(), node->children[i]->points.end(),
                        [](const Point& a, const Point& b) {
                            return a.labels.z < b.labels.z;
                        });

                    // Añade los puntos del nodo hoja a flattenedPoints
                    for (const auto& point : node->children[i]->points) {
                        flattenedPoints.push_back(point);
                    }
                }
            }
            
        }
        if (nChilds == 0) {
            node->leaf = true;
        }
        
    }

    bool inBounds(const vec3& point, const vec3& minBound, const vec3& maxBound, const float epsilon = 1e-6f) {
        return point.x >= minBound.x - epsilon && point.x <= maxBound.x + epsilon &&
            point.y >= minBound.y - epsilon && point.y <= maxBound.y + epsilon &&
            point.z >= minBound.z - epsilon && point.z <= maxBound.z + epsilon;
    }



};


