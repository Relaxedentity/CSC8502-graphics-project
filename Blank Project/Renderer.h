#pragma once

#include "../nclgl/OGLRenderer.h"
#include "../6) Scene Graph/SceneNode.h"
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
	void RenderScene() override;
	void RenderScenePP();
	void UpdateScene(float dt) override;

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
	//SubMesh* sub;

	Frustum frameFrustum;

	GLuint bufferFBO;
	GLuint processFBO;
	GLuint PPbufferFBO;
	GLuint bufferColourTex;
	GLuint processbufferColourTex[2];
	GLuint bufferDepthTex;
	GLuint PPbufferDepthTex;
	GLuint bufferNormalTex;
	GLuint shadowTex;
	GLuint shadowFBO;

	GLuint pointLightFBO;
	GLuint lightDiffuseTex;
	GLuint lightSpecularTex;

	vector <SceneNode*> transparentNodeList;
	vector <SceneNode*> nodeList;
	vector <Mesh*> sceneMeshes;
	vector <Matrix4 > sceneTransforms;

	int currentFrame;
	float frameTime;
};
