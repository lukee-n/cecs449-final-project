#include "AssimpImport.h"
#include <iostream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <filesystem>
#include <unordered_map>

std::filesystem::path projectRoot() {
	 return std::filesystem::path(__FILE__).parent_path().parent_path();
}

std::vector<Texture> loadMaterialTextures(
	aiMaterial* mat,
	aiTextureType type,
	const std::string& typeName,
	const std::filesystem::path& modelPath,
	std::unordered_map<std::string, Texture>& loadedTextures
) {
	std::vector<Texture> textures{};
	for (uint32_t i{ 0 }; i < mat->GetTextureCount(type); ++i) {
		aiString name{};
		mat->GetTexture(type, i, &name);

		// Texture path
		std::string textureName = name.C_Str();
		// Decode spaces from glTF URI paths.
		size_t pos = 0;
		while ((pos = textureName.find("%20", pos)) != std::string::npos) {
			textureName.replace(pos, 3, " ");
			pos += 1;
		}
		std::filesystem::path texPath{ modelPath.parent_path() / textureName };

		std::cout << "loading " << texPath << " as " << typeName << std::endl;

		std::string cacheKey = typeName + "|" + texPath.string();
		auto existing{ loadedTextures.find(cacheKey) };
		if (existing != loadedTextures.end()) {
			textures.push_back(existing->second);
		}
		else {
			StbImage image{};
			image.loadFromFile(texPath.string());
			Texture tex{ Texture::loadImage(image, typeName) };
			textures.push_back(tex);
			loadedTextures.insert(std::make_pair(cacheKey, tex));

			// Automatically load matching spec map for base textures (tex_name.png -> tex_name_spec.png)
			if (typeName == "baseTexture") {
				std::filesystem::path specPath = projectRoot() / texPath;
				specPath.replace_filename(
					texPath.stem().string() + "_spec" + texPath.extension().string()
				);

				if (std::filesystem::exists(specPath)) {
					std::cout << "Loaded spec map: " << specPath << '\n';

					std::string specCacheKey = "specMap|" + specPath.string();

					auto existingSpec{ loadedTextures.find(specCacheKey) };
					if (existingSpec != loadedTextures.end()) {
						textures.push_back(existingSpec->second);
					}
					else {
						StbImage specImage{};
						specImage.loadFromFile(specPath.string());

						Texture specTex{ Texture::loadImage(specImage, "specMap") };

						textures.push_back(specTex);
						loadedTextures.insert(std::make_pair(specCacheKey, specTex));
					}
				}
			}
		}
	}
	return textures;
}

Mesh fromAssimpMesh(const aiMesh* mesh, const aiScene* scene, const std::filesystem::path& modelPath,
	std::unordered_map<std::string, Texture>& loadedTextures) {
	std::vector<Vertex3D> vertices;

	for (size_t i{ 0 }; i < mesh->mNumVertices; i++) {
		aiVector3D texCoord{ 0.0f, 0.0f, 0.0f };

		auto& meshVertex{ mesh->mVertices[i] }; 
		if (mesh->mTextureCoords[0]) {  // safety
			texCoord = mesh->mTextureCoords[0][i];
		}
		
		aiVector3D normal{ 0.0f, 1.0f, 0.0f };
		if (mesh->HasNormals()) {
			normal = mesh->mNormals[i];
		}

		// TODO: construct a Vertex3D object using this assimp node's data, referenced
		// with the variables above. A Vertex3D is constructed with a position x/y/z,  
		// texture u/v (x/y), and normal x/y/z, in that order.
		// Then push_back the Vertex3D into the vertice list.
		vertices.push_back(
			Vertex3D{
				meshVertex.x,
				meshVertex.y,
				meshVertex.z,

				texCoord.x,
				texCoord.y,

				normal.x,
				normal.y,
				normal.z
			}
		);
	}

	std::vector<uint32_t> faces;
	for (size_t i{ 0 }; i < mesh->mNumFaces; ++i) {
		auto& meshFace{ mesh->mFaces[i] };

		// Skip non-triangle (fixes boulder)
		if (meshFace.mNumIndices != 3)
			continue;

		faces.push_back(meshFace.mIndices[0]);
		faces.push_back(meshFace.mIndices[1]);
		faces.push_back(meshFace.mIndices[2]);
	}

	// Load any base textures, specular maps, and normal maps associated with the mesh.
	std::vector<Texture> textures{};
	if (mesh->mMaterialIndex >= 0) {
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		std::vector<Texture> diffuseMaps{
			loadMaterialTextures(material, aiTextureType_DIFFUSE, "baseTexture", modelPath, loadedTextures)
		};
		textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

		std::vector<Texture> specularMaps{
			loadMaterialTextures(material, aiTextureType_SPECULAR, "specMap", modelPath, loadedTextures)
		};
		textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

		std::vector<Texture> normalMaps{
			loadMaterialTextures(material, aiTextureType_HEIGHT, "normalMap", modelPath, loadedTextures)
		};
		textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

		normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, "normalMap", modelPath, loadedTextures);
		textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

		std::vector<Texture> emissiveMaps{
			loadMaterialTextures(material, aiTextureType_EMISSIVE, "emissiveTexture", modelPath, loadedTextures)
		};
		textures.insert(textures.end(), emissiveMaps.begin(), emissiveMaps.end());
	}

	return Mesh{ vertices, faces, std::move(textures) };
}

SceneObject assimpLoad(const std::string& path, bool flipTextureCoords) {
	Assimp::Importer importer{};

	auto options{ aiProcessPreset_TargetRealtime_MaxQuality };
	options |= aiProcess_Triangulate;
	if (flipTextureCoords) {
		options |= aiProcess_FlipUVs;
	}
	const aiScene* scene{ importer.ReadFile(path, options) };

	// If the import failed, report it
	if (nullptr == scene) {
		std::string error{ importer.GetErrorString() };
		std::cerr << "Error loading assimp file: " + error << std::endl;
		throw std::runtime_error("Error loading assimp file: " + error);
	}
	std::vector<Mesh> meshes{};
	std::unordered_map<std::string, Texture> loadedTextures{};
	return processAssimpNode(scene->mRootNode, scene, std::filesystem::path{ path }, loadedTextures);
}

// A "Node" in assimp is an Object3D in our framework. It has one or more meshes,
// plus zero or more children.
SceneObject processAssimpNode(
	const aiNode* node, 
	const aiScene* scene,
	const std::filesystem::path& modelPath,
	std::unordered_map<std::string, Texture>& loadedTextures
) {
	// Load the aiNode's meshes.
	std::vector<Mesh> meshes{};
	for (size_t i{ 0 }; i < node->mNumMeshes; ++i) {
		aiMesh* mesh{ scene->mMeshes[node->mMeshes[i]] };
		meshes.emplace_back(fromAssimpMesh(mesh, scene, modelPath, loadedTextures));
	}

	// Load the node's textures.
	std::vector<Texture> textures{};
	for (auto& p : loadedTextures) {
		textures.push_back(p.second);
	}

	// Initialize the base transform of the object. (Needs to be transposed from assimp.)
	glm::mat4 baseTransform{};
	for (uint32_t i{ 0 }; i < 4; ++i) {
		for (uint32_t j{ 0 }; j < 4; ++j) {
			baseTransform[i][j] = node->mTransformation[j][i];
		}
	}

	// Initialize the object.
	SceneObject parent{};
	parent.meshes = std::move(meshes);
	parent.baseTransform = baseTransform;

	// Recursively process the children of the node and add them as child objects.
	for (size_t i{ 0 }; i < node->mNumChildren; ++i) {
		SceneObject child{ processAssimpNode(node->mChildren[i], scene, modelPath, loadedTextures) };
		parent.children.push_back(std::move(child));
	}

	return parent;
}
