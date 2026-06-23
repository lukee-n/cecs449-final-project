#pragma once
#include <glm/ext.hpp>
#include <vector>
#include <string>
#include "Mesh.h"

/**
 * @brief An object placed in a scene to be rendered. 
 * To be honest, this class is poorly designed. Most of the public fields should
 * be private, and access to them should only be through class methods. But we are
 * building graphics demos, not large-scale projects, so we'll opt to make everything
 * public which reduces the amount of code we have to write.
 */
class SceneObject {
public:
	// An object has 0 or more meshes associated with its world-space location...
	std::vector<Mesh> meshes{};

	// ... and it also might have many child objects, that have their own model matrix that is 
	// relative to their parent.
	std::vector<SceneObject> children{};

	// The object's position, orientation, and scale in world space.
	glm::vec3 position{0, 0, 0};
	glm::vec3 orientation{0, 0, 0};
	glm::vec3 scale{1.0, 1.0, 1.0};
	// The object's center of rotation, if we want to rotate around some other point
	// than the local space origin.
	glm::vec3 center{0, 0, 0};

	// The object's material parameters (ambient, diffuse, specular, shininess).
	glm::vec4 material{0.1, 1.0, 0.3, 4}; // default to a rough-ish material.

	// The object's base transformation matrix, which is used by some model formats to set a "starting" 
	// transformation for this object relative to its parent.
	glm::mat4 baseTransform{1};

	// Some objects from Assimp imports have a "name" field, useful for debugging.
	std::string name{};

	// Construct a 4x4 model matrix from the object's position, orientation, scale, center, and base transform.
	glm::mat4 buildModelMatrix() const;
	// Trigger an OpenGL rendering of the object, including its mesh and all its child objects.
	void drawObject(ShaderProgram& program) const;

private:
	void drawObjectRecursive(const glm::mat4& parentModel, ShaderProgram& program) const;
};

