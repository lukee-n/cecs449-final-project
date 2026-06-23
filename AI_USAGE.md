# AI Usage Journal

**Course:** CECS 449-01  
**Assignment:** Individual Project  
**Student:** Luke Nguyen  
**Date:** 2026-05-13

---

## 1. Tools Used
List any AI tools you used (by name and version if known).  
Examples: ChatGPT (GPT-4, March 2025), GitHub Copilot (VSCode extension), Claude 3.5.

- [ ] No AI tools were used for this assignment  
- [x] AI tools were used (listed below):

**Tool(s):**  ChatGPT 5

## 2. Prompts / Queries
Identify the *exact* prompts or queries you entered into the AI, and what you did with them.
If too long, include representative excerpts. For each prompt, record:

### a. Prompt text
> "how do i convert an asset from fbx to gltf in blender?"

> "the assimp loader is fine i need to fix the textures on the blender side. they look fine in blender but dont render in my program. ive noticed that image textures for principled bsdf look fine when exported to my program"


### b. Raw AI Output(s)
What I’d do in Blender
For each material, rebuild it like this:

1. Use one clean Principled BSDF

In the Shader Editor:
```
Image Texture → Base Color
Principled BSDF → Material Output
```
For the base color texture:

* Color Space: sRGB
* Connect to Base Color

For normal maps:
```
Image Texture → Normal Map → Principled BSDF Normal
```

Set the normal texture to:

* Color Space: Non-Color

For roughness / metallic maps:

* Use **Non-Color**
* Connect directly to Roughness or Metallic

Important: bake anything procedural
If the texture is not a normal image file plugged into Principled BSDF, bake it.

Basic bake workflow:

1. Select the object.
2. Make sure it has UVs: <br>
   **Edit Mode → A → U → Smart UV Project if needed.**
3. In Shader Editor, create a new Image Texture node.
4. Click New, name it something like my_asset_basecolor.
5. Select that new Image Texture node.
6. Switch Render Engine to Cycles.
7. Go to Render Properties → Bake.
8. Bake Diffuse, with only Color enabled.
9. Save the baked image: <br>
   **Image Editor → Image → Save As**
10. Replace the old material with:  
```
Baked Image Texture → Principled BSDF Base Color
```
Then export again.

Export settings I’d use
Use:
```
File → Export → glTF 2.0
```
Recommended settings:
```
Format: glTF Separate
Images: Copy
Materials: Export
Geometry: UVs enabled
Geometry: Normals enabled
```
For your class/OpenGL project, glTF Separate is easier to debug because you can see the .gltf, .bin, and image files separately.

### c. What I Accepted, Modified, or Rejected
Briefly explain what you did with the AI output of this prompt.

- **Accepted:** Using blender to convert fbx to gltf and baking image textures to ensure they export correctly
- **Modified:** N/A
- **Rejected:** N/A 


## 3. Reflection
I was having trouble finding assets that were compatible with the program and this opened up a lot of options asset-wise with some time investment in Blender to make them compatible.  
This also helped me experiment with other blender features to modify assets such as stretching faces and adjusting vertices.  
This led to me figuring out how to make an emissive texture for my bonfire asset.