#pragma once

#define UNITY_NATIVE_EXPORT extern "C" __declspec(dllexport)

struct ParsedMeshStructure
{
	float * vertexData;
	float * normalData;
	float * colorData;

	int numTris;
};
