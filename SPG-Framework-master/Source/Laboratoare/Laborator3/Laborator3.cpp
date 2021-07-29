#include "Laborator3.h"

#include <vector>
#include <iostream>

#include <Core/Engine.h>
#include "slic.h"
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc/types_c.h>
using namespace std;

static float angle = 0;
static int poi = 0;

// Order of function calling can be seen in "Source/Core/World.cpp::LoopUpdate()"
// https://github.com/UPB-Graphics/SPG-Framework/blob/master/Source/Core/World.cpp

Laborator3::Laborator3()
{
}

Laborator3::~Laborator3()
{
}

void Laborator3::Init()
{


	/* Load the image and convert to Lab colour space. */
	//IplImage* image = cvLoadImage("C:\Users\ioana.ionescu98\Downloads\SLIC-Superpixels-master\SLIC-Superpixels-master\dog.png", 1);
	//std::string image_path = "C:\\Users\\ioana.ionescu98\\Downloads\\SLIC-Superpixels-master\\SLIC-Superpixels-master\\dog2.png";
	//cv::Mat img = cv::imread(image_path, cv::IMREAD_COLOR);
	////IplImage* lab_image = cvCloneImage(image);
	//cv::cvtColor(img, img, CV_BGR2Lab);


	//cv::Mat image = cv::imread(image_path, 1);
	//cv::Mat lab_image;
	//cv::cvtColor(image, lab_image, CV_BGR2Lab);

	///* Yield the number of superpixels and weight-factors from the user. */
	//int w = image.cols, h = image.rows;
	//int nr_superpixels = 400;
	//int nc = 40;

	//double step = sqrt((w * h) / (double)nr_superpixels);

	///* Perform the SLIC superpixel algorithm. */
	//Slic slic;
	//slic.generate_superpixels(lab_image, step, nc);
	//slic.create_connectivity(lab_image);

	///* Display the contours and show the result. */
	//slic.display_contours(image, Vec3b(0, 0, 255));
	//imshow("result", image);
	//waitKey(0);



	auto camera = GetSceneCamera();
	camera->SetPositionAndRotation(glm::vec3(0, 5, 4), glm::quat(glm::vec3(-30 * TO_RADIANS, 0, 0)));
	camera->Update();

	// Load a mesh from file into GPU memory
	{
		Mesh* mesh = new Mesh("bamboo");
		mesh->LoadMesh(RESOURCE_PATH::MODELS + "Vegetation/Bamboo", "bamboo.obj");
		meshes[mesh->GetMeshID()] = mesh;
	}

	{
		Mesh* mesh = new Mesh("quad");
		mesh->LoadMesh(RESOURCE_PATH::MODELS + "Primitives", "quad.obj");
		mesh->UseMaterials(false);
		meshes[mesh->GetMeshID()] = mesh;
	}

	std::string shaderPath = "Source/Laboratoare/Laborator3/Shaders/";

	// Create a shader program for rendering to texture
	{
		Shader *shader = new Shader("ShaderLab3");
		shader->AddShader(shaderPath + "VertexShader.glsl", GL_VERTEX_SHADER);
		shader->AddShader(shaderPath + "FragmentShader.glsl", GL_FRAGMENT_SHADER);
		shader->CreateAndLink();
		shaders[shader->GetName()] = shader;
	}

	auto resolution = window->GetResolution();


	//TODO - create a new framebuffer and generate attached textures
	frameBuffer = new FrameBuffer();
	frameBuffer->Generate(1024, 1024, 1, true);
}

void Laborator3::FrameStart()
{

}

void Laborator3::Update(float deltaTimeSeconds)
{
	static glm::vec3 mirrorPos(0, 4, -6);
	static glm::vec3 mirrorRotation = glm::vec3(0, RADIANS(180), 0);

	angle += 0.5f * deltaTimeSeconds;

	ClearScreen();
	
	// Save camera position and rotation
	auto camera = GetSceneCamera();

	glm::vec3 camPosition = camera->transform->GetWorldPosition();
	glm::quat camRotation = camera->transform->GetWorldRotation();

	//TODO - Render scene view from the mirror point of view
	// Use camera->SetPosition() and camera->SetRotation(glm::quat(euler_angles)) 
	{
		camera->SetPosition(glm::normalize(camPosition - mirrorPos));
		camera->SetRotation(glm::reflect(glm::normalize(camPosition), glm::vec3(0, 0, 1)));
		frameBuffer->Bind();
		DrawScene();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	// Render the scene normaly
	{
		camera->SetPosition(camPosition);
		camera->SetRotation(camRotation);

		DrawScene();
	}

	//TODO - Render the mirror view
	{
		auto shader = shaders["ShaderLab3"];
		// TODO - Use mirror texture
		frameBuffer->BindTexture(0, GL_TEXTURE0);
		//glUniform1i(shader->GetUniformLocation("texture_1"), frameBuffer->GetTextureID(0));

		glm::mat4 modelMatrix(1);
		modelMatrix = glm::translate(modelMatrix, mirrorPos);
		modelMatrix = glm::scale(modelMatrix, glm::vec3(4));
		RenderMesh(meshes["quad"], shader, modelMatrix);

	}

	std::string image_path = "C:\\Users\\ioana.ionescu98\\Downloads\\SLIC-Superpixels-master\\SLIC-Superpixels-master\\dog2.png";
	std::string image_path2 = "C:\\Users\\ioana.ionescu98\\Downloads\\SLIC-Superpixels-master\\SLIC-Superpixels-master\\fish.png";
	cv::Mat img = cv::imread(image_path2, cv::IMREAD_COLOR);
	//IplImage* lab_image = cvCloneImage(image);
	//cv::cvtColor(img, img, CV_BGR2Lab);

	Mat image(2048, 2592, CV_8UC1, &frameBuffer[0]);
	//cv::Mat image = cv::imread(image_path, 1);


	cv::Mat pixels(720, 1280,  CV_8UC3);
	glReadPixels(0, 0, 1280, 720, GL_RGB, GL_UNSIGNED_BYTE, pixels.data);
	cv::Mat cv_pixels(720, 1280,  CV_8UC3);
	for (int y = 0; y < 720; y++)
		for (int x = 0; x < 1280; x++)
	{
		cv_pixels.at<cv::Vec3b>(y, x)[2] = pixels.at<cv::Vec3b>(720 - y - 1, x)[0];
		cv_pixels.at<cv::Vec3b>(y, x)[1] = pixels.at<cv::Vec3b>(720 - y - 1, x)[1];
		cv_pixels.at<cv::Vec3b>(y, x)[0] = pixels.at<cv::Vec3b>(720 - y - 1, x)[2];
	}
	
	image = cv_pixels;
	//image = img;
	cv::Mat lab_image;

	cv::cvtColor(image, lab_image, CV_BGR2Lab);

	

	/* Yield the number of superpixels and weight-factors from the user. */
	int w = image.cols, h = image.rows;
	int nr_superpixels = 400;
	int nc = 40;

	double step = sqrt((w * h) / (double)nr_superpixels);

	/* Perform the SLIC superpixel algorithm. */
	Slic slic;
	slic.generate_superpixels(lab_image, step, nc);
	slic.create_connectivity(lab_image);

	/* Display the contours and show the result. */
	slic.colour_with_cluster_means(image);
	//slic.display_contours(image, Vec3b(0, 0, 255));
	//slic.display_center_grid(image, Vec3b(10, 30, 200));
	slic.grid_color(image);
	imshow("result", image);
	waitKey(0);

}

void Laborator3::DrawScene()
{
	for (int i = 0; i < 16; i++)
	{
		float rotateAngle = (angle + i) * ((i % 2) * 2 - 1);
		glm::vec3 position = glm::vec3(-4 + (i % 4) * 2.5, 0, -2 + (i / 4) * 2.5);

		glm::mat4 modelMatrix = glm::translate(glm::mat4(1), position);
		modelMatrix = glm::rotate(modelMatrix, rotateAngle, glm::vec3(0, 1, 0));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(0.1f));

		RenderMesh(meshes["bamboo"], shaders["ShaderLab3"], modelMatrix);
	}
}

void Laborator3::FrameEnd()
{
	DrawCoordinatSystem();
}

// Read the documentation of the following functions in: "Source/Core/Window/InputController.h" or
// https://github.com/UPB-Graphics/SPG-Framework/blob/master/Source/Core/Window/InputController.h

void Laborator3::OnInputUpdate(float deltaTime, int mods)
{
	// treat continuous update based on input
};

void Laborator3::OnKeyPress(int key, int mods)
{
	// add key press event
};

void Laborator3::OnKeyRelease(int key, int mods)
{
	// add key release event
};

void Laborator3::OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY)
{
	// add mouse move event
};

void Laborator3::OnMouseBtnPress(int mouseX, int mouseY, int button, int mods)
{
	// add mouse button press event
};

void Laborator3::OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods)
{
	// add mouse button release event
}

void Laborator3::OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY)
{
	// treat mouse scroll event
}

void Laborator3::OnWindowResize(int width, int height)
{
	// treat window resize event
}
