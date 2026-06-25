///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	// start with no textures loaded so texture slots are assigned correctly.
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	DestroyGLTextures();

	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// If this cleanup step is not done, the textures will remain in GPU memory
		// until the application is closed, which can lead to memory leaks
		// and inefficient resource usage.
		if (m_textureIDs[i].ID != 0)
		{
			// glDeleteTextures is used to free the texture memory associated with the texture ID.
			glDeleteTextures(1, &m_textureIDs[i].ID);
			m_textureIDs[i].ID = 0;
			m_textureIDs[i].tag.clear();
		}
	}

	// reset the texture count of loaded textures when destroyed
	m_loadedTextures = 0;
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	// return correct value if the material was found or not
	return bFound;
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/**************************************************************/
/*** DefineObjectMaterials()								***/
/***														***/
/*** This method defines the material properties used by	***/
/*** the objects in the scene. The values control how each  ***/
/*** surface reacts to ambient, diffuse, and specular light.***/
/**************************************************************/
void SceneManager::DefineObjectMaterials()
{
	// Materials are defined separately from rendering so each object can
	// have its own lighting behavior without changing the geometry code.
	// This scene uses distinct materials for aluminum, dark glass, drywall,
	// filament, printed plastic, screen glow, and wood so the objects do not
	// all react to light the same way.

	// Brushed aluminum-style material for the 3D printer frame.
	// This gives the printer body a slightly brighter, more reflective
	// response than the matte printed plastic objects in the scene.
	OBJECT_MATERIAL aluminumMaterial;
	aluminumMaterial.ambientColor = glm::vec3(0.35f, 0.35f, 0.33f);
	aluminumMaterial.ambientStrength = 0.18f;
	aluminumMaterial.diffuseColor = glm::vec3(0.72f, 0.72f, 0.68f);
	aluminumMaterial.specularColor = glm::vec3(0.45f, 0.45f, 0.42f);
	aluminumMaterial.shininess = 24.0f;
	aluminumMaterial.tag = "aluminum";
	m_objectMaterials.push_back(aluminumMaterial);

	// Blue-gray 3D printed plastic material for the caddy accent pieces.
	// This uses a slightly stronger highlight than the main body so the
	// drawer faces and patterned sections are easier to see, while still
	// keeping the overall look consistent with matte printed plastic.
	OBJECT_MATERIAL bluePlasticMaterial;
	bluePlasticMaterial.ambientColor = glm::vec3(0.00f, 0.06f, 0.16f);
	bluePlasticMaterial.ambientStrength = 0.18f;
	bluePlasticMaterial.diffuseColor = glm::vec3(0.00f, 0.07f, 0.36f);
	bluePlasticMaterial.specularColor = glm::vec3(0.05f, 0.07f, 0.09f);
	bluePlasticMaterial.shininess = 7.0f;
	bluePlasticMaterial.tag = "bluePlastic";
	m_objectMaterials.push_back(bluePlasticMaterial);

	// Dark tinted glass material for the 3D printer door.
	// This keeps the front panel very dark like the reference printer,
	// but adds a stronger specular response so it reads more like glossy glass.
	OBJECT_MATERIAL darkGlassMaterial;
	darkGlassMaterial.ambientColor = glm::vec3(0.01f, 0.015f, 0.018f);
	darkGlassMaterial.ambientStrength = 0.16f;
	darkGlassMaterial.diffuseColor = glm::vec3(0.03f, 0.035f, 0.04f);
	darkGlassMaterial.specularColor = glm::vec3(0.45f, 0.55f, 0.58f);
	darkGlassMaterial.shininess = 32.0f;
	darkGlassMaterial.tag = "darkGlass";
	m_objectMaterials.push_back(darkGlassMaterial);

	// Drywall material for the backdrop plane.
	// This is kept darker and mostly diffuse so it adds depth behind
	// the objects without creating a bright glowing hotspot.
	OBJECT_MATERIAL drywallSurfaceMaterial;
	drywallSurfaceMaterial.ambientColor = glm::vec3(0.22f, 0.22f, 0.22f);
	drywallSurfaceMaterial.ambientStrength = 0.08f;
	drywallSurfaceMaterial.diffuseColor = glm::vec3(0.42f, 0.42f, 0.40f);
	drywallSurfaceMaterial.specularColor = glm::vec3(0.02f, 0.02f, 0.02f);
	drywallSurfaceMaterial.shininess = 1.0f;
	drywallSurfaceMaterial.tag = "drywallSurface";
	m_objectMaterials.push_back(drywallSurfaceMaterial);

	// Brown-gold filament material for the spool.
	// This darker warm color makes the filament read more like a muted
	// bronze/gold plastic and keeps it from blending into the light tabletop.
	OBJECT_MATERIAL goldFilamentMaterial;
	goldFilamentMaterial.ambientColor = glm::vec3(0.14f, 0.09f, 0.035f);
	goldFilamentMaterial.ambientStrength = 0.22f;
	goldFilamentMaterial.diffuseColor = glm::vec3(0.34f, 0.22f, 0.075f);
	goldFilamentMaterial.specularColor = glm::vec3(0.14f, 0.10f, 0.045f);
	goldFilamentMaterial.shininess = 7.0f;
	goldFilamentMaterial.tag = "goldFilament";
	m_objectMaterials.push_back(goldFilamentMaterial);

	// Matte black 3D printed PETG material for the main body.
	// The low sheen and low specular color keep the caddy from
	// looking too glossy, while still allowing the light to 
	// show its shape and details.
	OBJECT_MATERIAL mattePlasticMaterial;
	mattePlasticMaterial.ambientColor = glm::vec3(0.05f, 0.05f, 0.05f);
	mattePlasticMaterial.ambientStrength = 0.22f;
	mattePlasticMaterial.diffuseColor = glm::vec3(0.20f, 0.20f, 0.20f);
	mattePlasticMaterial.specularColor = glm::vec3(0.08f, 0.08f, 0.08f);
	mattePlasticMaterial.shininess = 4.0f;
	mattePlasticMaterial.tag = "mattePlastic";
	m_objectMaterials.push_back(mattePlasticMaterial);

	// Orange 3D printed plastic material for the loose printed part.
	// This keeps the part bright enough to stand out on the wood surface,
	// while still using a mostly matte finish like printed plastic.
	OBJECT_MATERIAL orangePlasticMaterial;
	orangePlasticMaterial.ambientColor = glm::vec3(0.24f, 0.05f, 0.02f);
	orangePlasticMaterial.ambientStrength = 0.22f;
	orangePlasticMaterial.diffuseColor = glm::vec3(0.70f, 0.10f, 0.025f);
	orangePlasticMaterial.specularColor = glm::vec3(0.14f, 0.04f, 0.025f);
	orangePlasticMaterial.shininess = 6.0f;
	orangePlasticMaterial.tag = "orangePlastic";
	m_objectMaterials.push_back(orangePlasticMaterial);

	// Bright screen material for the 3D printer touch display.
	// The high ambient strength helps the screen look lit even though
	// the shader does not use a true emissive material.
	OBJECT_MATERIAL screenGlowMaterial;
	screenGlowMaterial.ambientColor = glm::vec3(0.02f, 0.45f, 0.55f);
	screenGlowMaterial.ambientStrength = 0.85f;
	screenGlowMaterial.diffuseColor = glm::vec3(0.05f, 0.75f, 0.90f);
	screenGlowMaterial.specularColor = glm::vec3(0.20f, 0.85f, 0.95f);
	screenGlowMaterial.shininess = 18.0f;
	screenGlowMaterial.tag = "screenGlow";
	m_objectMaterials.push_back(screenGlowMaterial);

	// Light wood workstation material for the base plane.
	// The workstation surface has a slight sheen, so it gets more specular reflection than the caddy, 
	// but it still has a low shininess value to keep the highlights broad and soft, 
	// which is more consistent with wood surfaces.
	OBJECT_MATERIAL woodSurfaceMaterial;
	woodSurfaceMaterial.ambientColor = glm::vec3(0.35f, 0.28f, 0.18f);
	woodSurfaceMaterial.ambientStrength = 0.18f;
	woodSurfaceMaterial.diffuseColor = glm::vec3(0.55f, 0.42f, 0.25f);
	woodSurfaceMaterial.specularColor = glm::vec3(0.22f, 0.18f, 0.12f);
	woodSurfaceMaterial.shininess = 12.0f;
	woodSurfaceMaterial.tag = "woodSurface";
	m_objectMaterials.push_back(woodSurfaceMaterial);
}

/**************************************************************/
/*** SetupSceneLights()                                     ***/
/***                                                        ***/
/*** This method configures the lighting for the full 3D    ***/
/*** printer workstation. The scene uses a main overhead    ***/
/*** shop light, a softer front fill light, a small screen  ***/
/*** accent light, and a subtle background light to create  ***/
/*** better depth, shadow contrast, and visual polish.      ***/
/**************************************************************/
void SceneManager::SetupSceneLights()
{
	// Main overhead shop light for the full workstation.
	// Slightly reduced so the wall and tabletop do not become too bright.
	m_pShaderManager->setVec3Value("lightSources[0].position", glm::vec3(-1.0f, 8.5f, 1.0f));
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", glm::vec3(0.035f, 0.035f, 0.035f));
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", glm::vec3(0.46f, 0.44f, 0.40f));
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", glm::vec3(0.24f, 0.23f, 0.21f));
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 16.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.12f);

	// Softer front fill light.
	// This brightens the objects from the camera side without increasing
	// the bright hotspot on the wall behind the scene.
	m_pShaderManager->setVec3Value("lightSources[1].position", glm::vec3(0.0f, 4.0f, 7.5f));
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", glm::vec3(0.03f, 0.028f, 0.025f));
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", glm::vec3(0.38f, 0.36f, 0.33f));
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", glm::vec3(0.12f, 0.11f, 0.10f));
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 24.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.08f);

	// Small accent light near the printer touch screen. This is kept
	// subtle because the screen geometry already provides the visible glow.
	// The light helps make the screen feel intentional in the scene.
	m_pShaderManager->setVec3Value("lightSources[2].position", glm::vec3(-10.15f, 7.2f, 2.65f));
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", glm::vec3(0.015f, 0.035f, 0.04f));
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", glm::vec3(0.08f, 0.22f, 0.26f));
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", glm::vec3(0.08f, 0.30f, 0.35f));
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 8.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.14f);

	// Fourth light slot initialized as a very soft background light.
	// This keeps the shader values controlled while avoiding an extra
	// strong light source that could flatten the scene.
	m_pShaderManager->setVec3Value("lightSources[3].position", glm::vec3(-6.0f, 5.0f, -5.5f));
	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", glm::vec3(0.005f, 0.005f, 0.006f));
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", glm::vec3(0.07f, 0.07f, 0.075f));
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", glm::vec3(0.02f, 0.02f, 0.025f));
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 20.0f);
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.04f);

	// Turn on lighting in the shader for the rendered scene.
	m_pShaderManager->setIntValue(g_UseLightingName, true);
}

/**************************************************************/
/*** LoadSceneTextures()                                    ***/
/***                                                        ***/
/*** This method loads all texture files used by the scene. ***/
/*** Keeping texture loading separate from PrepareScene()   ***/
/*** makes the setup easier to maintain as more objects and ***/
/*** materials are added to the final project.              ***/
/**************************************************************/
void SceneManager::LoadSceneTextures() 
{
	/**************************************************************/
	/***               3D printed object textures               ***/
	/**************************************************************/
	// These textures add printed-plastic detail to the caddy and
	// loose printed part so the objects do not rely only on flat color.
	CreateGLTexture("../../Utilities/textures/bluePrinted.jpg", "bluePrinted");
	CreateGLTexture("../../Utilities/textures/cheddar.jpg", "cheddar");
	CreateGLTexture("../../Utilities/textures/darkPrinted.jpg", "darkPrinted");
	CreateGLTexture("../../Utilities/textures/hexPattern.jpg", "hexPattern");

	/**************************************************************/
	/***               Environment textures                     ***/
	/**************************************************************/
	// These textures help the scene feel like a real workspace by
	// giving the table and wall more surface detail.
	CreateGLTexture("../../Utilities/textures/drywall.jpg", "drywall");
	CreateGLTexture("../../Utilities/textures/rusticwood.jpg", "rusticwood");

	/**************************************************************/
	/***               Printer and filament textures            ***/
	/**************************************************************/
	// These textures separate the printer, tinted panels, and filament
	// spool from the matte 3D printed plastic objects.
	CreateGLTexture("../../Utilities/textures/filament.jpeg", "filament");
	CreateGLTexture("../../Utilities/textures/glass.png", "glass");
	CreateGLTexture("../../Utilities/textures/stainless.jpg", "stainless");

	// Bind the textures once after loading them so they can be reused by tag
	// when each part of the scene is rendered.
	BindGLTextures();
}


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// Load and bind all the textures for the scene.
	LoadSceneTextures();

	// Define the materials for the objects in the scene to 
	// control how they react to the light sources.
	DefineObjectMaterials();

	// Set up the light sources for the scene to add depth
	// and realism to the rendered objects.
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadPrismMesh();
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadTorusMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/******************************************************************/
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/

	/******************************************************************/
	/***			Light wood workstation / base plane.			***/
	/******************************************************************/
	// set the scale for the wood workstation under the 3D printer scene
	// giving the scene a clear base surface for camera navigation
	// and a more realistic setting for the 3D printer workstation scene.
	scaleXYZ = glm::vec3(12.0f, 1.0f, 4.0f);

	// keep the plane flat and aligned with scene.
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// move the plane slightly below the scene objects so 
	// it works as a workstation surface and prevents z-position 
	// conflicts with the other meshes in the scene.
	positionXYZ = glm::vec3(0.0f, 0.0f, -1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the material for the workstation surface.
	// The wood material has a slight sheen so the desk reflects light
	// more than the matte caddy, but still keeps the highlights soft.
	SetShaderTexture("rusticwood");

	// Repeat the wood texture more across the wide workstation plane.
	// This keeps the grain from looking like one stretched image.
	SetTextureUVScale(4.0f, 2.0f);

	// Set the material for the workstation surface to give it a slight sheen and some light reflection
	SetShaderMaterial("woodSurface");

	// draw the base plane for the 3D printer scene
	m_basicMeshes->DrawPlaneMesh();

	/******************************************************************/
	/***              Front edge of the wood tabletop               ***/
	/******************************************************************/
	// Thin wood box along the front of the workstation. This gives the
	// tabletop visible thickness so it looks more like the real workbench
	// from the reference image instead of only a flat plane.
	scaleXYZ = glm::vec3(24.0f, 0.35f, 8.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, -0.18f, -1.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("rusticwood");
	SetTextureUVScale(4.0f, 0.5f);
	SetShaderMaterial("woodSurface");

	m_basicMeshes->DrawBoxMesh();

	/******************************************************************/
	/***                  Black metal table legs                    ***/
	/******************************************************************/
	// Simple black metal legs under the tabletop, matching the workbench
	// structure in the reference photo.
	scaleXYZ = glm::vec3(0.50f, 6.2f, 0.50f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// front left leg
	positionXYZ = glm::vec3(-11.0f, -3.45f, 2.15f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.02f, 0.02f, 0.02f, 1.0f);
	SetShaderMaterial("mattePlastic");

	m_basicMeshes->DrawBoxMesh();

	// front right leg
	positionXYZ = glm::vec3(11.0f, -3.45f, 2.15f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.02f, 0.02f, 0.02f, 1.0f);
	SetShaderMaterial("mattePlastic");

	m_basicMeshes->DrawBoxMesh();

	// back left leg
	positionXYZ = glm::vec3(-11.0f, -3.45f, -4.15f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.02f, 0.02f, 0.02f, 1.0f);
	SetShaderMaterial("mattePlastic");

	m_basicMeshes->DrawBoxMesh();

	// back right leg
	positionXYZ = glm::vec3(11.0f, -3.45f, -4.15f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.02f, 0.02f, 0.02f, 1.0f);
	SetShaderMaterial("mattePlastic");

	m_basicMeshes->DrawBoxMesh();

	/******************************************************************/
	/***		Backdrop plane to add more realism to the scene		***/
	/******************************************************************/
	scaleXYZ = glm::vec3(20.0f, 1.0f, 11.0f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 3.0f, -7.0f);

	SetTransformations(
		scaleXYZ, 
		XrotationDegrees, 
		YrotationDegrees, 
		ZrotationDegrees, 
		positionXYZ);

	// Apply the drywall texture to the vertical backdrop.
	// This gives the scene a simple background and helps the dark caddy
	// stand out better when viewing the scene from the camera.
	SetShaderTexture("drywall");

	// Tile the drywall texture evenly so the background stays subtle
	// and does not look stretched behind the caddy.
	SetTextureUVScale(2.0f, 1.5f);

	// Set the material for the drywall surface.
	// The wall stays mostly diffuse so it catches light softly without
	// pulling attention away from the caddy.
	SetShaderMaterial("drywallSurface");
	m_basicMeshes->DrawPlaneMesh();


	/****************************************************************/
	/***               Render custom items for scene              ***/
	/****************************************************************/
	RenderToolCaddy();
	Render3DPrinter();
	RenderFilamentSpool();
	Render3DPrintedPart();
}


/****************************************************************/
/***              Custom 3D Printer Tool Caddy                ***/
/***                                                          ***/
/***  This object represents the 3D printed tool caddy from   ***/
/***  my reference scene. The caddy was refactored for the    ***/
/***  final project after the original large angled prism     ***/
/***  did not texture or align as cleanly as expected. The    ***/
/***  updated top section uses repeated smaller meshes to	  ***/
/***  create  thes individual pen/tool holder slots with      ***/
/***  better visual alignment and cleaner material control.   ***/
/****************************************************************/
void SceneManager::RenderToolCaddy()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/****************************************************************/
	/***               Main body of the tool caddy                ***/
	/****************************************************************/
	// set the scale for the main body
	scaleXYZ = glm::vec3(5.0f, 1.1f, 3.0f);

	// set the XYZ rotation for the main body
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the main body
	positionXYZ = glm::vec3(0.0f, 0.55f, -3.0f);

	// set transformations for the main body
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Repeat the dark printed texture across the main body so the
	// printed surface detail stays subtle instead of stretching.
	SetShaderTexture("darkPrinted");
	SetTextureUVScale(2.0f, 1.0f);

	// Set the material for the main body to give it a matte finish with low specular highlights,
	// which is more consistent with the look of 3D printed plastics and helps the caddy 
	// look more realistic in the scene.
	SetShaderMaterial("mattePlastic");

	// draw the main body mesh
	// The main body provides the base structure for the tool caddy.
	// Future improvements could include independently modeled walls and faces
    // for a more realistic tool base structure.
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	/***     Bottom portion of the top tray of the tool caddy     ***/
	/****************************************************************/
	// set the scale for the bottom tray
	scaleXYZ = glm::vec3(5.01f, 0.185f, 3.01f);
	// set the XYZ rotation for the bottom portion of the top tray
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the bottom tray
	positionXYZ = glm::vec3(0.0f, 1.18f, -3.0f);

	// set transformations for the bottom portion of the top tray
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Use the hex pattern on this accent piece to add visual detail.
	// The UV scale tiles the pattern across the mesh, which is the main
	// advanced texture technique I am using for this milestone.
	SetShaderTexture("bluePrinted");
	SetTextureUVScale(4.0f, 0.25f);

	// Set the material for the bottom tray to give it a slightly higher specular highlight than the main body,
	// which helps the top tray stand out from the rest of the caddy and adds some visual interest to the scene.
	SetShaderMaterial("bluePlastic");

	// draw the bottom portion of the top tray mesh
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	/*** Bottom inlay added to make the top tray look more        ***/
	/*** accurate to the intended object.                         ***/
	/****************************************************************/
	// set the scale for the bottom tray
	scaleXYZ = glm::vec3(4.80f, 0.14f, 0.9f);
	// set the XYZ rotation for the bottom portion of the top tray
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the bottom tray
	positionXYZ = glm::vec3(0.0f, 1.21f, -2.0f);

	// set transformations for the bottom portion of the top tray
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Use the dark printed texture for the inlay so this added piece
	// blends back into the main body instead of looking like a separate part.
	SetShaderTexture("darkPrinted");
	SetTextureUVScale(0.8f, 2.0f);

	// Set the material for the bottom tray inlay to match the main body so it visually connects with the rest of the caddy,
	// while still adding some surface detail from the texture.
	SetShaderMaterial("mattePlastic");

	// draw the bottom portion of the top tray mesh
	m_basicMeshes->DrawBoxMesh();

	/******************************************************************/
	/***        Back row angled dividers for tool holder slots      ***/
	/******************************************************************/
	// Repeated small prism pieces create the back angled dividers for
	// the pen/tool holder slots. I refactored this section from one large
	// textured prism into smaller repeated pieces because the original prism
	// texture mapping tilted the image and did not look intentional.
	scaleXYZ = glm::vec3(1.25f, 0.08f, 0.625f);
	// set the XYZ rotation for the prism portion of the top tool holder
	XrotationDegrees = 135.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	for (int i = 0; i < 8; ++i) {
		// Offset each repeated prism along the X axis to create evenly
		// spaced tool holder slots across the width of the caddy.
		positionXYZ = glm::vec3(-2.46f + (i * 0.702f), 1.49f, -3.55f);

		// set transformations for the prism portion of the top tool holder
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		// Use the dark printed texture and matte material so the dividers
		// match the main 3D printed body of the caddy.
		SetShaderTexture("darkPrinted");
		SetTextureUVScale(0.8f, 0.8f);

		// Set the material for the angled top holder to match the main body so
		// it visually connects with the rest of the caddy, while still adding
		// some surface detail from the texture.
		SetShaderMaterial("mattePlastic");

		// draw the prism portion of the top tool holder mesh
		m_basicMeshes->DrawPrismMesh();
	}

	/******************************************************************/
	/***       Front row angled dividers for tool holder slots      ***/
	/******************************************************************/
	// A second row of smaller angled prisms completes the front edge of
	// the tool holder slots. Using a loop keeps the repeated geometry
	// easier to maintain and lines up the dividers consistently.
	scaleXYZ = glm::vec3(0.50f, 0.08f, 0.25f);
	// set the XYZ rotation for the prism portion of the top tool holder
	XrotationDegrees = 315.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	for (int i = 0; i < 8; ++i) {
		// set the XYZ position for the prism portion of the top tool holder
		positionXYZ = glm::vec3(-2.46f + (i * 0.702f), 1.57f, -3.02f);

		// set transformations for the prism portion of the top tool holder
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		// Use the dark printed texture and matte material so the tool holders 
		// match the caddy's main body.
		SetShaderTexture("darkPrinted");
		SetTextureUVScale(0.8f, 0.8f);

		// Set the material for the angled top holder to match the main body so
		// it visually connects with the rest of the caddy, while still adding
		// some surface detail from the texture.
		SetShaderMaterial("mattePlastic");

		// draw the prism portion of the top tool holder mesh
		m_basicMeshes->DrawPrismMesh();
	}

	

	/******************************************************************/
	/***              Middle cross brace for top holder             ***/
	/******************************************************************/
	// Horizontal brace that connects the repeated angled dividers.
	// This helps the refactored holder read as one connected printed part.
	scaleXYZ = glm::vec3(4.95f, 0.06f, 0.50f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 1.55f, -3.45f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Use the dark printed texture and matte material so the tool holders 
	// match the caddy's main body.
	SetShaderTexture("darkPrinted");
	SetTextureUVScale(0.8f, 0.8f);

	SetShaderMaterial("mattePlastic");

	m_basicMeshes->DrawBoxMesh();

	/******************************************************************/
	/***                 Front cap for top holder                   ***/
	/******************************************************************/
	// Front cap that closes the tool holder section and gives the top
	// of the caddy a cleaner finished edge.
	scaleXYZ = glm::vec3(5.0f, 0.06f, 0.39f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 1.46f, -2.91f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Use the dark printed texture and matte material so the tool holders 
	// match the caddy's main body.
	SetShaderTexture("darkPrinted");
	SetTextureUVScale(0.8f, 0.8f);

	SetShaderMaterial("mattePlastic");

	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	/***            Front blue panel for top holder slots         ***/
	/****************************************************************/
	// Blue printed panel that forms the front wall of the top holder.
	// This finishes the slot area and gives the caddy a cleaner
	// intentional top structure.
	scaleXYZ = glm::vec3(5.0f, 1.14f, 0.06f);
	// set the XYZ rotation for the top tool holder behind prism
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the top tool holder behind prism
	positionXYZ = glm::vec3(0.0f, 1.80f, -3.80f);

	// set transformations for the top tool holder behind prism
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply the blue printed texture to this accent panel so the top
	// holder matches the blue drawer faces while still showing printed
	// surface detail.
	SetShaderTexture("bluePrinted");
	SetTextureUVScale(4.0f, 1.0f);
	SetShaderMaterial("bluePlastic");

	// draw the top tool holder behind prism mesh
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	/***             Back blue panel for top holder slots         ***/
	/****************************************************************/
	// Blue printed panel that forms the back wall of the top holder.
	// This helps the refactored slot section match the drawer faces
	// while keeping the dark dividers visually separate.
	scaleXYZ = glm::vec3(5.0f, 1.14f, 0.06f);
	// set the XYZ rotation for the top tool holder behind prism
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the top tool holder behind prism
	positionXYZ = glm::vec3(0.0f, 1.80f, -4.48f);

	// set transformations for the top tool holder behind prism
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply the blue printed texture to this accent panel so the top
	// holder matches the blue drawer faces while still showing printed
	// surface detail.
	SetShaderTexture("bluePrinted");
	SetTextureUVScale(4.0f, 1.0f);
	SetShaderMaterial("bluePlastic");

	// draw the top tool holder behind prism mesh
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	/***           Blue side panels between top holder slots      ***/
	/****************************************************************/
	// I used a loop here to avoid manually repeating the same divider code
	// for each tool slot. After seeing repeated vertex patterns in the
	// freeCodeCamp OpenGL course, I realized I could use a similar idea here.
	// I am not sure why I did not think to use loops with the repeated mesh
	// pieces earlier, but this made the refactored caddy much cleaner.
	scaleXYZ = glm::vec3(0.68f, 1.135f, 0.06f);
	// set the XYZ rotation for the top tool holder behind prism
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	for (int i = 0; i < 8; ++i) {

		// Move each divider across the X axis to line up with the repeated
		// prism slots and create consistent spacing.
		positionXYZ = glm::vec3(-2.475f + (i * 0.707f), 1.80f, -4.12f);

		// set transformations for the top tool holder behind prism
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		// Apply the blue printed texture to this accent panel so the top
		// holder matches the blue drawer faces while still showing printed
		// surface detail.
		SetShaderTexture("bluePrinted");
		SetTextureUVScale(4.0f, 1.0f);
		SetShaderMaterial("bluePlastic");

		// draw the top tool holder behind prism mesh
		m_basicMeshes->DrawBoxMesh();
	}

	/****************************************************************/
	/***		   Bottom left drawer of the tool caddy			  ***/
	/****************************************************************/
	// set the scale for the bottom left drawer
	scaleXYZ = glm::vec3(2.3f, 0.4f, 2.45f);
	// set the XYZ rotation for the bottom left drawer
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the bottom left drawer
	positionXYZ = glm::vec3(-1.25f, 0.3f, -2.72f);

	// set transformations for the bottom left drawer
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Repeat the hex pattern across the drawer face so the texture
	// keeps a consistent scale instead of stretching across the mesh.
	SetShaderTexture("hexPattern");
	SetTextureUVScale(1.2f, 0.35f);

	// Set the material for the bottom left drawer to match the top tray pieces,
	// which helps visually connect the drawer faces with the rest of the caddy,
	SetShaderMaterial("bluePlastic");

	// draw the bottom left drawer mesh
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	/***		   Bottom left drawer handle details    		  ***/
	/****************************************************************/
	// set the scale for the bottom left drawer details
	scaleXYZ = glm::vec3(0.5f, 0.1f, 0.15f);
	// set the XYZ rotation for the bottom left drawer details
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the bottom left drawer details
	positionXYZ = glm::vec3(-1.2f, 0.45f, -1.45f);

	// set transformations for the bottom left drawer details
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("darkPrinted");
	SetTextureUVScale(1.0f, 1.0f);

	// Set the material for the bottom left drawer handle details
	// to match the main body so it visually connects with the rest of the caddy,
	// while still adding some surface detail from the texture.
	SetShaderMaterial("mattePlastic");

	// draw the bottom left drawer handle details mesh
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	/***		   Bottom right drawer of the tool caddy		  ***/
	/****************************************************************/
	// set the scale for bottom right drawer
	scaleXYZ = glm::vec3(2.3f, 0.4f, 2.45f);
	// set the XYZ rotation for the bottom right drawer
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the bottom right drawer
	positionXYZ = glm::vec3(1.25f, 0.3f, -2.72f);
	
	

	// set transformations for the bottom right drawer
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Repeat the hex pattern across the drawer face so the texture
	// keeps a consistent scale instead of stretching across the mesh.
	SetShaderTexture("hexPattern");
	SetTextureUVScale(1.2f, 0.35f);

	// Set the material for the bottom right drawer to match the top tray pieces,
	// which helps visually connect the drawer faces with the rest of the caddy,
	// while still adding some surface detail from the texture.
	SetShaderMaterial("bluePlastic");

	// draw the bottom right drawer mesh
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	/***		   Bottom right drawer handle detail    		  ***/
	/****************************************************************/
	// set the scale for the bottom right drawer handle details
	scaleXYZ = glm::vec3(0.5f, 0.1f, 0.15f);
	// set the XYZ rotation for the bottom right drawer handle details
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the bottom right drawer handle details
	positionXYZ = glm::vec3(1.3f, 0.45f, -1.45f);

	// set transformations for the bottom right drawer handle details
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Use the dark printed texture and matte material so the handles
	// read as part of the same printed plastic body.
	SetShaderTexture("darkPrinted");
	SetTextureUVScale(1.0f, 1.0f);

	// Set the material for the bottom right drawer handle details
	SetShaderMaterial("mattePlastic");

	// draw the bottom right drawer details mesh
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	/***				Top drawer of tool caddy		   		  ***/
	/****************************************************************/
	// set the scale for the top drawer
	scaleXYZ = glm::vec3(4.8f, 0.4f, 2.45f);
	// set the XYZ rotation for the top drawer
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the top drawer
	positionXYZ = glm::vec3(0.0f, 0.8f, -2.72f);

	// set transformations for the top drawer
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// The top drawer uses a wider UV scale so the hex pattern repeats
	// across the long drawer face without looking stretched.
	SetShaderTexture("hexPattern");
	SetTextureUVScale(2.5f, 0.35f);

	// Set the material for the top drawer to match the top tray pieces,
	// which helps visually connect the drawer faces with the rest of the caddy,
	// while still adding some surface detail from the texture.
	SetShaderMaterial("bluePlastic");

	// draw the top drawer
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	/***				Top drawer handle detail    			  ***/
	/****************************************************************/
	// set the scale for the top drawer details
	scaleXYZ = glm::vec3(0.5f, 0.1f, 0.15f);
	// set the XYZ rotation for the drawer details
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the top drawer details
	positionXYZ = glm::vec3(0.0f, 0.95f, -1.45f);

	// set transformations for the top drawer handle details
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Use the dark printed texture and matte material so the handles
	// read as part of the same printed plastic body.
	SetShaderTexture("darkPrinted");
	SetTextureUVScale(1.0f, 1.0f);

	// Set the material for the top drawer handle details
	SetShaderMaterial("mattePlastic");

	// draw the top drawer handle details mesh
	m_basicMeshes->DrawBoxMesh();
}

/****************************************************************/
/***				Simplified 3D Printer					  ***/
/*** The object is based on a large enclosed Bambu Labs 3D    ***/
/*** printer from my reference image. I am simplifying it so  ***/
/*** it reads clearly without overcomplicating the scene.     ***/
/****************************************************************/
void SceneManager::Render3DPrinter() 
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*************************************************************/
	/***				Main Outer Printer Body				   ***/
	/*************************************************************/
	// Large aluminum textured light gray box for the printer. This is 
	// the main shape of the printer and gives it a clear structure in the scene, 
	// while still keeping the details simplified so it does not compete 
	// with the tool caddy for attention.
	scaleXYZ = glm::vec3(6.4f, 8.0f, 6.8f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-8.2f, 4.0f, -1.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Metalic light gray material for the printer frame.
	SetShaderMaterial("aluminum");

	// The texture didn't tile well, keeping it 1:1 for the larger piece
	// texture still gives that metal feel for the 3D printer body.
	SetShaderTexture("stainless");
	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawBoxMesh();

	/*************************************************************/
	/***              Front Glass Door / Window                ***/
	/*************************************************************/
	// Dark tinted front panel for the printer door. Since the main
	// printer body is simplified as a solid box, this front panel creates
	// the look of a glass door without needing transparency.
	scaleXYZ = glm::vec3(4.65f, 5.80f, 0.08f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-8.2f, 3.6f, 2.44f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Add the glass texture and dark glass material to simulate the
	// printer's front door. The texture provides the visual panel detail,
	// while the material controls how the surface responds to light.
	SetShaderTexture("glass");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("darkGlass");

	m_basicMeshes->DrawBoxMesh();

	/*************************************************************/
	/***                  Printer Door Handle                  ***/
	/*************************************************************/
	// Simple vertical handle on the front glass door. This adds an
	// exterior detail from the printer reference and makes the front
	// panel read more clearly as a door.
	scaleXYZ = glm::vec3(0.68f, 0.25f, 0.14f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-6.05f, 3.6f, 2.55f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderMaterial("aluminum");

	// Scaled down to align with body, while still standing out as 
	// an independent piece.
	SetShaderTexture("stainless");
	SetTextureUVScale(0.2f, 0.2f);

	m_basicMeshes->DrawBoxMesh();

	/*************************************************************/
	/***				Top Lid / Panel						   ***/
	/*************************************************************/
	// Dark glass top panel for the printer lid. It adds contrast to
	// the light printer frame and matches the dark glass areas
	// visible in the reference image.
	scaleXYZ = glm::vec3(5.90f, 0.12f, 6.25f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-8.2f, 8.0f, -1.00f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("glass");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("darkGlass");

	m_basicMeshes->DrawBoxMesh();

	/*************************************************************/
	/***				Touch Screen Glow					   ***/
	/*************************************************************/
	// Slightly larger panel behind the touch screen to simulate a soft
	// display, added since I couldn't get the glow right with my first attempt.
	scaleXYZ = glm::vec3(1.65f, 1.12f, 0.04f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-10.15f, 7.2f, 2.39f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.00f, 0.22f, 0.28f, 1.0f);
	SetShaderMaterial("screenGlow");

	m_basicMeshes->DrawBoxMesh();

	/*************************************************************/
	/***				Front Touch Screen					   ***/
	/*************************************************************/
	// Small touch screen on the front left side of the printer.
	// This add a recognizable printer detail and gives an ideal
	// place to add a small glow enhancement later.
	scaleXYZ = glm::vec3(1.35f, 0.82f, 0.08f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-10.15f, 7.2f, 2.43f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Blue-green screen color. This will look like a lit display,
	// and we can also add a small light source near it. 
	SetShaderColor(0.02f, 0.70f, 0.90f, 1.0f);
	SetShaderMaterial("screenGlow");

	m_basicMeshes->DrawBoxMesh();

	/*************************************************************/
	/***				Screen Inner Detail					   ***/
	/*************************************************************/
	// Smaller dark insert on the touch screen to make it look more 
	// like a display instead of just a colored rectangle.
	scaleXYZ = glm::vec3(0.95f, 0.48f, 0.09f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-10.15f, 7.2f, 2.47f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0f, 0.35f, 0.50f, 1.0f);
	SetShaderMaterial("screenGlow");
	
	m_basicMeshes->DrawBoxMesh();
}

/****************************************************************/
/***					Filament Spool						  ***/
/*** This object represents a roll of filament from the       ***/
/*** reference image. It is simplified with cylinders and     ***/
/*** small detail pieces so it reads clearly without adding   ***/
/*** too much complexity to the scene.						  ***/
/****************************************************************/
void SceneManager::RenderFilamentSpool() {
	// declare the variables for the tranformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*************************************************************/
	/***				Main filament roll					   ***/
	/*************************************************************/
	// Main light gray cylinder for the filament wrapped around 
	// the spool.
	scaleXYZ = glm::vec3(1.45f, 0.65f, 1.45f);
	XrotationDegrees = 00.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(8.0f, 0.00f, -1.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderMaterial("goldFilament");

	SetShaderTexture("filament");
	SetTextureUVScale(1.0f, 2.0f);

	

	m_basicMeshes->DrawCylinderMesh();

	/*************************************************************/
	/***					Top Spool Rim					   ***/
	/*************************************************************/
	// Light gray top rim of the spool. This wraps the filament within 
	// the spool.
	scaleXYZ = glm::vec3(1.65f, 0.03f, 1.65f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(8.0f, 0.0f, -1.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.78f, 0.78f, 0.78f, 1.0f);
	SetShaderMaterial("drywallSurface");

	m_basicMeshes->DrawCylinderMesh();

	/*************************************************************/
	/***				Bottom Spool Rim					   ***/
	/*************************************************************/
	// Light gray bottom rim of the spool. This wraps the filament within 
	// the spool.
	scaleXYZ = glm::vec3(1.65f, 0.03f, 1.65f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(8.0f, 0.65f, -1.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.78f, 0.78f, 0.78f, 1.0f);
	SetShaderMaterial("drywallSurface");

	m_basicMeshes->DrawCylinderMesh();

	/*************************************************************/
	/***				Center Spool Hole					   ***/
	/*************************************************************/
	// Smaller center hub to show the hole in the filament spool. 
	// The torus for this just looked a bit off so simplified to use
	// cylinders. The hole is not a true cutout, but the dark should give the
	// illusion of an opening in the spool. 
	scaleXYZ = glm::vec3(0.58f, 0.14f, 0.58f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(8.00f, 0.55f, -1.00f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.03f, 0.03f, 0.03f, 1.0f);
	SetShaderMaterial("mattePlastic");

	m_basicMeshes->DrawCylinderMesh();
}

/*************************************************************/
/***				Printed Spool Holder	    		   ***/
/*************************************************************/

void SceneManager::Render3DPrintedPart() {
	// declare the variables for the tranformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*************************************************************/
	/***				   Round tall torus	   			   	   ***/
	/*************************************************************/
	// Torus detail for the rounded section of the printed part.
	// This gives the object a curved opening similar to the red part
	// in the reference image.
	scaleXYZ = glm::vec3(0.35f, 0.35f, 2.4f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(4.30f, 0.45f, 1.25f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("cheddar");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("orangePlastic");

	m_basicMeshes->DrawTorusMesh();

	/*************************************************************/
	/***				Sloped printed base					  ***/
	/*************************************************************/
	// Angled prism for the main sloped base of the loose printed part.
	// I needed more of a tapered rectangle but combining the shapes worked
	// pretty well for this part. The rotation helps it sit at an angle
	// like the reference object.
	scaleXYZ = glm::vec3(0.805f, 0.07f, 1.5f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = -60.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(3.6f, 0.035f, 1.647f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("cheddar");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("orangePlastic");

	m_basicMeshes->DrawPrismMesh();

	/*************************************************************/
	/***				Rounded lower body					  ***/
	/*************************************************************/
	// Cylinder body under the torus to make the prism rounded on the ends
	// like the reference piece.
	scaleXYZ = glm::vec3(0.41f, 0.07f, 0.41f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(4.28f, 0.0f, 1.25f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("cheddar");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("orangePlastic");

	m_basicMeshes->DrawCylinderMesh();

	/*************************************************************/
	/***				Small vertical printed post			  ***/
	/*************************************************************/
	// Small vertical cylinder detail on the front of the printed part.
	scaleXYZ = glm::vec3(0.14f, 0.70f, 0.14f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(3.00f, -0.025f, 2.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("cheddar");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("orangePlastic");

	m_basicMeshes->DrawCylinderMesh();

	/*************************************************************/
	/***				 Top cap on printed post			   ***/
	/*************************************************************/
	// Small cap on top of the vertical post. This adds detail and makes
	// the post look more like an intentional printed feature.
	scaleXYZ = glm::vec3(0.18f, 0.02f, 0.18f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(3.0f, 0.66f, 2.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("cheddar");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("orangePlastic");

	m_basicMeshes->DrawCylinderMesh();

	/*************************************************************/
	/***				Small front base detail				  ***/
	/*************************************************************/
	// Small box detail to connect the post area back into the angled base.
	// Used to give the tapered look to the prism like in the reference image.
	scaleXYZ = glm::vec3(0.50f, 0.065f, 0.27f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(3.20f, 0.034f, 1.88f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("cheddar");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("orangePlastic");

	m_basicMeshes->DrawBoxMesh();
}
