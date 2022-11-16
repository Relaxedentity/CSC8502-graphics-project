#pragma once

#include "../nclgl/OGLRenderer.h"
#include "../2) Matrix Transformations/Camera.h"
#include "SceneNode.h"
#include "CubeRobot.h"

class Renderer : public OGLRenderer {
public:
	Renderer(Window& parent);
	~Renderer(void);

	void UpdateScene(float dt) override;
	void RenderScene() override;

protected:
	void DrawNode(SceneNode* n);

	SceneNode* root;
	//SceneNode* root2;
	Camera* camera;
	Mesh* cube;
	Shader* shader;
};

