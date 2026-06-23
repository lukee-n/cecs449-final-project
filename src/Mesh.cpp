#include <glad/glad.h>
#include "Mesh.h"
#include <cstdint>

Mesh Mesh::square(std::vector<Texture> textures) {
	Mesh m{
		{
			{ 0.5, 0.5, 0, 1, 0, 0, 0, 1 },    // TR
			{ 0.5, -0.5, 0, 1, 1, 0, 0, 1 },   // BR
			{ -0.5, -0.5, 0, 0, 1, 0, 0, 1 },  // BL
			{ -0.5, 0.5, 0, 0, 0, 0, 0, 1 },   // TL
		},
		{
			2, 1, 3,
			3, 1, 0,
		},
		std::move(textures)
	};
	return m;
}

// Fix stretch by tiling
Mesh Mesh::squareTiled(std::vector<Texture> textures, float tiles) {
	Mesh m{
		{
			{  0.5,  0.5, 0, tiles, 0,     0, 0, 1 },  // TR
			{  0.5, -0.5, 0, tiles, tiles, 0, 0, 1 },  // BR
			{ -0.5, -0.5, 0, 0,     tiles, 0, 0, 1 },  // BL
			{ -0.5,  0.5, 0, 0,     0,     0, 0, 1 },  // TL
		},
		{
			2, 1, 3,
			3, 1, 0,
		},
		std::move(textures)
	};
	return m;
}

Mesh::Mesh(const std::vector<Vertex3D> &vertices, const std::vector<uint32_t> &faces, 
	std::vector<Texture> meshTextures)
	: faceCount{ static_cast<uint32_t>(faces.size()) }, textures{std::move(meshTextures)}
{
	// Generate a vertex array object on the GPU.
	glGenVertexArrays(1, &vao);
	// "Bind" the newly-generated vao, which makes future functions operate on that specific object.
	glBindVertexArray(vao);

	// Generate a vertex buffer object on the GPU.
	uint32_t vbo;
	glGenBuffers(1, &vbo);

	// "Bind" the newly-generated vbo, which makes future functions operate on that specific object.
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	// This vbo is now associated with m_vao.
	// Copy the contents of the vertices list to the buffer that lives on the GPU.
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex3D), &vertices[0], GL_STATIC_DRAW);


	// TODO: use glVertexAttribPointer and glEnableVertexAttribArray to inform OpenGL about our
	// vertex attributes. Attribute 0 is position (3 floats), 1 is texture coords (2 floats), and 
	// 2 is normal (3 floats), 
	// 
	// Textures and Lighting, Slide 18
	// [x y z][u v][nx ny nz]
	// 
	// Position
	glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex3D), 0);
	glEnableVertexAttribArray(0);
	
	// Texture coords
	glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(Vertex3D), (void*)offsetof(Vertex3D, u));  // offset of 3 floats (12)
	glEnableVertexAttribArray(1);

	// Normals
	glVertexAttribPointer(2, 3, GL_FLOAT, false, sizeof(Vertex3D), (void*)offsetof(Vertex3D, nx)); // offset of 5 floats (20)
	glEnableVertexAttribArray(2);


	// Generate a second buffer, to store the indices of each triangle in the mesh.
	uint32_t ebo;
	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, faces.size() * sizeof(uint32_t), faces.data(), GL_STATIC_DRAW);  //glBufferData(GL_ELEMENT_ARRAY_BUFFER, faces.size() * sizeof(uint32_t), &faces[0], GL_STATIC_DRAW);

	// Unbind the vertex array, so no one else can accidentally mess with it.
	glBindVertexArray(0);
}

void Mesh::drawMesh(ShaderProgram& program) const {
	bool hasSpecMap = false;
	bool hasEmission = false;

	for (uint32_t i{ 0 }; i < textures.size(); ++i) {
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, textures[i].textureId);
		program.setUniform(textures[i].samplerName, static_cast<int>(i));

		if (textures[i].samplerName == "specMap") {          // check if mesh has specular map
			hasSpecMap = true;
		}
		if (textures[i].samplerName == "emissiveTexture") {  // check if the mesh has emissive texture
			hasEmission = true;
		}
	}
	program.setUniform("hasSpecMap", hasSpecMap ? 1 : 0);
	program.setUniform("hasEmission", hasEmission ? 1 : 0);
	
	glBindVertexArray(vao);
	// Draw the vertex array, using is "element buffer" to identify the faces, and whatever ShaderProgram
	// has been activated prior to this.
	glDrawElements(GL_TRIANGLES, faceCount, GL_UNSIGNED_INT, nullptr);
	// Deactivate the mesh's vertex array.
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

