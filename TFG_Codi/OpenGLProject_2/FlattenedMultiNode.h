#pragma once
#include<glm.hpp>

struct FlattenedMultiNodeFloats {
    glm::vec3 minBound;
    glm::vec3 maxBound;
};

struct FlattenedMultiNodeOctreeInfo {
    int pointIndex;
    int pointCount;
    bool depends;
    bool leaf;
};

struct FlattenedMultiNodeInts {
    FlattenedMultiNodeOctreeInfo octreeInfo[3];
    int childrenIndices[8];
};

struct FlattenedMultiNodeIntsSend1 {
    glm::ivec4 pointInfo;
    glm::ivec4 childrenInfo1;
    glm::ivec4 childrenInfo2;
};

struct FlattenedMultiNodeIntsSend2 {
    glm::ivec4 pointInfo;
    glm::ivec4 pointInfo1;
    glm::ivec4 childrenInfo1;
    glm::ivec4 childrenInfo2;
};

struct FlattenedMultiNodeIntsSend3 {
    glm::ivec4 pointInfo1;
    glm::ivec4 pointInfo2;
    glm::ivec4 pointInfo3;
    glm::ivec4 childrenInfo1;
    glm::ivec4 childrenInfo2;
};