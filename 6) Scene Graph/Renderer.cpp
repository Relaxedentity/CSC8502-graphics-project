#include "Renderer.h"

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	cube = Mesh::LoadFromMeshFile("OffsetCubeY.msh");
	camera = new Camera();

	shader = new Shader("SceneVertex.glsl", "SceneFragment.glsl");

	if (!shader->LoadSuccess()) {
		return;
	}

	projMatrix = Matrix4::Perspective(1.0f, 10000.0f, (float)width / (float)height, 45.0f);

	camera->SetPosition(Vector3(0, 30, 175));

	root = new SceneNode();
	//root->SetModelScale(Vector3(20, 30,10));
	root->AddChild(new CubeRobot(cube));
	

	//root2 = new SceneNode();
	//root2 ->AddChild(new CubeRobot(cube));
	//root2->SetTransform(Matrix4::Translation(Vector3(0, 100, 0)));
	

	glEnable(GL_DEPTH_TEST);
	init = true;
}

Renderer ::~Renderer(void) {
	delete root;
	//delete root2;
	delete shader;
	delete camera;
	delete cube;
}

void Renderer::UpdateScene(float dt) {
	camera->UpdateCamera(dt);
	viewMatrix = camera->BuildViewMatrix();
	root->Update(dt);
	//root2->Update(dt);
}

void Renderer::RenderScene() {
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	BindShader(shader);
	UpdateShaderMatrices();

	glUniform1i(glGetUniformLocation(shader->GetProgram(), "diffuseTex"), 1);

	DrawNode(root);
	//DrawNode(root2);
}

void Renderer::DrawNode(SceneNode* n) {
	if (n->GetMesh()) {
		Matrix4 model = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());
		glUniformMatrix4fv(glGetUniformLocation(shader->GetProgram(), "modelMatrix"), 1, false, model.values);

		glUniform4fv(glGetUniformLocation(shader->GetProgram(), "nodeColour"), 1, (float*)& n->GetColour());

		glUniform1i(glGetUniformLocation(shader->GetProgram(), "useTexture"), 0); //Next tutorial ;)
		n->Draw(*this);
	}

	for (vector <SceneNode*>::const_iterator
		i = n->GetChildIteratorStart(); i != n->GetChildIteratorEnd(); ++i) {
		DrawNode(*i);
	}
}

