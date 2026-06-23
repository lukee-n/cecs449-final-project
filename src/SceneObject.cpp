#include "SceneObject.h"
#include "ShaderProgram.h"
#include <glm/ext.hpp>

glm::mat4 SceneObject::buildModelMatrix() const {
	auto m{ glm::translate(glm::mat4{ 1 }, position) };
	m = glm::translate(m, center * scale);
	m = glm::rotate(m, orientation[2], glm::vec3{ 0, 0, 1 });
	m = glm::rotate(m, orientation[0], glm::vec3{ 1, 0, 0 });
	m = glm::rotate(m, orientation[1], glm::vec3{ 0, 1, 0 });
	m = glm::scale(m, scale);
	m = glm::translate(m, -center);
	m = m * baseTransform;
	return m;
}

void SceneObject::drawObject(ShaderProgram& program) const {
	drawObjectRecursive(glm::mat4{ 1 }, program);
}

void SceneObject::drawObjectRecursive(const glm::mat4& parentModel, ShaderProgram& program) const {
	glm::mat4 localModel{ buildModelMatrix() };

	// TODO: to render a hierarchical mesh, we must build the "true" model matrix for htis object,
	// which is the combination of its parent's model matrix and the local model matrix.
	// At the moment, we assume that localModel == trueModel, which is only true for the root 
	// of a hierarchy. So this code only draws a mesh correctly if it has no children at all.

	glm::mat4 trueModel{ parentModel * localModel };
	// TODO: you must change the calculation above, so that parentModel is combined with localModel to 
	// make trueModel. We want localModel's transformations to happen BEFORE parentModel's.
	program.setUniform("model", trueModel);


	// Render each *mesh* in the object.
	for (auto& mesh : meshes) {  // const?
		mesh.drawMesh(program);
	}

	// TODO: to render the rest of the hierarchy, you must loop through each element of "children",
	// and have them render themselves recursively. The parent model matrix for your children is your own
	// true model matrix.
	for (const auto& child : children) {
		child.drawObjectRecursive(trueModel, program);
	}
}