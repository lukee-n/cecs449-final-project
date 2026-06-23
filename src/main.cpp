/**
* Starter code for CECS 449 final project.
* Major modules:
*	Mesh: an OpenGL VAO associated with one or more textures. Transfers vertices from RAM to VRAM. (INCOMPLETE)
*   Object3D: pairs a Mesh with world-space parameters (position, orientation, scale), and methods to manipulate
*			  the object in world space.
*			  Object3D is hierarchical; a "hierarchical mesh" is actually a tree of Object3Ds, which render
*			  recursively. (INCOMPLETE)
*   AssimpImport: loads an assimp model file into an hierarchical Object3D. (INCOMPLETE)
*   Animator: a list of animations to apply over given time intervals.
*   Main: initializes a Scene, advances Animators, and renders objects in the scene.
*/
#include <glad/glad.h>
#include <iostream>
#include <filesystem>
#include <numbers>

#include <SFML/Window/Event.hpp>
#include <SFML/Window/Window.hpp>
#include <SFML/Graphics.hpp>

#include "AssimpImport.h"
#include "Mesh.h"
#include "SceneObject.h"
#include "ShaderProgram.h"

#define M_PI std::numbers::pi_v<float>

// We use a structure to track all the elements of a scene, including a list of objects,
// a list of animators, and a shader program to use to render those objects.
struct Scene {
	ShaderProgram program{};
	std::vector<SceneObject> objects{};
};

/**
 * @brief Constructs a shader program that applies the Phong reflection model.
 */
ShaderProgram phongLightingShader() {
	ShaderProgram shader{};
	try {
		// These shaders are INCOMPLETE.
		shader.load("shaders/light_perspective.vert", "shaders/lighting.frag");
	}
	catch (std::runtime_error& e) {
		std::cout << "ERROR: " << e.what() << std::endl;
		exit(1);
	}
	return shader;
}

/**
 * @brief Constructs a shader program that performs texture mapping with no lighting.
 */
ShaderProgram texturingShader() {
	ShaderProgram shader{};
	try {
		shader.load("shaders/texture_perspective.vert", "shaders/texturing.frag");
	}
	catch (std::runtime_error& e) {
		std::cout << "ERROR: " << e.what() << std::endl;
		exit(1);
	}
	return shader;
}

/**
 * @brief Loads an image from the given path into an OpenGL texture.
 */
Texture loadTexture(const std::filesystem::path& path, const std::string& samplerName = "baseTexture") {
	StbImage i{};
	i.loadFromFile(path.string());
	return Texture::loadImage(i, samplerName);
}

/*****************************************************************************************
*  DEMONSTRATION SCENES
*****************************************************************************************/
Scene bunny() {
	Scene scene{ texturingShader() };

	// We assume that (0,0) in texture space is the upper left corner, but some artists use (0,0) in the lower
	// left corner. In that case, we have to flip the V-coordinate of each UV texture location. The last parameter
	// to assimpLoad controls this. If you load a model and it looks very strange, try changing the last parameter.
	auto bunny{ assimpLoad("models/bunny_textured.obj", true) };
	bunny.scale = glm::vec3{ 9, 9, 9 };
	bunny.position = glm::vec3{ 0.2, -1, 0 };

	// Move all objects into the scene's objects list.
	scene.objects.push_back(std::move(bunny));
	return scene;
}

/**
 * @brief Demonstrates loading a square, oriented as the "floor", with a manually-specified texture
 * that does not come from Assimp.
 */
Scene marbleSquare() {
	Scene scene{ texturingShader() };

	std::vector<Texture> textures{
		loadTexture("models/White_marble_03/Textures_2K/white_marble_03_2k_baseColor.tga", "baseTexture"),
	};
	auto mesh{ Mesh::square(textures) };
	SceneObject floor;
	floor.meshes.push_back(mesh);
	floor.scale = glm::vec3{ 5, 5, 5 };
	floor.position = glm::vec3{ 0, -1.5, 0 };
	floor.orientation = glm::vec3{ -M_PI / 2, 0, 0 };

	scene.objects.push_back(std::move(floor));
	return scene;
}

/**
 * @brief Loads a cube with a cube map texture.
 */
Scene cube() {
	Scene scene{ texturingShader() };
	auto cube{ assimpLoad("models/cube.obj", true) };
	scene.objects.push_back(std::move(cube));
	return scene;
}

/**
 * @brief Constructs a scene of a tiger sitting in a boat, where the tiger is the child object
 * of the boat.
 * @return
 */
Scene lifeOfPi() {
	// This scene is more complicated; it has child objects, as well as animators.
	Scene scene{ texturingShader() };

	auto boat{ assimpLoad("models/boat/boat.fbx", true) };
	boat.position = glm::vec3{ 0, -0.7, 0 };
	boat.scale = glm::vec3{ 0.01, 0.01, 0.01 };
	
	auto tiger{ assimpLoad("models/tiger/scene.gltf", true) };
	boat.children.push_back(std::move(tiger));

	// Move the boat into the scene list.
	scene.objects.push_back(std::move(boat));
	//scene.objects.push_back(std::move(tiger));
	return scene;
}


/*****************************************************************************************
*  GARDEN SCENE HELPERS
* 
*  These functions return an empty SceneObject where the children are the actual models
*  This is purely for organization and to clean up garden() function
*****************************************************************************************/

constexpr int MAX_POINT_LIGHTS = 8;

// Store ground object to be copied by scene groups
// so that children are placed relative to the ground
const float groundY = -1.5f;
const glm::vec3 groundPos{ 0, groundY, 0 };
SceneObject groupTemplate;

SceneObject campsite() {
	SceneObject camp{ groupTemplate };

	// Bonfire
	SceneObject bonfire;
	bonfire.scale = glm::vec3{ 0.2f };

	auto base{ assimpLoad("models/garden/Campsite/bonfire/scene.gltf", true) };
	base.position.y = 0.03;

	auto flame{ assimpLoad("models/garden/Campsite/flame/scene.gltf", true) };

	bonfire.children.push_back(std::move(base));
	bonfire.children.push_back(std::move(flame));

	// Logs
	auto log1{ assimpLoad("models/garden/Campsite/log/scene.gltf", true) };
	auto log2{ log1 };
	auto log3{ log1 };

	log1.scale = glm::vec3{ 200 };
	log2.scale = glm::vec3{ 170 };
	log3.scale = glm::vec3{ 190 };

	log1.position = glm::vec3{  0.0f, 0.007f,  1.0f };
	log2.position = glm::vec3{  1.0f, 0.007f, -0.6f };
	log3.position = glm::vec3{ -1.0f, 0.007f, -0.6f };

	log1.orientation.y = M_PI / 2;
	log2.orientation.y = M_PI / 4;
	log3.orientation.y = -M_PI / 4;

	// Add to group
	camp.children.push_back(std::move(bonfire));
	camp.children.push_back(std::move(log1));
	camp.children.push_back(std::move(log2));
	camp.children.push_back(std::move(log3));

	return camp;
}

SceneObject terrain() {
	SceneObject terrain{ groupTemplate };

	// Mountains
	auto mountain{ assimpLoad("models/garden/Terrain/mountain/scene.gltf", true) };
	mountain.position = glm::vec3{ 4.5f, 3.0f , -15.0f };
	mountain.scale = glm::vec3{ 1 };

	auto mountain2{ assimpLoad("models/garden/Terrain/mountain/scene2.gltf", true) };
	mountain2.position = glm::vec3{ 13.0f, 1.58f , 2.5f };
	mountain2.scale = glm::vec3{ 1 };
	mountain2.orientation.y = -M_PI / 2;

	// Floating island
	auto floating{ assimpLoad("models/garden/Terrain/floating/scene.gltf", true) };
	floating.position = glm::vec3{ 6.75f, 4.5f, -5.0f };
	floating.scale = glm::vec3{ 1 };

	// Rock pile
	auto boulders{ assimpLoad("models/garden/Terrain/boulders/scene.gltf", true) };
	boulders.scale = glm::vec3{ 0.7 };
	boulders.position = glm::vec3{ -9.5f,  0.9f,  11.5f };
	boulders.orientation.y = glm::radians(130.0f);

	// River
	auto river{ assimpLoad("models/garden/Terrain/river/scene.gltf", true) };
	river.position = glm::vec3{ -3.0f, 0.025f, -1.15f };
	river.scale = glm::vec3{ 0.8 };
	river.orientation.y = glm::radians(-140.0f);
	auto river2{ river };
	river2.position = glm::vec3{ -8.0f, 0.025f, 5.0f };
	river2.orientation.y = glm::radians(120.0f);

	// Pond
	auto pond{ assimpLoad("models/garden/Terrain/pond/scene2.gltf", true) };
	pond.position = glm::vec3{ 4.8f, 0.133f, -4.0f };
	pond.scale = glm::vec3{ 0.2 };
	pond.orientation.y = -M_PI / 2;

	// Add to group
	terrain.children.push_back(std::move(mountain));
	terrain.children.push_back(std::move(mountain2));
	terrain.children.push_back(std::move(boulders));
	terrain.children.push_back(std::move(river));
	terrain.children.push_back(std::move(river2));
	terrain.children.push_back(std::move(floating));
	terrain.children.push_back(std::move(pond));

	return terrain;
}

SceneObject trees() {
	struct Tree {
		int type;
		glm::vec3 position;
	};

	// Parent
	SceneObject forest{ groupTemplate };

	// Assets list
	std::vector<SceneObject> treeTemplates{
		assimpLoad("models/garden/Plants/Trees/1/scene.gltf", true),
		assimpLoad("models/garden/Plants/Trees/sakura/scene.gltf", true)
	};
	treeTemplates[1].scale = glm::vec3{ 2.0f };

	std::vector<Tree> treeData{
		{ 0, { -4.5f, 0.0f, -4.0f } },
		{ 0, { -2.5f, 0.0f, -8.5f } },
		{ 0, {  7.8f, 0.0f, -9.9f } },
		{ 1, { -7.0f, 0.0f, -9.0f } },

		{ 0, {  1.5f, 0.0f, -2.5f } },
		{ 1, {  9.0f, 0.0f,  4.0f } },
		{ 0, {  4.2f, 0.0f,  7.0f } },
		{ 1, {  1.0f, 0.0f, -6.0f } },

		{ 0, { -7.5f, 0.0f,  0.5f } },
		{ 0, {  1.5f, 0.0f, -2.5f } },
		{ 0, {  4.8f, 0.0f,  2.0f } },
		{ 1, { -3.5f, 0.0f,  2.5f } },

		{ 0, {  2.5f, 0.0f, -8.5f } },
	};

	// Iterate positions and place a copy of the tree
	for (size_t i = 0; i < treeData.size(); ++i) {
		const Tree& data{ treeData[i] };

		auto tree{ treeTemplates[data.type] };
		tree.position = data.position;

		// Varied size and orientation
		tree.scale = glm::vec3{ 0.2f + 0.05f * static_cast<float>(i % 4) };
		tree.orientation.y = 20.0f * static_cast<float>(i);

		forest.children.push_back(std::move(tree));
	}

	return forest;
}

SceneObject grass() {
	SceneObject grassGroup{ groupTemplate };

	// Template
	auto grassTemplate{ assimpLoad("models/garden/plants/grass/scene.gltf", true) };

	// Placements
	std::vector<glm::vec3> positions{
		{ -1.3f, 0.0f,  5.9f },
		{ -2.2f, 0.0f,  1.8f },
		{  2.2f, 0.0f,  1.8f },
		{ -1.1f, 0.0f,  5.1f },
		{  3.1f, 0.0f,  2.7f },
		{ -3.1f, 0.0f,  2.7f },
		{ -4.3f, 0.0f,  3.6f },
		{  4.3f, 0.0f,  3.6f },
		{  5.2f, 0.0f,  4.5f },
		{ -5.2f, 0.0f,  4.5f },
		{ -2.3f, 0.0f,  4.3f },
		{  3.2f, 0.0f,  3.2f },
		{ -3.2f, 0.0f,  3.2f },
		{  1.1f, 0.0f,  5.4f },
		{  2.3f, 0.0f,  4.3f },
		{ -1.1f, 0.0f,  2.1f },
		{  1.3f, 0.0f,  5.9f },
		{  4.1f, 0.0f,  2.1f },
		{ -6.0f, 0.0f,  1.0f },
		{ -2.3f, 0.0f, -6.9f },
		{ -2.2f, 0.0f, -5.8f },
		{  2.2f, 0.0f, -6.8f },
		{ -1.1f, 0.0f, -7.1f },
		{  3.1f, 0.0f, -8.7f },
		{ -3.1f, 0.0f, -9.7f },
		{ -4.3f, 0.0f, -9.2f },
		{  4.3f, 0.0f, -6.6f },
		{  5.2f, 0.0f, -5.5f },
		{ -5.2f, 0.0f, -6.5f },
		{ -2.3f, 0.0f, -7.3f },
		{  3.2f, 0.0f, -8.2f },
		{ -3.2f, 0.0f, -9.2f },
		{  1.1f, 0.0f, -9.4f },
		{  2.3f, 0.0f, -9.3f },
		{ -1.1f, 0.0f, -8.1f },
		{  1.3f, 0.0f, -7.9f },
		{  4.1f, 0.0f, -6.1f },
		{ -6.0f, 0.0f, -5.0f },
	};

	// Iterate and place copies
	for (size_t i = 0; i < positions.size(); ++i) {
		auto grass{ grassTemplate };
		
		grass.position = positions[i];
		grass.scale = glm::vec3{ 0.2f + 0.05f * static_cast<float>(i % 4) };
		grass.orientation.y = 0.4f * static_cast<float>(i);

		grassGroup.children.push_back(std::move(grass));
	}
	return grassGroup;
}

SceneObject pathway() {
	struct Path {
		glm::vec3 position;
		float yawDegrees;
	};

	SceneObject walkway{ groupTemplate };
	auto pathTemplate{ assimpLoad("models/garden/Misc/path/scene.gltf", true) };
	pathTemplate.scale = glm::vec3{ 0.8f };

	// Placement
	std::vector<Path> pathData{
		{ { -6.1f, 0.0f, -1.0f },  55.0f },  // gazebo

		{ { -2.0f, 0.0f,  2.0f }, 225.0f },  // river (left)
		{ { -0.6f, 0.0f,  4.5f }, 200.0f },

		{ { -1.5f, 0.0f,  7.5f }, -40.0f },  // behind
		{ { -3.0f, 0.0f,  9.8f }, -20.0f },

		{ {  2.2f, 0.0f,  6.0f }, 100.0f },  // right
		{ {  4.8f, 0.0f,  5.1f }, 120.0f },
		{ {  6.7f, 0.0f,  3.2f }, 150.0f },
		{ {  8.0f, 0.0f,  1.0f }, -30.0f },
		{ {  9.9f, 0.0f, -1.0f }, -50.0f },
	};
	// Pre-allocate (not sure if theres actually any noticable performance gain i just read its good practice or something)
	walkway.children.reserve(pathData.size());

	// Iterate and set pos/yaw
	for (const Path& data : pathData) {
		auto pathSegment{ pathTemplate };
		pathSegment.position = data.position;
		pathSegment.orientation.y = glm::radians(data.yawDegrees);
		walkway.children.push_back(std::move(pathSegment));
	}

	return walkway;
}

SceneObject misc() {
	SceneObject miscObjs{ groupTemplate };

	// Lamp
	SceneObject lamps;
	auto lampTemplate{ assimpLoad("models/garden/Misc/lamp/scene.gltf", true) };
	lampTemplate.scale = glm::vec3{ 2.0f };

	std::vector<glm::vec3> lampPositions{
		{ -2.0f, 0.0f, -7.5f },
		{  4.5f, 0.0f, -1.5f },
		{ -8.5f, 0.0f,  1.5f },
		{  6.0f, 0.0f,  7.0f }
	};
	lamps.children.reserve(lampPositions.size());

	for (const auto& position : lampPositions) {
		SceneObject lamp{ lampTemplate };
		lamp.position = position;
		lamps.children.push_back(std::move(lamp));
	}

	// Sword in stone
	auto sword{ assimpLoad("models/garden/Misc/sword/scene.gltf", true) };
	sword.position = glm::vec3{ -10.0f, 0.0f, -10.0f };
	sword.scale = glm::vec3{ 1.0 };

	// Circular platform
	auto platform{ assimpLoad("models/garden/Misc/circular/scene.gltf", true) };
	platform.position = glm::vec3{ 0.0f, 0.12f, 6.0f };
	platform.scale = glm::vec3{ 0.5 };

	// Gazebo
	auto gazebo{ assimpLoad("models/garden/Misc/gazebo/scene.gltf", true) };
	gazebo.position = glm::vec3{ -8.3f, 0.0f, -2.9f };
	gazebo.scale = glm::vec3{ 3 };
	gazebo.orientation.y = glm::radians(50.0f);

	// Bridge
	auto bridge{ assimpLoad("models/garden/Misc/bridge/scene.gltf", true) };
	bridge.scale = glm::vec3{ 1.7 };
	bridge.position = glm::vec3{ -4.0f, 0.0f, 0.5f };
	bridge.orientation.y = glm::radians(-35.0f);

	// Gandalf
	auto gandalf{ assimpLoad("models/garden/Misc/gandalf/scene.gltf", true) };
	gandalf.scale = glm::vec3{ 1.7 };
	gandalf.position = glm::vec3{ -0.8, 0.0f, -2.0f };
	gandalf.orientation.y = glm::radians(-20.0f);

	// Add to group
	miscObjs.children.push_back(std::move(lamps));
	miscObjs.children.push_back(std::move(sword));
	miscObjs.children.push_back(std::move(platform));
	miscObjs.children.push_back(std::move(gazebo));
	miscObjs.children.push_back(std::move(bridge));
	miscObjs.children.push_back(std::move(gandalf)); 

	return miscObjs;
}

// Garden scene
Scene garden() {
	Scene scene{ phongLightingShader() };  
	groupTemplate.position = groundPos;    // set template scene's position for group functions to copy

	// Grass texture
	Texture grassTex = loadTexture("models/garden/Textures/grass_baseColor.tga", "baseTexture");

	glBindTexture(GL_TEXTURE_2D, grassTex.textureId);              // bind
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);  // repeat in u direction
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);  // repeat in v direction
	glBindTexture(GL_TEXTURE_2D, 0);                               // unbind

	std::vector<Texture> textures{ grassTex };
	auto mesh{ Mesh::squareTiled(textures, 12.5f) };

	// Create floor
	SceneObject floor;
	floor.meshes.push_back(mesh);
	floor.scale = glm::vec3{ 25.0f };
	floor.position = groundPos;
	floor.orientation = glm::vec3{ -M_PI / 2.0f, 0.0f, 0.0f };

	// Move all objects into the scene's objects list.
	scene.objects.push_back(std::move(floor));
	scene.objects.push_back(campsite());
	scene.objects.push_back(terrain());
	scene.objects.push_back(trees());
	scene.objects.push_back(grass());
	scene.objects.push_back(misc());
	scene.objects.push_back(pathway());

	return scene;
}


/*****************************************************************************************
*  RUNTIME HELPERS
* 
*  These help remove clutter from main
*  They handle camera/event input, matrix caching, animation, and point lighting
*****************************************************************************************/

struct CameraState {
	// Source: https://learnopengl.com/Getting-started/Camera
	bool locked{ true };

	float sensitivity{ 0.1f };
	float speed{ 12.5f };
	float scrollSpeed{ 1.0f };

	float yaw{ -90.0f };
	float pitch{ 0.0f };

	glm::vec3 position{ 0.0f, 0.0f, 5.0f };
	glm::vec3 front{ 0.0f, 0.0f, -1.0f };
	glm::vec3 up{ 0.0f, 1.0f, 0.0f };
};

struct MatrixCache {
	glm::mat4 projection{ 1.0f };
	glm::mat4 view{ 1.0f };

	bool projectionDirty{ true };  // true means needs updating
	bool viewDirty{ true };        //
};

void setDefaultUniforms(ShaderProgram& program) {
	program.setUniform("ambientColor", glm::vec3{ 0.2f, 0.2f, 0.24f });
	program.setUniform("directionalLight", glm::normalize(glm::vec3{ 0.2f, -1.0f, 0.3f }));
	program.setUniform("directionalColor", glm::vec3{ 0.35f, 0.35f, 0.40f });

	// material = k_a, k_d, k_s, shininess
	program.setUniform("material", glm::vec4{ 0.2f, 1.0f, 1.0f, 16.0f });

	// Overwritten by drawMesh()
	program.setUniform("hasEmission", 0);
	program.setUniform("hasSpecMap", 0);

	program.setUniform("emissiveStrength", 2.5f);
}

// Handles window close/resize, camera lock toggle, raw mouse movement, and mouse scrolling
void handleEvents(sf::Window& window, CameraState& camera, MatrixCache& matrices) {
	while (const std::optional event{ window.pollEvent() }) {
		if (event->is<sf::Event::Closed>()) {
			window.close();
		}
		if (event->is<sf::Event::Resized>()) {
			auto size = window.getSize();
			glViewport(0, 0, size.x, size.y);
			matrices.projectionDirty = true;
		}

		// Toggle camera lock 
		if (event->is<sf::Event::KeyPressed>()) {
			if (event->getIf<sf::Event::KeyPressed>()->scancode == sf::Keyboard::Scancode::Escape) {
				camera.locked = !camera.locked;                // reverse flag
				window.setMouseCursorVisible(camera.locked);   // hide mouse
				window.setMouseCursorGrabbed(!camera.locked);  // prevent mouse from leaving window
			}
		}

		// Skip if locked
		if (camera.locked) continue;
		
		// Mouse (look around)
		if (const auto* mouseMovedRaw = event->getIf<sf::Event::MouseMovedRaw>()) {
			camera.yaw   += mouseMovedRaw->delta.x * camera.sensitivity;
			camera.pitch -= mouseMovedRaw->delta.y * camera.sensitivity;

			// Limit pitch to prevent flipping
			if (camera.pitch >  89.0f) camera.pitch =  89.0f;
			if (camera.pitch < -89.0f) camera.pitch = -89.0f;

			// Convert to direction vector
			glm::vec3 direction;
			direction.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
			direction.y = sin(glm::radians(camera.pitch));
			direction.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));

			camera.front = glm::normalize(direction);
			matrices.viewDirty = true;
		}

		// Scroll (zoom/move back and forth)
		if (const auto* wheel = event->getIf<sf::Event::MouseWheelScrolled>()) {
			camera.position += camera.front * (wheel->delta * camera.scrollSpeed);
			matrices.viewDirty = true;
		}
	}
}

// Handles WASD camera movement
// Outside of handleEvents() so it runs every frame and movement is smooth
void updateCameraMovement(CameraState& camera, MatrixCache& matrices, float dt) {
	if (camera.locked) return;

	float cameraSpeed{ camera.speed * dt }; // ensure consistent speed despite fps

	// Flatten front for level W/S movement
	glm::vec3 forwardFlat{ camera.front.x, 0.0f, camera.front.z };
	// Prevent division by 0
	forwardFlat = glm::length(forwardFlat) > 0.0001f
		? glm::normalize(forwardFlat)
		: glm::vec3{ 0.0f, 0.0f, -1.0f }; // move forwards

	glm::vec3 right = glm::normalize(glm::cross(forwardFlat, camera.up));  // calc right vec

	// XZ plane
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
		camera.position += cameraSpeed * forwardFlat;
		matrices.viewDirty = true;
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) {
		camera.position -= cameraSpeed * forwardFlat;
		matrices.viewDirty = true;
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
		camera.position -= cameraSpeed * right;
		matrices.viewDirty = true;
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
		camera.position += cameraSpeed * right;
		matrices.viewDirty = true;
	}

	// Vertical
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)) {
		camera.position += cameraSpeed * camera.up;
		matrices.viewDirty = true;
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) {
		camera.position -= cameraSpeed * camera.up;
		matrices.viewDirty = true;
	}
}

// Recomputes matrices only when needed
void updateMatrices(sf::Window& window, ShaderProgram& program, const CameraState& camera, MatrixCache& matrices) {
	// Projection
	if (matrices.projectionDirty) {
		matrices.projection = glm::perspective(glm::radians(45.0f), static_cast<float>(window.getSize().x) / window.getSize().y, 0.1f, 100.0f);
		matrices.projectionDirty = false;
	}

	// View
	if (matrices.viewDirty) {
		matrices.view = glm::lookAt(camera.position, camera.position + camera.front, camera.up);
		matrices.viewDirty = false;
	}

	// Uniforms
	program.setUniform("view", matrices.view);
	program.setUniform("projection", matrices.projection);
	program.setUniform("cameraPos", camera.position);
}

// Animates flame (child of bonfire) using sine waves
float animateFlame(SceneObject& flame, float totalTime) {
	float slowPulse = 0.5f + 0.5f * sin(totalTime * 6.0f);        // 0.5f + 0.5f * sin(...) -> sin() from [-1,1] to [0,1] 
	float fastPulse = 0.5f + 0.5f * sin(totalTime * 17.0f);       // totalTime * x --> x controls speed
	float combinedPulse = 0.75f * slowPulse + 0.25f * fastPulse;  // less rhythmic/more natural flicker (numbers are ratios)

	const glm::vec3 baseFlameScale{ 1.35f, 1.75f, 1.35f };

	flame.scale = glm::vec3{
		baseFlameScale.x * (0.92f + 0.6f * fastPulse),       // min size + range
		baseFlameScale.y * (0.95f + 0.18f * combinedPulse),  // ex: 0.85 + 0.30 ranges from 0.85 to 1.15 percent
		baseFlameScale.z * (0.92f + 0.6f * slowPulse)        //     of its original size
	};

	flame.orientation.y = 0.10f * sin(totalTime * 5.0f);
	flame.position.y = 0.70f + 0.04f * combinedPulse;

	return 0.65f + 0.55f * combinedPulse;
}

// Send point light positions/colors to lighting.frag
void setPointLightUniforms(ShaderProgram& program, const SceneObject& campGroup, const SceneObject& lamps, const SceneObject& sword, float fireBrightness) {
	std::vector<glm::vec3> lightPositions;
	std::vector<glm::vec3> lightColors;

	lightPositions.reserve(MAX_POINT_LIGHTS);
	lightColors.reserve(MAX_POINT_LIGHTS);

	// Campfire point light
	lightPositions.push_back(campGroup.position + glm::vec3{ 0.0f, 0.6f, 0.0f });
	lightColors.push_back(glm::vec3{ 5.0f, 2.0f, 0.6f } * fireBrightness);

	// Lamp point lights
	for (const auto& lamp : lamps.children) {
		if (static_cast<int>(lightPositions.size()) >= MAX_POINT_LIGHTS)
			break;
		
		lightPositions.push_back(lamps.position + lamp.position + glm::vec3{ 0.0f, 1.5f, 0.0f });
		lightColors.push_back(glm::vec3{ 4.0f, 3.0f, 1.4f });
	}

	// Sword in stone point light
	lightPositions.push_back(sword.position);
	lightColors.push_back(glm::vec3{ 6.0f, 4.5f, 2.1f });

	// Set uniforms
	const int numLights = static_cast<int>(lightPositions.size());
	program.setUniform("numPointLights", numLights);

	for (int i = 0; i < numLights; ++i) {
		program.setUniform("pointLightPos[" + std::to_string(i) + "]", lightPositions[i]);
		program.setUniform("pointLightColor[" + std::to_string(i) + "]", lightColors[i]);
	}
}

int main() {
	std::cout << std::filesystem::current_path() << std::endl;

	// Initialize the window and OpenGL.
	sf::ContextSettings settings;
	settings.depthBits = 24;   // Request a 24 bits depth buffer
	settings.stencilBits = 8;  // Request a 8 bits stencil buffer
	settings.majorVersion = 3; // You might have to change these on Mac.
	settings.minorVersion = 3;

	sf::Window window{
		sf::VideoMode::getFullscreenModes().at(0), "Modern OpenGL",
		sf::Style::Resize | sf::Style::Close,
		sf::State::Windowed, settings
	};

	gladLoadGL();
	glEnable(GL_DEPTH_TEST);                            // enable depth
	glEnable(GL_BLEND);                                 // enable alpha blending
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  //
	glClearColor(0.03f, 0.05f, 0.12f, 1.0f);            // change background color (TODO: skybox later if i have time)

	// Inintialize scene objects
	auto myScene{ garden() };

	// References to specific objects
	auto& campGroup{ myScene.objects[1]    }; 
	auto& bonfire  { campGroup.children[0] };
	auto& flame    { bonfire.children[1]   };

	auto& miscGroup{ myScene.objects[5]    };
	auto& lamps    { miscGroup.children[0] };
	auto& sword    { miscGroup.children[1] };

	// Activate the shader program.
	myScene.program.activate();
	setDefaultUniforms(myScene.program);

	// Ready, set, go!
	sf::Clock clock;
	float totalTime{};
   
	// Create camera and matrix structs
	CameraState camera;
	MatrixCache matrices;

	// Rendering loop
	while (window.isOpen()) {
		// Get time since last frame in seconds
		float dt = clock.restart().asSeconds();
		totalTime += dt;
		dt = std::min(dt, 0.016f);  // clamp

		// Events
		handleEvents(window, camera, matrices);
		updateCameraMovement(camera, matrices, dt);
		updateMatrices(window, myScene.program, camera, matrices);
		
		// Lighting
		float fireBrightness = animateFlame(flame, totalTime);                            // animate bonfire's flame
		setPointLightUniforms(myScene.program, campGroup, lamps, sword, fireBrightness);  // update point light uniforms
		myScene.program.setUniform("emissiveStrength", 1.8f * fireBrightness);            // update emissive strength
		
		// Clear the OpenGL "context".
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// Render the scene objects.
		for (auto& o : myScene.objects) {
			o.drawObject(myScene.program);
		}
		window.display();
	}

	return 0;
}


