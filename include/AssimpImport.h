#pragma once
#include "Texture.h"
#include "SceneObject.h"
#include <assimp/scene.h>
#include <unordered_map>
#include <filesystem>
#include <string>

SceneObject assimpLoad(const std::string& path, bool flipUVCoords);
SceneObject processAssimpNode(
	const aiNode* node, 
	const aiScene* scene,
	const std::filesystem::path& modelPath,
	std::unordered_map<std::string, Texture>& loadedTextures);