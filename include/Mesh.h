#pragma once
#include <vector>
#include "Texture.h"
#include "ShaderProgram.h"

struct Vertex3D {
	float x;
	float y;
	float z;

	float u;
	float v;

	float nx;
	float ny;
	float nz;
};

struct Mesh {
	uint32_t vao;
	uint32_t faceCount;
	std::vector<Texture> textures;

	Mesh(const std::vector<Vertex3D>& vertices, const std::vector<uint32_t>& faces, std::vector<Texture> textures);
	void drawMesh(ShaderProgram& program) const;

	static Mesh square(std::vector<Texture> textures);
	static Mesh squareTiled(std::vector<Texture> textures, float tiles);
};

