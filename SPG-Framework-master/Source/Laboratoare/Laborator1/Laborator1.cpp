#include "Laborator1.h"


#include <vector>
#include <iostream>

#include <Core/Engine.h>

using namespace std;

// Order of function calling can be seen in "Source/Core/World.cpp::LoopUpdate()"
// https://github.com/UPB-Graphics/SPG-Framework/blob/master/Source/Core/World.cpp

Laborator1::Laborator1()
{
}

Laborator1::~Laborator1()
{
}

void Laborator1::Init()
{
	nrInstances = 0;
	maxInstances = 50;
	shrink = 0;
	
	auto camera = GetSceneCamera();
	//camera->SetPositionAndRotation(glm::vec3(5,5,5), glm::quat(glm::vec3(0)));
	//camera->SetRotation(glm::quat(0.906538, -0.205128, 0.35984, 0.0814233));
	camera->SetPositionAndRotation(glm::vec3(4.5, 2, 0), glm::quat(glm::vec3(0)));
	camera->SetRotation(glm::quat(0.696596, -0.040775, 0.71508, 0.0418567));
	camera->Update();

	
	switchToSuperpixels = 1;
	ssbo = new SSBO<Superpixel>(0, true);
	// Load a mesh from file into GPU memory
	
	{
		Mesh* mesh = new Mesh("cornellBox");
		mesh->LoadMesh(RESOURCE_PATH::MODELS + "CornellBox", "CornellBox_Empty_3sides.obj");
		meshes[mesh->GetMeshID()] = mesh;
	}
	{
		Mesh* mesh = new Mesh("lucy");
		mesh->LoadMesh(RESOURCE_PATH::MODELS + "Lucy", "Winged_Victory.obj");
		meshes[mesh->GetMeshID()] = mesh;
	}

	std::string shaderPath = "Source/Laboratoare/Laborator1/Shaders/";
	//std::string shaderPath = "Source/Laboratoare/Laborator3/Shaders/";
	// Create a shader program for rendering to texture
	{
		Shader *shader = new Shader("Instances");
		shader->AddShader(shaderPath + "VertexShader.glsl", GL_VERTEX_SHADER);
		//shader->AddShader(shaderPath + "GeometryShader.glsl", GL_GEOMETRY_SHADER);
		shader->AddShader(shaderPath + "FragmentShader.glsl", GL_FRAGMENT_SHADER);
		shader->CreateAndLink();
		shaders[shader->GetName()] = shader;
	}
	
	{
		shaderPath = "Source/Laboratoare/Laborator1/SLICShaders/";
		Shader* shader = new Shader("SLIC");
		shader->AddShader(shaderPath + "VertexShader.glsl", GL_VERTEX_SHADER);
		shader->AddShader(shaderPath + "FragmentShader.glsl", GL_FRAGMENT_SHADER);
		shader->CreateAndLink();
		shaders[shader->GetName()] = shader;
	}

	{
		shaderPath = "Source/Laboratoare/Laborator1/BlenderShaders/";
		Shader* shader = new Shader("Blender");
		shader->AddShader(shaderPath + "VertexShader.glsl", GL_VERTEX_SHADER);
		shader->AddShader(shaderPath + "FragmentShader.glsl", GL_FRAGMENT_SHADER);
		shader->CreateAndLink();
		shaders[shader->GetName()] = shader;
	}

	{
		shaderPath = "Source/Laboratoare/Laborator7/Shaders/";
		Shader* shader = new Shader("ImageProcessing");
		shader->AddShader(shaderPath + "VertexShader.glsl", GL_VERTEX_SHADER);
		shader->AddShader(shaderPath + "FragmentShader.glsl", GL_FRAGMENT_SHADER);
		shader->CreateAndLink();
		shaders[shader->GetName()] = shader;
	}

	DrawRSM();
	// Render from light pov and SLIC
	SLIC();
	RenderSegments();
	

}

void Laborator1::DrawRSM() {

	frameBuffer = new FrameBuffer();
	frameBuffer->Generate(1024, 1024, 3, true);
	
	

	auto shader = shaders["SLIC"];

	frameBuffer->Bind();

	frameBuffer->BindAllTextures();

	//render depth of scene to texture ( from light's perspective)
	float near_plane = 1.0f, far_plane = 7.5f;
	//glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
	glm::mat4 lightProjectionMatrix = glm::perspective(60.0f * 3.14f/180.0f, 1.0f, 0.2f, 10.0f);
	glm::vec3 lightPosition = glm::vec3(4);
	glm::mat4 lightViewMatrix = glm::lookAt(lightPosition,
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f));
	lightSpaceMatrix = lightProjectionMatrix * lightViewMatrix;
	shader->Use();
	//GLint lightSpaceMatrixLocation = glGetUniformLocation(simpleDepthShader->program, "lightSpaceMatrix");
	//glUniformMatrix4fv(lightSpaceMatrixLocation, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	//unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT2 };
	//glDrawBuffers(3, attachments);
	RenderScene(shader, lightViewMatrix, lightProjectionMatrix);


}

cv::Mat Laborator1::readImagePixels(int textureId) {
	frameBuffer->BindTexture(textureId, GL_TEXTURE_2D);
	cv::Mat pixels(SHADOW_HEIGHT, SHADOW_WIDTH, CV_32FC3);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, pixels.data);//
	if (textureId == 0) {

		cv::imwrite("xx.png", pixels*255);
	}
	cv::Mat cv_pixels(SHADOW_HEIGHT, SHADOW_WIDTH, CV_32FC3);
	for (int y = 0; y < SHADOW_HEIGHT; y++)
		for (int x = 0; x < SHADOW_WIDTH; x++)
		{
			cv_pixels.at<cv::Vec3f>(y, x)[2] = pixels.at<cv::Vec3f>(SHADOW_HEIGHT - y - 1, x)[0];//
			cv_pixels.at<cv::Vec3f>(y, x)[1] = pixels.at<cv::Vec3f>(SHADOW_HEIGHT - y - 1, x)[1];
			cv_pixels.at<cv::Vec3f>(y, x)[0] = pixels.at<cv::Vec3f>(SHADOW_HEIGHT - y - 1, x)[2];
		}

	return cv_pixels;
}

void Laborator1::SLIC() {
	

	cv::Mat image_color = readImagePixels(0);
	cv::Mat image_normals = readImagePixels(1);
	cv::Mat image_positions = readImagePixels(2);
	cv::Mat lab_image;

	cv::cvtColor(image_color, lab_image, CV_BGR2Lab);

		

	/* Yield the number of superpixels and weight-factors from the user. */
	int w = image_color.cols, h = image_color.rows;
	int nr_superpixels = 1000;
	int nc = 100;

	double step = sqrt((w * h) / (double)nr_superpixels);

	/* Perform the SLIC superpixel algorithm. */
	slic.generate_superpixels(lab_image, step, nc);
	slic.create_connectivity(lab_image);

	//slic.display_contours(image_normals, Vec3b(0, 0, 255));
	cv::imwrite("image-colors.jpg", image_color * 255);
	cv::imwrite("image-normals.jpg", image_normals * 255);
	cv::imwrite("image-positions.jpg", image_positions * 255);
	/* Display the contours and show the result. */
	//slic.colour_with_cluster_means(image_color);
	//slic.display_contours(image_color, Vec3b(0, 0, 255));
	//slic.display_center_grid(image, Vec3b(10, 30, 200));
	vector<Superpixel> temp=  slic.getSuperpixels(image_color, image_normals, image_positions);
	vector<Superpixel> superpixels_vector(temp.begin(), temp.end());
	superpixels = new Superpixel[slic.nr_superpixels]();
	superpixels = &superpixels_vector[0];
	//imshow("result", image);
	cv::imwrite("SLIC.jpg", image_color *255);
	cv::imwrite("SLICn.jpg", image_normals * 255);
	cv::imwrite("SLICp.jpg", image_positions * 255);
	
	uniformSuperpixels();
	//waitKey(0);
}


void Laborator1::FrameStart()
{

}

void Laborator1::uniformSuperpixels()
{

	//Superpixel* data = superpixels;
	//GLuint ssbo;
	//glGenBuffers(1, &ssbo);
	//glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	////Superpixel ssp = { glm::vec2(1,4) ,glm::vec4(25,25,25,0) ,glm::vec4(25,25,25,0) ,glm::vec4(25,25,25,0) , 20};
	////glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ssp), &ssp, GL_DYNAMIC_DRAW);
	//glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Superpixel) * slic.nr_superpixels, data, GL_DYNAMIC_COPY); 
	//
	////glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo);
	//glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind




	

	

	ssbo = new SSBO<Superpixel>(slic.nr_superpixels, true);
	//Superpixel* data = const_cast<Superpixel*>(ssbo->GetBuffer());
	//data = superpixels;

	ssbo->SetBufferData(superpixels);

	
}
/*
void Laborator1::uniformSuperpixels()
{
	auto shader = shaders["Instances"];


	unsigned int block_index = glGetUniformBlockIndex(shader->program, "superpixels");

	GLuint binding_point_index = 2;
	
	glUniformBlockBinding(shader->program, block_index, 0);


	GLuint ubo = 0;
	glGenBuffers(1, &ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, ubo);
	glBufferData(GL_UNIFORM_BUFFER, superpixels.colors.size() * 5 * sizeof(cv::Vec4f) , NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glBindBufferRange(GL_UNIFORM_BUFFER, 0, ubo, 0, superpixels.colors.size() * 5 * sizeof(cv::Vec4f));
	//glBindBufferBase(GL_UNIFORM_BUFFER, binding_point_index, ubo);

	int size = superpixels.colors.size() * sizeof(cv::Vec4f);
	//coord
	glBindBuffer(GL_UNIFORM_BUFFER, ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, size, &superpixels.coord);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	//colors
	glBindBuffer(GL_UNIFORM_BUFFER, ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, size, size, &superpixels.colors);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	//normals
	glBindBuffer(GL_UNIFORM_BUFFER, ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, size * 2, size, &superpixels.normals);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	//positions
	glBindBuffer(GL_UNIFORM_BUFFER, ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, size * 3, size, &superpixels.positions);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	//weight
	glBindBuffer(GL_UNIFORM_BUFFER, ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, size * 4, size, &superpixels.weight);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	
}

//
void Laborator1::uniformSuperpixels()
{
	auto shader = shaders["Instances"];

	GLuint ubo = 0;
	glGenBuffers(1, &ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, ubo);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(superpixels) * superpixels.coord.size(), &superpixels, GL_DYNAMIC_DRAW); //ignora size-ul ala initial ca nu stiam exact ce scriu 
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	//glBindBuffer(GL_UNIFORM_BUFFER, ubo);
	//GLvoid* p = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	//memcpy(p, &superpixels, sizeof(superpixels));
	//glUnmapBuffer(GL_UNIFORM_BUFFER);

	unsigned int block_index = glGetUniformBlockIndex(shader->program, "superpixels");

	GLuint binding_point_index = 2;
	glBindBufferBase(GL_UNIFORM_BUFFER, binding_point_index, ubo);

	glUniformBlockBinding(shader->program, block_index, binding_point_index);

}
*/

void Laborator1::RenderScene(Shader* shader, glm::mat4 viewMatrix, glm::mat4 projectionMatrix) {
	auto camera = GetSceneCamera();

	// render an object using the specified shader and the specified position
	shader->Use();

	ssbo->BindBuffer(0);

	glUniformMatrix4fv(shader->loc_view_matrix, 1, GL_FALSE, glm::value_ptr(viewMatrix));
	glUniformMatrix4fv(shader->loc_projection_matrix, 1, GL_FALSE, glm::value_ptr(projectionMatrix)); //lightProjection

	glm::vec3 lightPosition = glm::vec3(4);

	// Set light position uniform
	GLint locLightPos = glGetUniformLocation(shader->program, "light_position");
	glUniform3fv(locLightPos, 1, glm::value_ptr(lightPosition));

	// Set eye position (camera position) uniform
	glm::vec3 eyePosition = GetSceneCamera()->transform->GetWorldPosition();
	GLint locEyePos = glGetUniformLocation(shader->program, "eye_position");
	glUniform3fv(locEyePos, 1, glm::value_ptr(eyePosition));

	int materialShininess = 10;
	float materialKd = 1.0f;
	float materialKs = 1.0f;

	// Set material property uniforms (shininess, kd, ks, object color) 
	GLint locMaterial = glGetUniformLocation(shader->program, "material_shininess");
	glUniform1i(locMaterial, materialShininess);

	GLint locMaterialKd = glGetUniformLocation(shader->program, "material_kd");  // diffuse light
	glUniform1f(locMaterialKd, materialKd);

	GLint locMaterialKs = glGetUniformLocation(shader->program, "material_ks");  // specular light
	glUniform1f(locMaterialKs, materialKs);

	GLint superpixels_total = glGetUniformLocation(shader->program, "superpixels_total");
	glUniform1i(superpixels_total, slic.nr_superpixels);

	GLint locSuperpixels = glGetUniformLocation(shader->program, "switchToSuperpixels");
	glUniform1i(locSuperpixels, switchToSuperpixels);
		
	glActiveTexture(GL_TEXTURE5);
	frameBuffer->GetDepthTexture()->Bind();
	//glBindTexture(GL_TEXTURE_2D, frameBuffer->GetDepthTexture()->GetTextureID());
	GLint shadow = glGetUniformLocation(shader->program, "depthMap");  // depth map
	glUniform1i(shadow, 5);


	glActiveTexture(GL_TEXTURE6);
	frameBuffer->GetTexture(0)->Bind();
	GLint flux_texture = glGetUniformLocation(shader->program, "flux_texture");  // depth map
	glUniform1i(flux_texture, 6);

	glActiveTexture(GL_TEXTURE7);
	frameBuffer->GetTexture(1)->Bind();
	GLint normals_texture = glGetUniformLocation(shader->program, "normals_texture");  // depth map
	glUniform1i(normals_texture, 7);

	glActiveTexture(GL_TEXTURE8);
	frameBuffer->GetTexture(2)->Bind();
	GLint positions_texture = glGetUniformLocation(shader->program, "positions_texture");  // depth map
	glUniform1i(positions_texture, 8);


	GLint lightSM = glGetUniformLocation(shader->program, "lightSpaceMatrix");
	glUniformMatrix4fv(lightSM, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

	glm::mat4 model(1);
	model = glm::translate(model, glm::vec3(0.0f));
	model = glm::scale(model, glm::vec3(0.007f));
	model = glm::rotate(model, 45.0f, glm::vec3(0, 1, 0));

	glUniformMatrix4fv(shader->loc_model_matrix, 1, GL_FALSE, glm::value_ptr(model));
	meshes["lucy"]->Render(shader);

	model = glm::mat4(1);
	model = glm::translate(model, glm::vec3(0.0f));
	model = glm::scale(model, glm::vec3(1.4f));
	glUniformMatrix4fv(shader->loc_model_matrix, 1, GL_FALSE, glm::value_ptr(model));
	meshes["cornellBox"]->Render(shader);
}

cv::Mat Laborator1::readAmbientLight() {
	/*frameBufferAmbient->BindTexture(0, GL_TEXTURE_2D);
	cv::Mat pixels(1024, 1024, CV_32FC3);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, pixels.data);
	
	cv::Mat cv_pixels(1, 1, CV_32FC3);

	cv_pixels.at<cv::Vec3f>(0, 0)[2] = pixels.at<cv::Vec3f>(0, 0)[0];
	cv_pixels.at<cv::Vec3f>(0, 0)[1] = pixels.at<cv::Vec3f>(0, 0)[1];
	cv_pixels.at<cv::Vec3f>(0, 0)[0] = pixels.at<cv::Vec3f>(0, 0)[2];


	return cv_pixels;*/

	frameBufferAmbient->BindTexture(0, GL_TEXTURE_2D);
	cv::Mat pixels(window->props.resolution.y, window->props.resolution.x, CV_32FC3);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, pixels.data);//
	//glReadPixels(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT, GL_RGB, GL_FLOAT, pixels.data);

	cv::Mat cv_pixels(window->props.resolution.y, window->props.resolution.x, CV_32FC3);
	for (int y = 0; y < window->props.resolution.y; y++)
		for (int x = 0; x < window->props.resolution.x; x++)
		{
			cv_pixels.at<cv::Vec3f>(y, x)[2] = pixels.at<cv::Vec3f>(window->props.resolution.y - y - 1, x)[0];//
			cv_pixels.at<cv::Vec3f>(y, x)[1] = pixels.at<cv::Vec3f>(window->props.resolution.y - y - 1, x)[1];
			cv_pixels.at<cv::Vec3f>(y, x)[0] = pixels.at<cv::Vec3f>(window->props.resolution.y - y - 1, x)[2];
		}

	return cv_pixels;
}

void Laborator1::RenderSegments() 
{

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	frameBufferAmbient = new FrameBuffer();
	frameBufferAmbient->Generate(window->props.resolution.x, window->props.resolution.y, 1, false);

	Vec3f ambient = Vec3f(0.0f);

	auto camera = GetSceneCamera();
	auto shader = shaders["Blender"];

	frameBufferAmbient->Bind();
	cv::Mat image;

	shader->Use();
	

	for (int i = 0; i <  16 ; i++) { // 1024 / 32
		for (int j = 0; j < 16; j++) {
			GLint row = glGetUniformLocation(shader->program, "row");
			glUniform1i(row, i);

			GLint column = glGetUniformLocation(shader->program, "column");
			glUniform1i(column, j);
			cout << "row"<<i<<"col"<<j << endl;

			RenderScene(shader, camera->GetViewMatrix(), camera->GetProjectionMatrix());
			glFinish();
		}
	}
	image = readAmbientLight();
	String name = "final.jpg";
	cv::imwrite(name, image * 255);

	// Al doilea frameBuffer 
	{

		auto shader = shaders["ImageProcessing"];
		shader->Use();

		glDisable(GL_BLEND);
		frameBufferQuad = new FrameBuffer();
		frameBufferQuad->Generate(window->props.resolution.x, window->props.resolution.y, 1, false);

		frameBufferQuad->Bind();
		frameBufferAmbient->BindTexture(0, GL_TEXTURE9);
		//frameBufferAmbient->BindTexture(0, GL_TEXTURE_2D);

		int locTexture = shader->GetUniformLocation("textureImage");
		glUniform1i(locTexture, 9);

		RenderMesh(meshes["quad"], shader, glm::mat4(1));

		//citire textura si salvare ca jpg
		frameBufferQuad->BindTexture(0, GL_TEXTURE_2D);
		cv::Mat pixels(window->props.resolution.y, window->props.resolution.x, CV_32FC3);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, pixels.data);

		cv::Mat cv_pixels(window->props.resolution.y, window->props.resolution.x, CV_32FC3);
		for (int y = 0; y < window->props.resolution.y; y++)
			for (int x = 0; x < window->props.resolution.x; x++)
			{
				cv_pixels.at<cv::Vec3f>(y, x)[2] = pixels.at<cv::Vec3f>(window->props.resolution.y - y - 1, x)[0];//
				cv_pixels.at<cv::Vec3f>(y, x)[1] = pixels.at<cv::Vec3f>(window->props.resolution.y - y - 1, x)[1];
				cv_pixels.at<cv::Vec3f>(y, x)[0] = pixels.at<cv::Vec3f>(window->props.resolution.y - y - 1, x)[2];
			}

		cv::imwrite("quad.jpg", cv_pixels * 255);


	}

}

void Laborator1::notBlack(cv::Mat& image) {
	Vec3f rgb;
	for (int i = 0; i < image.cols; i++) {
		for (int j = 0; j < image.rows; j++) {
			rgb=image.at<Vec3f>(j, i);
			if (rgb != Vec3f(0.0)) {
				//cout << rgb<<endl;
			}
		}
	}
}

void Laborator1::Update(float deltaTimeSeconds)
{
	{
		auto shader = shaders["Instances"];
		
		auto camera = GetSceneCamera();
		//cout << camera->GetViewMatrix() <<endl;
		glDisable(GL_BLEND);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		RenderScene(shader, camera->GetViewMatrix(), camera->GetProjectionMatrix());


	}
}

//oid Laborator1::RenderCompleteRSM() {
	//setez fb
	//blending settings
	// poate alt shader?
	// setari contor..
	//
//}
void Laborator1::FrameEnd()
{
	//DrawCoordinatSystem();
}

// Read the documentation of the following functions in: "Source/Core/Window/InputController.h" or
// https://github.com/UPB-Graphics/SPG-Framework/blob/master/Source/Core/Window/InputController.h

void Laborator1::OnInputUpdate(float deltaTime, int mods)
{
	// treat continuous update based on input with window->KeyHold()

	//TODO add update for modifying the shrink parameter
};

void Laborator1::OnKeyPress(int key, int mods)
{
	// add key press event

	if (key == GLFW_KEY_EQUAL)
	{
		nrInstances++;
		nrInstances %= maxInstances;
	}


	if (key == GLFW_KEY_MINUS)
	{
		nrInstances--;
		nrInstances %= maxInstances;
	}

	if (key == GLFW_KEY_G)
	{
		switchToSuperpixels = (switchToSuperpixels + 1) % 2;
	}

	if (key == GLFW_KEY_1)
	{
		switchToSuperpixels = 1;
	}

	if (key == GLFW_KEY_2)
	{
		switchToSuperpixels = 2;
	}

	if (key == GLFW_KEY_3)
	{
		switchToSuperpixels = 3;
	}

	if (key == GLFW_KEY_4)
	{
		switchToSuperpixels = 4;
	}
};

void Laborator1::OnKeyRelease(int key, int mods)
{
	// add key release event
};

void Laborator1::OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY)
{
	// add mouse move event
};

void Laborator1::OnMouseBtnPress(int mouseX, int mouseY, int button, int mods)
{
	// add mouse button press event
};

void Laborator1::OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods)
{
	// add mouse button release event
}

void Laborator1::OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY)
{
	// treat mouse scroll event
}

void Laborator1::OnWindowResize(int width, int height)
{
	// treat window resize event
}
