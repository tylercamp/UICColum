
#include "Unity.h"

UNITY_NATIVE_EXPORT void FreeMesh( ParsedMeshStructure * mesh )
{
	if( mesh->colorData ) delete[] mesh->colorData;
	if( mesh->normalData ) delete[] mesh->normalData;
	if( mesh->vertexData ) delete[] mesh->vertexData;

	delete mesh;
}
