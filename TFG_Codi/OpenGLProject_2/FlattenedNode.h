#pragma once
#include<glm.hpp>


struct FlattenedNodeFloats {
    glm::vec3 minBound;
    glm::vec3 maxBound;
};

struct FlattenedNodeInts {
    int pointIndex;
    int pointCount;
    int depth;
    int extra2;
    int childrenIndices[8];
};

struct FlattenedNodeIntsSend {
    glm::ivec4 pointInfo;
    glm::ivec4 childrenInfo1;
    glm::ivec4 childrenInfo2;
};