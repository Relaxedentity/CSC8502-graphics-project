#include "Renderer.h"
#include "../nclgl/Light.h"
#include "../2) Matrix Transformations/Camera.h"
#include "../nclgl/HeightMap.h"
#include <algorithm >
#include <ctime>
#include "../nclgl/MeshMaterial.h"
#include "../nclgl/MeshAnimation.h"

const int LIGHT_NUM = 60;
#define SHADOWSIZE 2048
const int POST_PASSES = 5;

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {

	sphere = Mesh::LoadFromMeshFile("Sphere.msh");
	quad = Mesh::GenerateQuad();
	heightMap = new HeightMap(TEXTUREDIR"heightmaptest3.jpg");
	gaussian = false;
	tree = Mesh::LoadFromMeshFile("tree9_3.msh");
	manmesh = Mesh::LoadFromMeshFile("Role_T.msh");

	treematerial = new MeshMaterial("tree9_3.mat");
	manmaterial = new MeshMaterial("Role_T.mat");
	mananim = new MeshAnimation("Role_T.anm");

	texture = SOIL_load_OGL_texture(
		TEXTUREDIR"Barren Reds.JPG", SOIL_LOAD_AUTO,
		SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	bumpmap = SOIL_load_OGL_texture(
		TEXTUREDIR"Barren RedsDOT3.JPG", SOIL_LOAD_AUTO,
		SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);


	treetexture = SOIL_load_OGL_texture(
	TEXTUREDIR"stainedglass.tga", SOIL_LOAD_AUTO, 
	SOIL_CREATE_NEW_ID, 0);

	cubeMap = SOIL_load_OGL_cubemap(
	TEXTUREDIR"rusted_west.jpg", TEXTUREDIR"rusted_east.jpg",
	TEXTUREDIR"rusted_up.jpg", TEXTUREDIR"rusted_down.jpg",
	TEXTUREDIR"rusted_south.jpg", TEXTUREDIR"rusted_north.jpg",
	SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	waterTex = SOIL_load_OGL_texture(
	TEXTUREDIR"water.TGA", SOIL_LOAD_AUTO,
	SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	SetTextureRepeating(texture, true);
	SetTextureRepeating(bumpmap, true);
	SetTextureRepeating(treetexture, true);
	SetTextureRepeating(waterTex, true);

	Vector3 heightmapSize = heightMap->GetHeightmapSize();
	light = new Light(Vector3(0.5f, 1000.0f, 0.5f),
		Vector4(1, 1, 1, 1), heightmapSize.x);
	camera = new Camera(-45.0f, 0.0f,
		heightmapSize * Vector3(0.5f, 5.0f, 0.5f));

	pointLights = new Light[LIGHT_NUM];

	for (int i = 0; i < LIGHT_NUM; ++i) {
		Light& l = pointLights[i];
		l.SetPosition(Vector3(rand() % (int)heightmapSize.x,
			150.0f, rand() % (int)heightmapSize.z));

		l.SetColour(Vector4(0.5f + (float)(rand() / (float)RAND_MAX),
			0.5f + (float)(rand() / (float)RAND_MAX),
			0.5f + (float)(rand() / (float)RAND_MAX), 1));

		l.SetRadius(500.0f + (rand() % 250));
	}
	shader = new Shader("BumpVertex.glsl", "BumpFragment.glsl");
	nodeshader = new Shader("SceneVertex.glsl", "SceneFragment.glsl");
	processShader = new Shader("BumpVertex.glsl", "processfrag.glsl");
	skyboxShader = new Shader("skyboxVertex.glsl", "skyboxFragment.glsl");
	reflectShader = new Shader("reflectVertex.glsl", "reflectFragment.glsl");
	submeshshader = new Shader("texturedVertex.glsl", "texturedFragment.glsl");
	animmeshshader = new Shader("SkinningVertex.glsl", "texturedFragment.glsl");
	sceneShader = new Shader("BumpVertex.glsl", "bufferFragment.glsl");
	pointlightShader = new Shader("pointlightvert.glsl", "pointlightfrag.glsl");
	combineShader = new Shader("combinevert.glsl", "combinefrag.glsl");
	shadowsceneShader = new Shader("shadowscenevert.glsl", "shadowscenefrag.glsl");
	shadowShader = new Shader("shadowVert.glsl", "shadowFrag.glsl");

	if (!sceneShader->LoadSuccess() || !pointlightShader->LoadSuccess() ||
		!combineShader->LoadSuccess()) {
		return;
	}
	for (int i = 0; i < tree->GetSubMeshCount(); ++i) {
	const MeshMaterialEntry* matEntry =
		treematerial->GetMaterialForLayer(i);

	const string* filename = nullptr;
	matEntry->GetEntry("Diffuse", &filename);
	string path = TEXTUREDIR + *filename;
	GLuint texID = SOIL_load_OGL_texture(path.c_str(), SOIL_LOAD_AUTO,
		SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y);
	treetextures.emplace_back(texID);
	}

	for (int i = 0; i < manmesh->GetSubMeshCount(); ++i) {
	const MeshMaterialEntry* matEntry =
		manmaterial->GetMaterialForLayer(i);

	const string* filename = nullptr;
	matEntry->GetEntry("Diffuse", &filename);
	string path = TEXTUREDIR + *filename;
	GLuint texID = SOIL_load_OGL_texture(path.c_str(), SOIL_LOAD_AUTO,
		SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y);
	mantextures.emplace_back(texID);
	}
	//shadow buffer generation
	glGenTextures(1, &shadowTex);
	glBindTexture(GL_TEXTURE_2D, shadowTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		SHADOWSIZE, SHADOWSIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &shadowFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
		GL_TEXTURE_2D, shadowTex, 0);

	glDrawBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	sceneMeshes.emplace_back(heightMap);
	sceneTransforms.emplace_back(Matrix4());
	root = new SceneNode();
	srand((unsigned)time(0));
	for (int i = 0; i < 20; ++i) {
	SceneNode* s = new SceneNode();
	s->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	Matrix4 trans = Matrix4::Translation(
		Vector3((rand() % 3000), -540.0f, (rand() % -3000) + 100.0f + 500));
	s->SetTransform(trans);
	s->SetModelScale(Vector3(10.0f, 10.0f, 10.0f));
	s->SetBoundingRadius(100.0f);
	s->SetMesh(tree);
	sceneMeshes.emplace_back(tree);
	sceneTransforms.emplace_back(trans);
	for (int i = 0; i < tree->GetSubMeshCount(); ++i) {
		SceneNode* ss = new SceneNode();
		ss->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
		s->AddChild(ss);
		s->SetTexture(treetextures[i]);
	}
	root->AddChild(s);
}
root->Update(0);

	//G buffer generation
	glGenFramebuffers(1, &bufferFBO);
	glGenFramebuffers(1, &pointLightFBO);

	GLenum buffers[2] = {
		GL_COLOR_ATTACHMENT0 ,
		GL_COLOR_ATTACHMENT1
	};

	GenerateScreenTexture(bufferDepthTex, true);
	GenerateScreenTexture(bufferColourTex);
	GenerateScreenTexture(bufferNormalTex);
	GenerateScreenTexture(lightDiffuseTex);
	GenerateScreenTexture(lightSpecularTex);

	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, bufferColourTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
		GL_TEXTURE_2D, bufferNormalTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
		GL_TEXTURE_2D, bufferDepthTex, 0);
	glDrawBuffers(2, buffers);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
		GL_FRAMEBUFFER_COMPLETE) {
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, pointLightFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, lightDiffuseTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
		GL_TEXTURE_2D, lightSpecularTex, 0);
	glDrawBuffers(2, buffers);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
		GL_FRAMEBUFFER_COMPLETE) {
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glGenTextures(1, &PPbufferDepthTex);
	glBindTexture(GL_TEXTURE_2D, PPbufferDepthTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height,
		0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
	
	for (int i = 0; i < 2; ++i) {
		glGenTextures(1, &processbufferColourTex[i]);
		glBindTexture(GL_TEXTURE_2D, processbufferColourTex[i]);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	}
	
	glGenFramebuffers(1, &PPbufferFBO); //We’ll render the scene into this
	glGenFramebuffers(1, &processFBO);//And do post processing in this
	
	glBindFramebuffer(GL_FRAMEBUFFER, PPbufferFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
		GL_TEXTURE_2D, PPbufferDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
		GL_TEXTURE_2D, PPbufferDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, processbufferColourTex[0], 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
		GL_FRAMEBUFFER_COMPLETE || !PPbufferDepthTex || !processbufferColourTex[0]) {
		return;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_DEPTH_TEST);
	//glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	currentFrame = 0;
	frameTime = 0.0f;
	waterRotate = 0.0f;
	waterCycle = 0.0f;
	init = true;
}

Renderer ::~Renderer(void) {
	delete sceneShader;
	delete combineShader;
	delete pointlightShader;
	glDeleteTextures(1, &shadowTex);
	glDeleteFramebuffers(1, &shadowFBO);
	//for (auto& i : sceneMeshes) {
	//	delete i;
	//}
	delete heightMap;
	delete camera;
	delete sphere;
	delete quad;
	delete[] pointLights;
	glDeleteTextures(1, &bufferColourTex);
	glDeleteTextures(1, &bufferNormalTex);
	glDeleteTextures(1, &bufferDepthTex);
	glDeleteTextures(1, &lightDiffuseTex);
	glDeleteTextures(1, &lightSpecularTex);

	glDeleteFramebuffers(1, &bufferFBO);
	glDeleteFramebuffers(1, &pointLightFBO);
}

void Renderer::UpdateScene(float dt) {
	waterRotate += dt * 2.0f;
	waterCycle += dt * 0.25f;
	frameTime -= dt;

	while (frameTime < 0.0f) {
		currentFrame = (currentFrame + 1) % mananim->GetFrameCount();
		frameTime += 1.0f / mananim->GetFrameRate();
	}
	if (Window::GetKeyboard()->KeyDown(KEYBOARD_G)) {
		if (!gaussian) {
			gaussian = true;
		}
		else {
			gaussian = false;
		}
	}
	camera->UpdateCamera(dt);
	//viewMatrix = camera->BuildViewMatrix();
	//frameFrustum.FromMatrix(projMatrix * viewMatrix);
	
}

void Renderer::FillBuffers() {
	
	DrawShadowScene();
	BindShader(shadowsceneShader);
	SetShaderLight(*light);
	viewMatrix = camera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f,
		(float)width / (float)height, 45.0f);
	
	glUniform1i(glGetUniformLocation(shadowsceneShader->GetProgram(),
		"diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(shadowsceneShader->GetProgram(),
		"bumpTex"), 1);
	glUniform1i(glGetUniformLocation(shadowsceneShader->GetProgram(),
		"shadowTex"), 2);
	glUniform3fv(glGetUniformLocation(shadowsceneShader->GetProgram(),
		"cameraPos"), 1, (float*)& camera->GetPosition());

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, bumpmap);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, shadowTex);

	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	BindShader(sceneShader);
	glUniform1i(
		glGetUniformLocation(sceneShader->GetProgram(), "diffuseTex"), 0);
	glUniform1i(
		glGetUniformLocation(sceneShader->GetProgram(), "bumpTex"), 1);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, bumpmap);
	
	modelMatrix.ToIdentity();
	viewMatrix = camera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 10000.0f,
		(float)width / (float)height, 45.0f);

	Vector3 heightmapSize = heightMap->GetHeightmapSize();
	//viewMatrix = Matrix4::BuildViewMatrix(
		//light->GetPosition(), heightmapSize * 0.5f);
	//projMatrix = Matrix4::Perspective(1, 1500, 1, 45);

	UpdateShaderMatrices();

	heightMap->Draw();
	BuildNodeLists(root);
	SortNodeLists();
	BindShader(nodeshader);
	UpdateShaderMatrices();
	DrawNodes();
	ClearNodeLists();
	DrawAnim();
	DrawWater();
	//DrawAnim();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::GenerateScreenTexture(GLuint& into, bool depth) {
	glGenTextures(1, &into);
	glBindTexture(GL_TEXTURE_2D, into);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	GLuint format = depth ? GL_DEPTH_COMPONENT24 : GL_RGBA8;
	GLuint type = depth ? GL_DEPTH_COMPONENT : GL_RGBA;

	glTexImage2D(GL_TEXTURE_2D, 0,
		format, width, height, 0, type, GL_UNSIGNED_BYTE, NULL);

	glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::DrawPointLights() {
	glBindFramebuffer(GL_FRAMEBUFFER, pointLightFBO);
	BindShader(pointlightShader);

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glBlendFunc(GL_ONE, GL_ONE);
	glCullFace(GL_FRONT);
	glDepthFunc(GL_ALWAYS);
	glDepthMask(GL_FALSE);

	glUniform1i(glGetUniformLocation(
		pointlightShader->GetProgram(), "depthTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferDepthTex);

	glUniform1i(glGetUniformLocation(
		pointlightShader->GetProgram(), "normTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, bufferNormalTex);

	glUniform3fv(glGetUniformLocation(pointlightShader->GetProgram(),
		"cameraPos"), 1, (float*)& camera->GetPosition());

	glUniform2f(glGetUniformLocation(pointlightShader->GetProgram(),
		"pixelSize"), 1.0f / width, 1.0f / height);

	Matrix4 invViewProj = (projMatrix * viewMatrix).Inverse();
	glUniformMatrix4fv(glGetUniformLocation(
		pointlightShader->GetProgram(), "inverseProjView"),
		1, false, invViewProj.values);

	UpdateShaderMatrices();
	for (int i = 0; i < LIGHT_NUM; ++i) {
		Light& l = pointLights[i];
		SetShaderLight(l);
		sphere->Draw();
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glCullFace(GL_BACK);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);

	glClearColor(0.2f, 0.2f, 0.2f, 1);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::CombineBuffers() {
	BindShader(combineShader);
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();

	glUniform1i(glGetUniformLocation(
		combineShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferColourTex);

	glUniform1i(glGetUniformLocation(
		combineShader->GetProgram(), "diffuseLight"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, lightDiffuseTex);

	glUniform1i(glGetUniformLocation(
		combineShader->GetProgram(), "specularLight"), 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, lightSpecularTex);
	//DrawPostProcess();
	//PresentScene();
	var = quad;
	//quad->Draw();
}

void Renderer::RenderScene() {
	if (!gaussian) {
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		FillBuffers();
		DrawPointLights();
		CombineBuffers();
		var->Draw();
	}
	else {
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		FillBuffers();
		DrawPointLights();
		CombineBuffers();
		DrawScene();
		DrawPostProcess();
		PresentScene();
	}

}
//void Renderer::RenderScenePP() {
//	DrawScene();
//	DrawPostProcess();
//	PresentScene();
//}
void Renderer::DrawShadowScene() {
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);

	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, SHADOWSIZE, SHADOWSIZE);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	BindShader(shadowShader);
	Vector3 heightmapSize = heightMap->GetHeightmapSize();
	viewMatrix = Matrix4::BuildViewMatrix(
	light->GetPosition(), heightmapSize *0.5f);

	projMatrix = Matrix4::Perspective(1, 3000, 1, 45);
	shadowMatrix = projMatrix * viewMatrix; //used later

	for (int i = 0; i < 4; ++i) {
		modelMatrix = sceneTransforms[i];
		UpdateShaderMatrices();
		sceneMeshes[i]->Draw();
	}
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glViewport(0, 0, width, height);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::BuildNodeLists(SceneNode* from) {
		Vector3 dir = from->GetWorldTransform().GetPositionVector() - camera->GetPosition();
		from->SetCameraDistance(Vector3::Dot(dir, dir));
		if (from->GetColour().w < 1.0f) {
			transparentNodeList.push_back(from);
		}
		else {
			nodeList.push_back(from);
		}
		for (vector <SceneNode*>::const_iterator i =
			from->GetChildIteratorStart();
			i != from->GetChildIteratorEnd(); ++i) {
			BuildNodeLists((*i));
		}
	
}

void Renderer::SortNodeLists() {
	std::sort(transparentNodeList.rbegin(),
		transparentNodeList.rend(), 
		SceneNode::CompareByCameraDistance);

	std::sort(nodeList.begin(), nodeList.end(),
		SceneNode::CompareByCameraDistance);
}

void Renderer::DrawNodes() {
	for (const auto& i : nodeList) {
		DrawNode(i);
	}
	for (const auto& i : transparentNodeList) {
		DrawNode(i);
	}
}

void Renderer::DrawAnim() {
	BindShader(animmeshshader);
	glUniform1i(glGetUniformLocation(animmeshshader->GetProgram(),
		"diffuseTex"), 0);
	Vector3 var = Vector3(0.2f, 0.8f, 0.2f);
	Vector3 hSize = heightMap->GetHeightmapSize();
	Matrix4 model = Matrix4::Translation(hSize * var)*
		Matrix4::Scale(Vector3(100.0f, 100.0f, 100.0f))*
		Matrix4::Rotation(0, Vector3(1, 1, 1));
	UpdateNodeShaderMatrices();
	glUniformMatrix4fv(glGetUniformLocation(animmeshshader->GetProgram(),
		"modelMatrix"), 1, false, model.values);
	vector <Matrix4 > frameMatrices;

	const Matrix4* invBindPose = manmesh->GetInverseBindPose();
	const Matrix4* frameData = mananim->GetJointData(currentFrame);
	for (unsigned int i = 0; i < manmesh->GetJointCount(); ++i) {
		frameMatrices.emplace_back(frameData[i] * invBindPose[i]);
	}

	int j = glGetUniformLocation(animmeshshader->GetProgram(), "joints");
	glUniformMatrix4fv(j, frameMatrices.size(), false, (float*)frameMatrices.data());

	for (int i = 0; i < manmesh->GetSubMeshCount(); ++i) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mantextures[i]);
		manmesh->DrawSubMesh(i);
	}
}

void Renderer::DrawNode(SceneNode* n) {
	if (n->GetMesh()) {

		BindShader(nodeshader);
		glUniform1i(glGetUniformLocation(nodeshader->GetProgram(),
			"diffuseTex"), 0);
		//UpdateShaderMatrices();
		glUniform4fv(glGetUniformLocation(nodeshader->GetProgram(),
			"nodeColour"), 1, (float*)& n->GetColour());

		for (int i = 0; i < tree->GetSubMeshCount(); ++i) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, treetextures[i]);
			glUniform1i(glGetUniformLocation(nodeshader->GetProgram(),
				"useTexture"), treetextures[i]);
			Matrix4 model = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());
			glUniformMatrix4fv(glGetUniformLocation(nodeshader->GetProgram(),
				"modelMatrix"), 1, false, model.values);

			UpdateNodeShaderMatrices();
			tree->DrawSubMesh(i);
		}
	}
}
void Renderer::ClearNodeLists() {
	transparentNodeList.clear();
	nodeList.clear();
}

void Renderer::DrawScene() {


	glBindFramebuffer(GL_FRAMEBUFFER, PPbufferFBO);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	var->Draw();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::DrawPostProcess() {
	glBindFramebuffer(GL_FRAMEBUFFER, processFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, processbufferColourTex[1], 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	BindShader(processShader);
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();

	glDisable(GL_DEPTH_TEST);

	glActiveTexture(GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(
		processShader->GetProgram(), "sceneTex"), 0);
	for (int i = 0; i < POST_PASSES; ++i) {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_2D, processbufferColourTex[1], 0);
		glUniform1i(glGetUniformLocation(processShader->GetProgram(),
			"isVertical"), 0);
		glBindTexture(GL_TEXTURE_2D, processbufferColourTex[0]);
		quad->Draw();

		//Now to swap the colour buffers , and do the second blur pass
		glUniform1i(glGetUniformLocation(processShader->GetProgram(),
			"isVertical"), 1);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_2D, processbufferColourTex[0], 0);
		glBindTexture(GL_TEXTURE_2D, processbufferColourTex[1]);
		quad->Draw();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_DEPTH_TEST);
}
void Renderer::PresentScene() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	BindShader(submeshshader);
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	textureMatrix.ToIdentity();
	UpdateShaderMatrices();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, processbufferColourTex[0]);
	glUniform1i(glGetUniformLocation(
		submeshshader->GetProgram(), "diffuseTex"), 0);

	quad->Draw();
}

void Renderer::DrawSkybox() {
	glDepthMask(GL_FALSE);

	BindShader(skyboxShader);
	UpdateShaderMatrices();

	quad->Draw();

	glDepthMask(GL_TRUE);
}
void Renderer::DrawWater() {
	BindShader(reflectShader);
	glUniform3fv(glGetUniformLocation(reflectShader->GetProgram(),
		"cameraPos"), 1, (float*)& camera->GetPosition());

	glUniform1i(glGetUniformLocation(
		reflectShader->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(
		reflectShader->GetProgram(), "cubeTex"), 2);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, waterTex);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);

	Vector3 hSize = heightMap->GetHeightmapSize();
	Vector3 var = Vector3(0.1f,0.1f,0.1f);
	modelMatrix =
		Matrix4::Translation(hSize * var) *
		Matrix4::Scale(hSize * 0.2f) *
		Matrix4::Rotation(90, Vector3(1, 0, 0));

	textureMatrix =
		Matrix4::Translation(Vector3(waterCycle, 0.0f, waterCycle)) *
		Matrix4::Scale(Vector3(10, 10, 10)) *
		Matrix4::Rotation(waterRotate, Vector3(0, 0, 1));

	UpdateShaderMatrices();
	quad->Draw();
}