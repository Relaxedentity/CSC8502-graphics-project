#pragma once

#include "../nclgl/OGLRenderer.h"
#include "../nclgl/SceneNode.h"
#include "../nclgl/Frustum.h"

class HeightMap;
class Camera;
class Light;
class Shader;
class MeshMaterial;
class MeshAnimation;

class Renderer : public OGLRenderer {
public:
	Renderer(Window& parent);
	~Renderer(void);
	bool gaussian;
	bool splitscreen;
	bool cameramove;
	void RenderScene() override;
	void RenderScenePP();
	void UpdateScene(float dt) override;
	float posx;
	float posy;
	float posz;
	int rotation;
	bool rotate;
	float c1;
	float c2;
	float camrotate;
	bool returning;
	bool returningcam;

protected:
	void BuildNodeLists(SceneNode* from);
	void SortNodeLists();
	void ClearNodeLists();
	void DrawNodes();
	void DrawNode(SceneNode* n);
	void PresentScene();
	void DrawPostProcess();
	void DrawScene();
	void DrawSkybox();
	void DrawWater();
	void DrawAnim();
	void FillBuffers();
	void DrawPointLights();
	void CombineBuffers();
	void GenerateScreenTexture(GLuint& into, bool depth = false);
	void DrawShadowScene();
	
	SceneNode* root;
	HeightMap* heightMap;
	int dir;
	Shader* shader;
	Shader* nodeshader;
	Shader* processShader;
	Shader* skyboxShader;
	Shader* reflectShader;
	Shader* submeshshader;
	Shader* animmeshshader;
	Shader* sceneShader;
	Shader* pointlightShader;
	Shader* combineShader;
	Shader* shadowsceneShader;
	Shader* shadowShader;

	Camera* camera;
	Camera* camera2;
	Light* light; 
	Light* pointLights;
	GLuint texture;
	MeshMaterial* treematerial;
	MeshMaterial* manmaterial;
	MeshAnimation* mananim;
	vector <GLuint > treetextures;
	vector <GLuint > mantextures;
	GLuint treetexture;
	GLuint bumpmap;
	GLuint cubeMap;
	GLuint waterTex;

	float waterRotate;
	float waterCycle;

	Mesh* tree;
	Mesh* quad;
	Mesh* manmesh;
	Mesh* sphere;
	Mesh* var;
	Mesh* var2;
	//SubMesh* sub;

	Frustum frameFrustum;

	GLuint bufferFBO;
	GLuint bufferFBO2;
	GLuint processFBO;
	GLuint PPbufferFBO;
	GLuint processFBO2;
	GLuint PPbufferFBO2;
	GLuint bufferColourTex;
	GLuint bufferColourTex2;
	GLuint processbufferColourTex[2];
	GLuint processbufferColourTex2[2];
	GLuint bufferDepthTex;
	GLuint bufferDepthTex2;
	GLuint PPbufferDepthTex;
	GLuint PPbufferDepthTex2;
	GLuint bufferNormalTex;
	GLuint bufferNormalTex2;
	GLuint shadowTex;
	GLuint shadowFBO;

	GLuint pointLightFBO;
	GLuint pointLightFBO2;
	GLuint lightDiffuseTex;
	GLuint lightDiffuseTex2;
	GLuint lightSpecularTex;
	GLuint lightSpecularTex2;

	vector <SceneNode*> transparentNodeList;
	vector <SceneNode*> nodeList;
	vector <Mesh*> sceneMeshes;
	vector <Matrix4 > sceneTransforms;

	int currentFrame;
	float frameTime;
};
