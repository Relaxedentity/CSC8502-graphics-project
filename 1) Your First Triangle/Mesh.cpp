#include "Mesh.h"

Mesh* Mesh::GenerateTriangle() {
	Mesh* m = new Mesh();
	m->numVertices = 3;
	m->vertices = new Vector3[m->numVertices];