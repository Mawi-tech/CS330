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
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
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
		glGenTextures(1, &m_textureIDs[i].ID);
	}
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

	return(true);
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

void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL samsungLabel;
	samsungLabel.ambientColor = glm::vec3(1.0f, 1.0f, 1.0f);
	samsungLabel.ambientStrength = 0.4f;
	samsungLabel.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	samsungLabel.specularColor = glm::vec3(0.01f, 0.01f, 0.01f);
	samsungLabel.shininess = 1.0f;
	samsungLabel.tag = "samsungLabel";
	m_objectMaterials.push_back(samsungLabel);

	OBJECT_MATERIAL blackPlastic;
	blackPlastic.ambientColor = glm::vec3(1.0f, 1.0f, 1.0f);
	blackPlastic.ambientStrength = 0.65f;
	blackPlastic.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	blackPlastic.specularColor = glm::vec3(0.02f, 0.02f, 0.02f);
	blackPlastic.shininess = 2.0f;
	blackPlastic.tag = "blackPlastic";
	m_objectMaterials.push_back(blackPlastic);

	OBJECT_MATERIAL deskMaterial;
	deskMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	deskMaterial.ambientStrength = 0.05f;
	deskMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f);
	deskMaterial.specularColor = glm::vec3(0.02f, 0.02f, 0.02f);
	deskMaterial.shininess = 32.0f;
	deskMaterial.tag = "deskMaterial";
	m_objectMaterials.push_back(deskMaterial);

	OBJECT_MATERIAL screenMaterial;
	screenMaterial.ambientColor = glm::vec3(0.05f, 0.05f, 0.05f);
	screenMaterial.ambientStrength = 0.05f;
	screenMaterial.diffuseColor = glm::vec3(0.05f, 0.05f, 0.05f);
	screenMaterial.specularColor = glm::vec3(0.9f, 0.9f, 0.95f);
	screenMaterial.shininess = 130.0f;
	screenMaterial.tag = "screenMaterial";
	m_objectMaterials.push_back(screenMaterial);

	OBJECT_MATERIAL speakerMaterial;
	speakerMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	speakerMaterial.ambientStrength = 0.3f;
	speakerMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
	speakerMaterial.specularColor = glm::vec3(0.05f, 0.05f, 0.05f);
	speakerMaterial.shininess = 5.0f;
	speakerMaterial.tag = "speakerMaterial";
	m_objectMaterials.push_back(speakerMaterial);
}


/***Set up Scene Lights****/

void SceneManager::SetupSceneLights()
{
	//m_pShaderManager->setBoolValue(g_UseLightingName, true);

	/*** STUDENTS - add the code BELOW for setting up light sources ***/
	/*** Up to four light sources can be defined. Refer to the code ***/
	/*** in the OpenGL Sample for help                              ***/

	//Ceiling light source 
	m_pShaderManager->setVec3Value("lightSources[0].position",0.0f, 15.0f, 2.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 1.0f, 0.95f, 0.9f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 0.001f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.02f);

	//Front Fill light source
	m_pShaderManager->setVec3Value("lightSources[1].position", 0.0f, 10.0f, -20.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.05f, 0.05f, 0.25f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.10f, 0.1f, 0.15f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", .05f, 0.05f, 0.05f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 0.01f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.02f);

	//Background Area Fill light source
	m_pShaderManager->setVec3Value("lightSources[2].position", 0.0f, 5.0f, 10.0f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 0.01f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.1f);

	//Monitor Screen Light
	m_pShaderManager->setVec3Value("lightSources[3].position", 0.0f, 7.5f, 3.0f);
	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", 0.30f, 0.30f, 0.5f);
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.5f); 

	m_pShaderManager->setBoolValue("bUseLighting", true);
}
//LoadSceneTextures is used for preparing 3D scene by loading shapes, textures
// in memory to support the 3D scene rendering

void SceneManager::LoadSceneTextures()
{
	// load the textures for the 3D scene
	bool bReturn = false;
	bReturn = CreateGLTexture("../../Utilities/textures/carbonFiber5.jpg", "deskTexture");
	bReturn = CreateGLTexture("../../Utilities/textures/blackPlastic3.jpg", "monitorStand");
	bReturn = CreateGLTexture("../../Utilities/textures/SamsungLogo.jpg", "samsungLogo");
	bReturn = CreateGLTexture("../../Utilities/textures/monitorBorder.jpg", "monitorBorder");
	bReturn = CreateGLTexture("../../Utilities/textures/TVOff.jpg", "monitorScreen");
	bReturn = CreateGLTexture("../../Utilities/textures/woodWall.jpg", "wallTexture");
	bReturn = CreateGLTexture("../../Utilities/textures/speakerTexture.jpg", "speakerTexture");
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
	DefineObjectMaterials();
	SetupSceneLights();
	//load textures for the 3D scene
	LoadSceneTextures();
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadSphereMesh();	
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

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/

	/******************************************************************/
	/*Backwall*/
	/******************************************************************/
	scaleXYZ = glm::vec3(20.0f, 10.0f, 10.0f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 5.0f, -3.0f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f);
	SetShaderTexture("wallTexture");
	SetShaderMaterial("deskMaterial");
	SetTextureUVScale(4.0f, 1.0f);
	m_basicMeshes->DrawPlaneMesh();
	
	/******************************************************************/
	/*Desk*/
	/******************************************************************/
	//Desk/bottom of scene
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 5.0f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.5f, 0.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderTexture("deskTexture");
	SetShaderMaterial("deskMaterial");
	SetTextureUVScale(12.0f, 12.0f);
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	//Shelf for desk
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 0.5f, 4.0f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 3.7f, 0.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderTexture("deskTexture");
	SetShaderMaterial("deskMaterial");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	//Left Desk shelf support
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 3.0f, 1.0f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-8.0f, 0.5f, 0.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderTexture("deskTexture");
	SetShaderMaterial("deskMaterial");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//Right Desk shelf support
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 3.0f, 1.0f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(8.0f, 0.5f, 0.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderTexture("deskTexture");
	SetShaderMaterial("deskMaterial");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	SetTextureUVScale(1.0f, 1.0f);

	/******************************************************************/
	/*Border around monitor*/
	/******************************************************************/
	/*Samsung Label*/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 0.5f, 0.3f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 5.5f, 0.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(.4f, .4f, .4f, 1);
	SetShaderMaterial("samsungLabel");
	//set the texture for the mesh
	SetShaderTexture("samsungLogo");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(8.0f, 4.5f, 0.3f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 7.4f, -0.5f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(.05, .05, .05, 1);
	SetShaderMaterial("blackPlastic");
	//set the texture for the mesh
	SetShaderTexture("monitorBorder");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/
	/*Moniter
	/******************************************************************/
	/*Moniter Screen*/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(6.0f, 3.3f, 0.3f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 7.4f, -0.35f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	// Turn off lighting so the screen renders as self-illuminated (glowing)
	m_pShaderManager->setBoolValue(g_UseLightingName, false);
	SetShaderColor(0.35f, 0.65f, 1.0f, 1.0f); 
	m_basicMeshes->DrawBoxMesh();
	// Re-enable lighting for everything else in the scene
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	/*Verticle monitor stand*/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.8f, 0.2f, 0.5f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = -60.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 4.5f, -1.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(.1f, .1f, .1f, 1.0f);
	SetShaderMaterial("blackPlastic");
	//set the texture for the mesh
	SetShaderTexture("monitorStand");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/*Left monitor stand*/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.0f, 0.2f, 0.2f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 40.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.5f, 4.0f, 0.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(0.1f, 0.1f, 0.1f, 0.1f);
	SetShaderMaterial("blackPlastic");
	//set the texture for the mesh
	SetShaderTexture("monitorStand");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	/*Right monitor stand*/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.0f, 0.2f, 0.2f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -40.0f;
	ZrotationDegrees = 0.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.5f, 4.0f, 0.0f);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(.1f, .1f, .1f, 1.0f);
	SetShaderMaterial("blackPlastic");
	//set the texture for the mesh
	SetShaderTexture("monitorStand");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	

	/******************************************************************/
	/*Speakers*/
	/******************************************************************/

	/*Left Speaker*/
	//Speaker Body
	scaleXYZ = glm::vec3(1.5f, 3.0f, 1.2f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-5.0f, 5.4f, 0.0f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderMaterial("speakerMaterial");
	SetShaderTexture("speakerTexture");
	m_basicMeshes->DrawBoxMesh();

	//Speaker Volume Knob
	scaleXYZ = glm::vec3(0.15f, 0.1f, 0.15f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-5.40f, 4.5f, 0.6f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(0.4f, 0.4f, 0.4f, 1.0f);
	SetShaderMaterial("blackPlastic");
	m_basicMeshes->DrawSphereMesh();

	//Speaker Bass Knob 
	scaleXYZ = glm::vec3(0.15f, 0.1f, 0.15f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-4.55f, 4.5f, 0.6f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(0.4f, 0.4f, 0.4f, 1.0f);
	SetShaderMaterial("blackPlastic");
	m_basicMeshes->DrawSphereMesh();


	/*Right Speaker*/
	//Speaker Body
	scaleXYZ = glm::vec3(1.5f, 3.0f, 1.2f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(5.0f, 5.4f, 0.0f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderMaterial("speakerMaterial");
	SetShaderTexture("speakerTexture");
	m_basicMeshes->DrawBoxMesh();

	//Speaker Volume Knob
	scaleXYZ = glm::vec3(0.15f, 0.1f, 0.15f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(5.40f, 4.5f, 0.6f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(0.4f, 0.4f, 0.4f, 1.0f);
	SetShaderMaterial("blackPlastic");
	m_basicMeshes->DrawSphereMesh();

	//Speaker Bass Knob 
	scaleXYZ = glm::vec3(0.15f, 0.1f, 0.15f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(4.55f, 4.5f, 0.6f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderColor(0.4f, 0.4f, 0.4f, 1.0f);
	SetShaderMaterial("blackPlastic");
	m_basicMeshes->DrawSphereMesh();


}
