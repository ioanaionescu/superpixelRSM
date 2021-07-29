#pragma once

#include <Component/SimpleScene.h>
#include <opencv2/core/mat.hpp>
#include "../Laborator3/slic.h"
#include <Core/GPU/SSBO.h>

class Laborator1 : public SimpleScene
{
	public:
		Laborator1();
		~Laborator1();

		void Init() override;

		void SLIC();

		void DrawRSM();

		cv::Mat readImagePixels(int textureId);

		

	private:
		void FrameStart() override;
		void uniformSuperpixels();
		void InitFBO();
		void DrawScene(Shader* shader);
		void DrawScene();
		void RenderScene(Shader* shader, glm::mat4 viewMatrix, glm::mat4 projectionMatrix);
		void Update1(float deltaTimeSeconds);
		void RenderScene();
		void Update(float deltaTimeSeconds) override;
		void FrameEnd() override;

		void OnInputUpdate(float deltaTime, int mods) override;
		void OnKeyPress(int key, int mods) override;
		void OnKeyRelease(int key, int mods) override;
		void OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY) override;
		void OnMouseBtnPress(int mouseX, int mouseY, int button, int mods) override;
		void OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods) override;
		void OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY) override;
		void OnWindowResize(int width, int height) override;

	private:
		unsigned int nrInstances;
		unsigned int maxInstances;
		float shrink;
		FrameBuffer* frameBuffer;
		unsigned int depthMapFBO;
		const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024, SCR_WIDTH = 1280, SCR_HEIGHT = 720;
		unsigned int depthMap;
		int firstFrame = 0;
		int switchToSuperpixels;
		Slic slic;
		Superpixel* superpixels;
		glm::mat4 lightSpaceMatrix;
		SSBO<Superpixel>* ssbo;
		vector<Superpixel> superpixels_vector;
		//GLuint ssbo;

};
