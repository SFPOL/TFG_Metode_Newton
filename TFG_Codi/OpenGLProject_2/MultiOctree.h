#pragma once


#include<glm.hpp>
#include <vector>
#include <memory>
#include <array>
#include <algorithm>
#include "Octree.h"
#include"FlattenedNode.h"
#include"FlattenedMultiNode.h"
#include"Point.h"
#include <map>
#include <queue>


using namespace glm;

struct OctreeData {
    std::vector<Point> points;
    unsigned int pointIndex;
    unsigned int pointCount;
    bool depends;
    bool leaf;
    unsigned int typeNode;
};

class OctreeNodeMulti {
public:
    OctreeNodeMulti(const vec3& minBound, const vec3& maxBound, unsigned int depth)
        : minBound(minBound), maxBound(maxBound), depth(depth) {
        for (auto& child : children) {
            child = nullptr;
        }
    }

    ~OctreeNodeMulti() {
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

    // Añade aquí funciones para manejar los datos del octree como las necesites

    vec3 minBound;
    vec3 maxBound;
    unsigned int depth;
    bool isLeafForAnyOctree;
    unsigned int altura;
    std::array<OctreeNodeMulti*, 8> children;

    // Aquí es donde almacenamos los datos para cada octree
    std::map<int, OctreeData> octreeDataMap;
};

class MultiOctree{
public:
    MultiOctree(const Octree* octree) {
        // Construir el Octree Multi con base en el Octree existente
        root = new OctreeNodeMulti(octree->root->minBound, octree->root->maxBound, 0);
        constructMultiOctree(root, octree->root);
        this->updateFlattenedPointsPerOctree();
        this->nOctrees++;
    }

    MultiOctree(const vec3& minBound, const vec3& maxBound, const std::vector<Point>& points, unsigned int maxDepth, unsigned int maxPointsPerNode, bool childContainingAllPointsActivated,unsigned int type = 0){
        // Construir el Octree Multi con base en el Octree existente
        Octree* octree = new Octree(minBound, maxBound, points,maxDepth, maxPointsPerNode, childContainingAllPointsActivated,type);
        root = new OctreeNodeMulti(octree->root->minBound, octree->root->maxBound, 0);
        this->nodesPerOctree[0] = octree->getFlattenedNodes().first.size();
        constructMultiOctree(root, octree->root);
        this->updateFlattenedPointsPerOctree();
        this->nOctrees++;
    }

    ~MultiOctree() {
        delete root;
    }

    void constructMultiOctree(OctreeNodeMulti* multiNode, OctreeNode* node) {
        // Aquí está cómo copiarías la información de los puntos de OctreeNode a OctreeNodeMulti para el Octree original
        OctreeData data;
        data.points = node->points;
        data.pointIndex = node->pointIndex;
        data.pointCount = node->pointCount;
        data.depends = true;
        data.typeNode = node->typeNode;
        data.leaf = node->leaf;
        if (data.leaf) {
            multiNode->isLeafForAnyOctree = true;
        }

        multiNode->octreeDataMap[0] = data; // Considerando que el ID del primer Octree es 0

        // Luego debes hacer lo mismo para cada uno de los hijos del nodo
        for (int i = 0; i < 8; i++) {
            if (node->children[i] != nullptr) {
                multiNode->children[i] = new OctreeNodeMulti(node->children[i]->minBound, node->children[i]->maxBound, node->depth + 1);
                constructMultiOctree(multiNode->children[i], node->children[i]);
            }
        }
    }

    std::vector<Point> getFlattenedPoints(int n)
    {
        try
        {
            return this->flattenedPointsPerOctree.at(n);
        }
        catch (const std::exception&)
        {
            return std::vector<Point>();
        }
        
    }

    void mergeOctree(Octree* newOctree) {

        this->mergeOctree(this->root, newOctree->root, this->nOctrees);
        this->nodesPerOctree[this->nOctrees] = newOctree->getFlattenedNodes().first.size();
        this->nOctrees++;
        this->updateFlattenedPointsPerOctree();
    }

    void mergeOctree(const vec3& minBound, const vec3& maxBound, const std::vector<Point>& points, unsigned int maxDepth , unsigned int maxPointsPerNode, bool childContainingAllPointsActivated, unsigned int type = 0) {

        Octree* newOctree = new Octree(minBound, maxBound,points, maxDepth, maxPointsPerNode, childContainingAllPointsActivated,type);
        this->nodesPerOctree[this->nOctrees] = newOctree->getFlattenedNodes().first.size();
        this->mergeOctree(this->root, newOctree->root, this->nOctrees);
        this->nOctrees++;
        this->updateFlattenedPointsPerOctree();
    }

    std::pair<std::vector<FlattenedMultiNodeFloats>, std::vector<FlattenedMultiNodeInts>> getFlattenedNodesMultiOctree(int n) {
        std::vector<FlattenedMultiNodeFloats> flattenedNodesFloats;
        std::vector<FlattenedMultiNodeInts> flattenedNodesInts;
        flattenMultiOctree(this->root, flattenedNodesFloats, flattenedNodesInts, n);
        return std::make_pair(flattenedNodesFloats, flattenedNodesInts);
    }

    bool checkNodeIntegrity() {
        return checkNodeIntegrityHelper(root);
    }

    bool checkNodeIntegrityHelper(OctreeNodeMulti* node) {
        if (!node) return true;

        // Comprueba que todos los nodos tienen información de todos los octrees
        if (node->octreeDataMap.size() != nOctrees) {
            return false;
        }

        for (auto it = node->octreeDataMap.begin(); it != node->octreeDataMap.end(); ++it) {
            const auto& key = it->first;
            const auto& val = it->second;

            if (val.leaf && !val.depends) {
				return false;
			}

            if (!val.depends) {
                for (OctreeNodeMulti* child : node->children) {
                    if (child && child->octreeDataMap[key].depends) {
                        return false;
                    }
                }
            }

            if (val.leaf) {
                for (OctreeNodeMulti* child : node->children) {

                    bool containedPoints = false;
                    for (Point p : val.points) {
                        if (child && inBounds(p.position,child->minBound, child->maxBound)) {
							containedPoints = true;
							break;
						}
					}   
                    if (containedPoints && !child->octreeDataMap[key].leaf) {
                        return false;
                    }
                }
            }

         }

        // Realiza comprobaciones de integridad en los nodos hijos
        for (OctreeNodeMulti* child : node->children) {
            if (!checkNodeIntegrityHelper(child)) {
                return false;
            }
        }

        return true;
    }

    int getOctreeNumberOctrees() {
		return this->nOctrees;
	}

    int getMaxDepth() {
		return this->maxDepth;
	}

    int getNodesPerOctree(int i) {
        return this->nodesPerOctree[i];
    }


private:
    OctreeNodeMulti* root;
    int nOctrees = 0;
    std::map<int, std::vector<Point>> flattenedPointsPerOctree;
    std::map<int, int> nodesPerOctree;

    int maxDepth;
    int maxPointsPerNode;

    OctreeData createOctreeData(OctreeNode* node) {
        OctreeData data;
        data.points = node->points;
        data.pointIndex = node->pointIndex;
        data.pointCount = node->pointCount;
        data.depends = true;
        data.leaf = node->leaf;
        data.typeNode = node->typeNode;
        if (this->maxDepth < node->depth) {
            this->maxDepth = node->depth;
        }
        return data;
    }

    void mergeOctree(OctreeNodeMulti* multiNode, OctreeNode* node, int octreeIndex) {

        if (node && multiNode) {

            OctreeData data = createOctreeData(node);
            multiNode->octreeDataMap[octreeIndex] = data;
            bool aumentedProfundity = false;


            //Si es un leaf y por lo tanto no tiene hijos, entonces.
            if (node->leaf) {

                // No va a tener mas hijos, por lo tanto, tenemos que tratar este nodo como un leaf y crear mas posibles subleaf segun los otros octrees.
                populateLeafNodeData(multiNode, node, octreeIndex);

            }
            else {

                for (int i = 0; i < 8; i++) {
                    
                    if (node->children[i]) {

                        if (multiNode->children[i]) {

                            // El hijo existe para el nodo y exista para el multiNodo, entonces seguimos avanzando.
                            mergeOctree(multiNode->children[i], node->children[i], octreeIndex);
                        }

                        else {

                            
                            aumentedProfundity = true;

                            createNewChildForMultiNode(multiNode, node, octreeIndex, i);

                            mergeOctree(multiNode->children[i], node->children[i], octreeIndex);

                        }
                    }
                    else {

                        //Si no existe hijo pero si existe hijo del multi, lo rellenamos de null.
                        if (multiNode->children[i]) {
                            populateNullNodeData(multiNode->children[i], octreeIndex);
                        }

                    }
                }

                if (aumentedProfundity)
                {
                    updateProfundityForNode(multiNode, octreeIndex);

                }
            }
        }
    }

    void populateNullNodeData(OctreeNodeMulti* multiNode, int octreeIndex) {
        for (int i = 0; i < 8; i++) {
            if (multiNode->children[i]) {
                populateNullNodeData(multiNode->children[i], octreeIndex);
            }
        }
        if (multiNode) {
            OctreeData data;
            data.depends = false;
            data.typeNode = 0;
            data.points = std::vector<Point>();
            data.pointIndex = -1;
            data.pointCount = 0;
            data.leaf = false;
            multiNode->octreeDataMap[octreeIndex] = data;
        }
    }

    void populateLeafNodeData(OctreeNodeMulti* multiNode, OctreeNode* node, int octreeIndex) {

        //primero comprobamos que ya no sea un nodo hoja el multiNode
        int contador = 0;
        for (int i = 0; i < 8; i++) {
            if (multiNode->children[i] == nullptr) {
                contador++;
            }
        }
        if (contador == 8) {
            return;
        }

        for (int i = 0; i < 8; i++) {

            //Primeramente, buscamos a que quadrante tendrian que ir estos nodos.
            vec3 childMinBound, childMaxBound;
            vec3 midPoint = (multiNode->minBound + multiNode->maxBound) * 0.5f;
            childMinBound = mix(multiNode->minBound, midPoint, vec3((i & 1) > 0, (i & 2) > 0, (i & 4) > 0));
            childMaxBound = mix(midPoint, multiNode->maxBound, vec3((i & 1) > 0, (i & 2) > 0, (i & 4) > 0));
            
            std::vector<Point> childPoints;
            for (const auto& point : node->points) {
                if (inBounds(point.position, childMinBound, childMaxBound)) {
                    childPoints.push_back(point);
                }
            }

            //Por otro lado, miramos si existe un hijo en el nodo multi.
            bool existSMultiNodeChild = multiNode->children[i] != nullptr;


            //Ahora hay el caso donde existe un hijo en el noto multi y hay puntos que se pueden asignar a ese hijo.
            if (childPoints.size() != 0 && existSMultiNodeChild) {
                OctreeData data;
                data.points = childPoints;
                data.depends = true;
                data.leaf = true;
                data.typeNode = node->typeNode;
                multiNode->children[i]->octreeDataMap[octreeIndex] = data;
                populateLeafNodeData(multiNode->children[i], node, octreeIndex);
            }

            //Caso 2: Existe un hijo en el nodo multi y NO hay puntos que se pueden asignar a ese hijo.
            if (childPoints.size() == 0 && existSMultiNodeChild) {
                populateNullNodeData(multiNode->children[i], octreeIndex);
            }

            //Caso 3: NO existe un hijo en el nodo multi y hay puntos que se pueden asignar a ese hijo.
            if (childPoints.size() != 0 && !existSMultiNodeChild) {
                multiNode->children[i] = new OctreeNodeMulti(childMinBound, childMaxBound, multiNode->depth + 1);

                // Rellenamos los datos de este nodo para los demás octrees con datos vacíos
                for (auto it = multiNode->octreeDataMap.begin(); it != multiNode->octreeDataMap.end(); ++it) {
                    const auto& key = it->first;
                    const auto& val = it->second;
                    if (it->first != octreeIndex) {
                        std::vector<Point> containedPoints;

                        OctreeData existingOctreeData;
                        existingOctreeData.points = containedPoints;
                        existingOctreeData.depends = false;
                        existingOctreeData.pointCount = -1;
                        existingOctreeData.pointIndex = 0;
                        existingOctreeData.typeNode = val.typeNode;
                        existingOctreeData.leaf = false;
                        multiNode->children[i]->octreeDataMap[key] = existingOctreeData;
                    }
                }
                OctreeData data;
                data.points = childPoints;
                data.depends = true;
                data.leaf = true;
                data.typeNode = node->typeNode;
                multiNode->children[i]->octreeDataMap[octreeIndex] = data;
            }

            //Caso 4: NO existe un hijo en el nodo multi y NO hay puntos que se pueden asignar a ese hijo.
            if (childPoints.size() == 0 && !existSMultiNodeChild) {
                //NADA
            }
        }
    }

    void createNewChildForMultiNode(OctreeNodeMulti* multiNode, OctreeNode* node, int octreeIndex, int i)
    {
        // El hijo no existe para el multiNodo, creamos uno nuevo con los bounds de este hijo.
        multiNode->children[i] = new OctreeNodeMulti(node->children[i]->minBound, node->children[i]->maxBound, multiNode->depth + 1);
        // Rellenar la información de los otros octrees en el multiNode nuevo.
        for (auto it = multiNode->octreeDataMap.begin(); it != multiNode->octreeDataMap.end(); ++it) {
            const auto& key = it->first;
            const auto& val = it->second;

            if (key != octreeIndex) {
                std::vector<Point> containedPoints;
                for (const auto& point : val.points) {
                    if (inBounds(point.position, node->children[i]->minBound, node->children[i]->maxBound)) {
                        containedPoints.push_back(point);
                    }
                }

                if (!containedPoints.empty()) {
                    OctreeData existingOctreeData;
                    existingOctreeData.points = containedPoints;
                    existingOctreeData.depends = true;
                    existingOctreeData.pointCount = val.pointCount;
                    existingOctreeData.pointIndex = val.pointIndex;
                    existingOctreeData.typeNode = val.typeNode;
                    existingOctreeData.leaf = true;
                    multiNode->children[i]->octreeDataMap[key] = existingOctreeData;
                }
                else {
                    OctreeData existingOctreeData;
                    existingOctreeData.points = containedPoints;
                    existingOctreeData.depends = false;
                    existingOctreeData.pointCount = -1;
                    existingOctreeData.pointIndex = 0;
                    existingOctreeData.typeNode = val.typeNode;
                    existingOctreeData.leaf = false;
                    multiNode->children[i]->octreeDataMap[key] = existingOctreeData;
                }
            }
        }
        OctreeData data;
        data.points = node->children[i]->points;
        data.pointIndex = node->children[i]->pointIndex;
        data.pointCount = node->children[i]->pointCount;
        data.depends = true;
        data.leaf = node->children[i]->leaf;
        data.typeNode = node->children[i]->typeNode;
        multiNode->children[i]->octreeDataMap[octreeIndex] = data;
    }

    void updateProfundityForNode(OctreeNodeMulti* multiNode, int octreeIndex) {
        for (int i = 0; i < 8; i++) {

            //Siempre sera en un possible hijo que no haya sido creado
            if (multiNode->children[i] == nullptr) {

                //Calculamos los bounds del possible hijo.
                vec3 childMinBound, childMaxBound;
                vec3 midPoint = (multiNode->minBound + multiNode->maxBound) * 0.5f;
                childMinBound = mix(multiNode->minBound, midPoint, vec3((i & 1) > 0, (i & 2) > 0, (i & 4) > 0));
                childMaxBound = mix(midPoint, multiNode->maxBound, vec3((i & 1) > 0, (i & 2) > 0, (i & 4) > 0));

                bool created = false;
                //Para cada octree, miramos si existe un nodo que contenga el possible hijo.
                for (auto it = multiNode->octreeDataMap.begin(); it != multiNode->octreeDataMap.end(); ++it) {
                    const auto& key = it->first;
                    const auto& val = it->second;
                    //Solo para todos los octrees menos el que estamos tratando.
                    if (key != octreeIndex) {
                        std::vector<Point> containedPoints;
                        for (const auto& point : val.points) {
                            if (inBounds(point.position, childMinBound, childMaxBound)) {
                                containedPoints.push_back(point);
                            }
                        }

                        //En caso de que existan puntos, creamos el nodo y lo rellenamos.
                        if (!containedPoints.empty()) {
                            multiNode->children[i] = new OctreeNodeMulti(childMinBound, childMaxBound, multiNode->depth + 1);
                            created = true;
                            OctreeData existingOctreeData;
                            existingOctreeData.points = containedPoints;
                            existingOctreeData.depends = true;
                            existingOctreeData.pointCount = val.pointCount;
                            existingOctreeData.pointIndex = val.pointIndex;
                            existingOctreeData.typeNode = val.typeNode;
                            existingOctreeData.leaf = true;
                            multiNode->children[i]->octreeDataMap[key] = existingOctreeData;


                        }

                    }
                }

                if (created) {
                    //Lo rellenamos de null con todos los demas octrees que no hayan entrado dentro.
                    for (auto it = multiNode->octreeDataMap.begin(); it != multiNode->octreeDataMap.end(); ++it) {
                        const auto& key = it->first;
                        const auto& val = it->second;

                        auto it2 = multiNode->children[i]->octreeDataMap.find(key);
                        if (it2 != multiNode->children[i]->octreeDataMap.end()) {
                            continue;
                        }
                        else {
                            populateNullNodeData(multiNode->children[i], key);
                        }

                    }

                }
            }




        }
    }

    
    bool hasLeafChildren(OctreeNodeMulti* node, int octreeIndex) {
        for (int i = 0; i < 8; i++) {
            if (node->children[i] && node->children[i]->octreeDataMap.count(octreeIndex) && node->children[i]->octreeDataMap[octreeIndex].leaf) {
                return true;
            }
        }
        return false;
    }

    bool inBounds(const vec3& point, const vec3& minBound, const vec3& maxBound, const float epsilon = 1e-6f) {
        return point.x >= minBound.x - epsilon && point.x <= maxBound.x + epsilon &&
            point.y >= minBound.y - epsilon && point.y <= maxBound.y + epsilon &&
            point.z >= minBound.z - epsilon && point.z <= maxBound.z + epsilon;
    }

    //Used to flatten the points with DFS.
    void updateFlattenedPointsPerOctreeDFS(OctreeNodeMulti* node, int octreeIndex, unsigned int& currentPointIndex, std::vector<Point>& flattenedPoints) {
        if (node == nullptr) return;

        // Comprobamos si el nodo actual tiene datos para el octreeIndex
        if (node->octreeDataMap.count(octreeIndex)) {
            OctreeData& nodeData = node->octreeDataMap[octreeIndex];
            if (node->depth == 4) {
                int a = 0;
            }
            if (nodeData.points.size() == 0) return;
            // Si el nodo es una hoja y no tiene hijos con nodos hojas para el mismo octreeIndex, agregamos sus puntos
            if (nodeData.leaf && !hasLeafChildren(node, octreeIndex)) {
                flattenedPoints.insert(flattenedPoints.end(), nodeData.points.begin(), nodeData.points.end());
                nodeData.pointIndex = currentPointIndex;
                nodeData.pointCount = nodeData.points.size();
                currentPointIndex += nodeData.points.size();
            }
            // Si el nodo no es una hoja o tiene hijos con nodos hojas para el mismo octreeIndex, recorremos sus hijos
            else {
                for (int i = 0; i < 8; i++) {
                    if (node->children[i]) {
                        updateFlattenedPointsPerOctreeDFS(node->children[i], octreeIndex, currentPointIndex, flattenedPoints);
                    }
                }

                // Actualizamos la información del nodo basándonos en los hijos
                int sum = 0;
                for (int i = 0; i < 8; i++) {
                    if (node->children[i] && node->children[i]->octreeDataMap.count(octreeIndex)) {
                        sum += node->children[i]->octreeDataMap[octreeIndex].pointCount;
                    }
                }
                nodeData.pointCount = nodeData.points.size();
                nodeData.pointIndex = flattenedPoints.size() - nodeData.pointCount;
            }
        }
        // Si el nodo actual no contiene datos para el octreeIndex, seguimos buscando en sus hijos
        else {
            for (int i = 0; i < 8; i++) {
                updateFlattenedPointsPerOctreeDFS(node->children[i], octreeIndex, currentPointIndex, flattenedPoints);
            }
        }
    }

    //Used to flatten the points.
    void updateFlattenedPointsPerOctree() {
        flattenedPointsPerOctree.clear();

        for (const auto& entry : root->octreeDataMap) {
            int octreeIndex = entry.first;
            unsigned int currentPointIndex = 0;
            std::vector<Point> flattenedPoints;

            updateFlattenedPointsPerOctreeDFS(root, octreeIndex, currentPointIndex, flattenedPoints);

            flattenedPointsPerOctree[octreeIndex] = flattenedPoints;
        }
    }

    //Used to flatten the nodes.
    int flattenMultiOctree(const OctreeNodeMulti* node, std::vector<FlattenedMultiNodeFloats>& flattenedNodesFloats, std::vector<FlattenedMultiNodeInts>& flattenedNodesInts, int n) {
        if (!node) {
            return -1;
        }

        FlattenedMultiNodeFloats flatNodeFloat;
        FlattenedMultiNodeInts flatNodeInt;
        flatNodeFloat.minBound = node->minBound;
        flatNodeFloat.maxBound = node->maxBound;

        for (int octreeIndex = 0; octreeIndex < n; ++octreeIndex) {
            if (node->octreeDataMap.count(octreeIndex)) {
                flatNodeInt.octreeInfo[octreeIndex].pointIndex = node->octreeDataMap.at(octreeIndex).pointIndex;
                flatNodeInt.octreeInfo[octreeIndex].pointCount = node->octreeDataMap.at(octreeIndex).pointCount;
                flatNodeInt.octreeInfo[octreeIndex].depends = node->octreeDataMap.at(octreeIndex).depends;
                flatNodeInt.octreeInfo[octreeIndex].leaf = node->octreeDataMap.at(octreeIndex).leaf;
            }
            else {
                // llenar con datos por defecto si el índice de octree no está presente
                flatNodeInt.octreeInfo[octreeIndex].pointIndex = -1;
                flatNodeInt.octreeInfo[octreeIndex].pointCount = 0;
                flatNodeInt.octreeInfo[octreeIndex].depends = false;
                flatNodeInt.octreeInfo[octreeIndex].leaf = false;
            }
        }

        int currentNodeIndex = flattenedNodesInts.size();
        flattenedNodesInts.push_back(flatNodeInt);
        flattenedNodesFloats.push_back(flatNodeFloat);

        for (int i = 0; i < 8; ++i) {
            if (node->children[i]) {
                flatNodeInt.childrenIndices[i] = flattenMultiOctree(node->children[i], flattenedNodesFloats, flattenedNodesInts, n);
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

};