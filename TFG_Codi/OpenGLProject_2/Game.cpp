#include "Game.h"



//Private functions

//Init configurations of GLSL
void Game::initGLFW()
{
	//Init glfw
	if (glfwInit() == GLFW_FALSE)
	{
		std::cout << "ERROR::GLFW_INIT_FAILED" << "\n";
		glfwTerminate();
	}
}

void Game::initWindow(
	const char* title,
	bool resizable
)
{
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//FOR THE VERSION X.Y MAJOR = X, MINOR = Y
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, this->GL_VERSION_MAJOR);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, this->GL_VERSION_MINOR);
	glfwWindowHint(GLFW_RESIZABLE, resizable);

	this->window = glfwCreateWindow(this->WINDOW_WIDTH, this->WINDOW_HEIGHT, title, NULL, NULL);

	if (this->window == nullptr) 
	{
		std::cout << "ERROR::GLFW_WINDOW_INIT_FAILED" << "\n";
		glfwTerminate();
	}

	//Solo lo necesitamos si no queremos hacer el resize
	glfwGetFramebufferSize(this->window, &this->framebufferWidth, &this->framebufferHeight);
	//glViewport(0, 0, framebufferWidth, framebufferHeight);
	//En case de resize
	glfwSetFramebufferSizeCallback(this->window, framebuffer_resize_callback);

	glfwMakeContextCurrent(this->window); //IMPORTANT!!

}

void Game::initGLEW()
{
	//INIT GLEW (NEEDS WINDOW AND OOPENGL CONTEXT)
	//GLEW make what functions are available and right can use it.
	glewExperimental = GL_TRUE;

	//check error
	if (glewInit() != GLEW_OK)
	{
		std::cout << "ERROR::GLEW_INIT_FAILED" << "\n";
		glfwTerminate();
	}
}

void Game::initOpenGLOptions()
{
	//opengl functions les he de mirar segons em vagin be a mi.
	//open gl is a state machine, anything we enable would be enable.

	//Makes possible z coordinate
	glEnable(GL_DEPTH_TEST);

	//texture3d
	//glEnable(GL_TEXTURE_3D);
	//PFNGLTEXIMAGE3DPROC glTexImage3D;
	//glTexImage3D = (PFNGLTEXIMAGE3DPROC) glfwGetProcAddress("glTexImage3D");

	//
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	//Most important, blending of colors. If you want to enable settings for this blend, the next
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); //GL_LINE TODO EN LINEAS, GL_FILL TODO RELLENO, ETC.

	//Input cursor
	glfwSetInputMode(this->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}


//First inizialitzation of the Matrices
void Game::initMatrices()
{

	this->ViewMatrix = glm::mat4(1.f);
	this->ViewMatrix = glm::lookAt(this->camPosition, this->camPosition + this->camFront, this->worldUp);

	//Dependemos del framebuffer.
	this->ProjectionMatrix = glm::perspective(
		glm::radians(this->fov),
		static_cast<float>(this->framebufferWidth) / this->framebufferHeight,
		this->nearPlane,
		this->farPlane
	);
}

//First inicialization of the Shaders
void Game::initShaders()
{
	//SHADER INIT
	this->shaders.push_back(
		new Shader(
			this->GL_VERSION_MAJOR,
			this->GL_VERSION_MINOR,
			"vertex_core.glsl",
			"fragment_core_Octree_Newton_multi.glsl"));
	this->shaders.push_back(
		new Shader(
		this->GL_VERSION_MAJOR, 
		this->GL_VERSION_MINOR, 
			"vertex_core.glsl", 
			"fragment_core_Voxel_DDA_Newton.glsl"));
	
	
}

void Game::initPointCloud()
{
	this->pointClouds.push_back(new MultiPointCloud(1, "OBJFiles/1_0_0.ply"));
}

void Game::initMaterials()
{
	this->materials.push_back(new Material(
		glm::vec3(1.f), glm::vec3(1.f,1.f,1.f), glm::vec3(1.f),
		1.f,
		1.f,
		0, 
		1));
}

void Game::initViewPort()
{
	
	
	this->viewPort = new Mesh(
		new Quad(),
		this->materials[MAT_0], //Mandar 3 materiales distintos.
		this->pointClouds[0],
		this->camera.getPosition() - *this->camera.getFront() * 2.f,
		glm::vec3(0.f),
		glm::vec3(0.f),
		glm::vec3(1.f)
	);

	

	
}

void Game::initPointLights()
{
	glm::vec3 center = this->pointClouds[0]->getCenter();
	this->pointLights.push_back(new PointLight(
		center + glm::vec3(100.f, 0.f, 0.f), 
		0.5f,
		glm::vec3(1.f, 1.f, 1.f)
	));
	this->pointLights.push_back(new PointLight(
		center + glm::vec3(0.f, 100.f, 0.f), 
		0.5f,
		glm::vec3(1.f, 1.f, 1.f)
	));
	this->pointLights.push_back(new PointLight(
		center + glm::vec3(0.f, 0.f, 100.f), 
		0.5f,
		glm::vec3(1.f, 1.f, 1.f)
	));
}

void Game::initLights()
{	
	this->globalLight = glm::vec3(0.01, 0.01, 0.01);
	this->initPointLights();
}

void Game::updateRandom()
{
	srand(time(nullptr));

	// Genera dos números aleatorios entre 0 y 1
	float random1 = static_cast<float>(rand()) / RAND_MAX;
	float random2 = static_cast<float>(rand()) / RAND_MAX;

	this->randomSeed = glm::vec2(random1, random2);
}

void Game::initUniforms()
{
	this->uEpsilon = 0.0001;
	this->uMaxRayDistance = 999.9f;
	this->updateRandom();
	this->shaders[this->currentShader]->setVec2f(this->randomSeed, "randomSeed");
	this->shaders[this->currentShader]->set1f(this->uEpsilon, "uEpsilon");
	this->shaders[this->currentShader]->set1f(this->uMaxRayDistance, "uMaxRayDistance");
	this->pointClouds[this->currentPointCloud]->initPlanes(this->shaders[this->currentShader]);

	this->shaders[this->currentShader]->setVec3f(this->globalLight, "globalLight");
	this->shaders[this->currentShader]->setMat4fv(this->ViewMatrix, "ViewMatrix");
	this->shaders[this->currentShader]->setMat4fv(this->ProjectionMatrix, "ProjectionMatrix");
	this->shaders[this->currentShader]->setVec2f(glm::vec2(this->framebufferWidth,this->framebufferHeight), "window_size");

	for (int i = 0; i < this->pointLights.size(); i++) {
				this->pointLights[i]->sendToShader(*this->shaders[this->currentShader],i);
	}
}

void Game::initImGui()
{

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");

	this->octreeModeSelected = true;
	this->voxelModeSelected = false;
}

void Game::renderImGui()
{
	// ImGui NewFrame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// Create ImGui windows
	static int maxPoints = 2;
	static int nOctrees = 3;
	static int totalSize = 150;
	static float scale = 0.5;
	static int maxDepth = 7;
	static bool childContainingAllPoints = false;
	static bool segmented = true;
	static bool error = true;
	static bool shadow = false;
	static bool errorShadow = true;
	static bool shading = true;
	static bool extra = true;
	static bool interpolation = true;
	static bool order = true;
	static bool showPlanes = true;

	if (ImGui::Begin("Configuración", nullptr, ImGuiWindowFlags_NoCollapse)) {
		if (ImGui::CollapsingHeader("PointCloud")) {
			// Content of sub-section 1
			ImGui::InputText("File URL", fileURL, IM_ARRAYSIZE(fileURL));

			if (ImGui::Button("Seleccionar archivo")) {
				const char* filterPatterns[1] = { "*.ply" };
				const char* fileName = tinyfd_openFileDialog(
					"Selecciona un archivo",  // Título de la ventana
					"",  // Ruta inicial (vacía para usar la ruta por defecto)
					1,  // Número de patrones de filtro
					filterPatterns,  // Patrones de filtro
					"Image Files",  // Descripción de los patrones de filtro
					0  // 0 para permitir múltiple selección, 1 para permitir solo una selección
				);

				if (fileName != NULL) {
					strncpy_s(fileURL, fileName, sizeof(fileURL));
					fileURL[sizeof(fileURL) - 1] = '\0';  // Asegurar que la cadena termine con '\0'
				}
			}
			if (ImGui::Button("ArchivoActual")) {
				std::string filePath = this->pointClouds[this->currentPointCloud]->GetFilePath();
				const char* charFilePath = filePath.c_str();
				strcpy_s(this->fileURL, sizeof(this->fileURL), charFilePath);
			}
			
			if (ImGui::CollapsingHeader("MultiOctree Options"))
			{

				ImGui::InputInt("Numero de Octrees (1-3)", &nOctrees);
				ImGui::InputInt("Máximo de puntos", &maxPoints);
				ImGui::InputInt("Máximo de profundidad", &maxDepth);
				ImGui::Checkbox("Child containing all points", &childContainingAllPoints);
				ImGui::InputInt("TotalSize", &totalSize);
				ImGui::InputFloat("Scale", &scale);

			}
			if (ImGui::Button("Cargar")) {
				
				try
				{
					this->pointClouds.push_back(new MultiPointCloud(1, this->fileURL, nOctrees, maxDepth, maxPoints, childContainingAllPoints, totalSize, scale));
					this->currentPointCloud = this->pointClouds.size() - 1;
					this->camera.setCenterPointCloud(this->pointClouds[this->currentPointCloud]->getCenter());
					this->viewPort->setPointCloud(this->pointClouds[this->currentPointCloud]);
					if (nOctrees == 3) {

					}

					this->initUniforms();
				}
				catch (const std::exception&)
				{
				}
				
			}
			if (ImGui::Button("Borrar")) {
				if (this->pointClouds.size() - 1 > 0) {
					this->pointClouds.erase(this->pointClouds.begin() + this->currentPointCloud);
					this->pointClouds.erase(this->pointClouds.begin() + this->currentPointCloud);
					this->currentPointCloud = this->pointClouds.size() - 1;

					this->camera.setCenterPointCloud(this->pointClouds[this->currentPointCloud]->getCenter());
					this->viewPort->setPointCloud(this->pointClouds[this->currentPointCloud]);
				}
				else {

				}
					
			}
			// Point cloud selector
			ImGui::Text("Select PointCloud:");
			for (int i = 0; i < this->pointClouds.size(); i++) {
				if (ImGui::RadioButton(
					this->pointClouds[i]->GetFilePath().c_str(),
					this->currentPointCloud == i)) {
					this->currentPointCloud = i;
					this->camera.setCenterPointCloud(this->pointClouds[this->currentPointCloud]->getCenter());
					this->viewPort->setPointCloud(this->pointClouds[this->currentPointCloud]);
					this->initUniforms();
				}
			}
			
		}
		
		if (ImGui::CollapsingHeader("Camera")) {
			ImGui::Checkbox("FreeMode", this->camera.getFreeMode());
			ImGui::SliderFloat("Fov", this->camera.getFov(), 1.0f, 120.0f);
			ImGui::SliderFloat("Movement Speed", this->camera.getMovementSpeed(), 0.f, 60.f);
			ImGui::SliderFloat("Sensitivity", this->camera.getSensitivity(), 0.f, 120.f);
			ImGui::SliderFloat3("Position", &this->camera.getPosition2()->x, -180.f, 180.f);
			ImGui::SliderFloat3("Front", &this->camera.getFront()->x, -180.f, 180.f);
			ImGui::SliderFloat("Linear", this->camera.getInterpolation(), 0.0f, 1.0f);
			/*if (ImGui::Button("Set Position 1")) {
				this->camera.setSpecialPosition(0);
			}*/
			
		}

		if (ImGui::CollapsingHeader("Modes")) {
			



			if (ImGui::Checkbox("Voxel", &voxelModeSelected)) {
				if (voxelModeSelected) octreeModeSelected = false;
				this->currentShader = SHADER_VOXEL;
				this->initUniforms();
			}

			

			if (ImGui::Checkbox("Octree", &octreeModeSelected)) {
				if (octreeModeSelected) voxelModeSelected = false;
				this->currentShader = SHADER_OCTREE;
				this->initUniforms();
			}
			ImGui::Checkbox("Phong", &shading);
			if (shading) {
				shadingMode = 1;
			}
			else {
				shadingMode = 0;
			}
			if (octreeModeSelected) {
				ImGui::Checkbox("Segmented", &segmented);
				ImGui::Checkbox("Error", &error);
				ImGui::Checkbox("Extra", &extra);
				ImGui::Checkbox("Order", &order);
				ImGui::Checkbox("Shadow", &shadow);
				ImGui::Checkbox("ErrorShadow", &errorShadow);
				octreeError = (error) ? 1 : 0;
				octreeExtra = (extra) ? 1 : 0;
				octreeGround = (segmented) ? 1 : 0;
				shadowMode = (shadow) ? 1 : 0;
				orderMode = (order) ? 1 : 0;
				specialShadowErrorMode = (errorShadow) ? 1 : 0;

			}
		}
		if (ImGui::CollapsingHeader("Shading")) {
			if (octreeModeSelected) {
				ImGui::SliderFloat("g", this->pointClouds[this->currentPointCloud]->getRadius(), 0.01f, 9.0f);
				ImGui::SliderFloat("t", this->pointClouds[this->currentPointCloud]->getRadiusError(), 0.01f, 9.0f);
			}

			bool addLightButtonPressed = false;
			bool deleteLightButtonPressed = false;

			// Add Light Button
			if (ImGui::Button("Add Light"))
			{
				addLightButtonPressed = true;
			}

			// Delete Light Button
			if (ImGui::Button("Delete Light"))
			{
				deleteLightButtonPressed = true;
			}

			// Check if the buttons were pressed
			if (addLightButtonPressed)
			{
				// Create a new light and add it to your list
				PointLight* newLight = new PointLight(glm::vec3(0.5f));
				pointLights.push_back(newLight);

				// Reset the button state
				addLightButtonPressed = false;
			}

			if (deleteLightButtonPressed)
			{
				// Check if there are lights to delete
				if (!pointLights.empty())
				{
					// Remove the last light from the list
					Light* lastLight = pointLights.back();
					pointLights.pop_back();

					// Delete the light object
					delete lastLight;
				}

				// Reset the button state
				deleteLightButtonPressed = false;
			}

			for (int i = 0; i < pointLights.size(); i++)
			{
				PointLight* light = pointLights[i];
				std::string lightName = "Point Light " + std::to_string(i + 1);

				if (ImGui::CollapsingHeader(lightName.c_str()))
				{
					ImGui::ColorEdit3("Ia", reinterpret_cast<float*>(&light->getIa()->x));
					ImGui::ColorEdit3("Id", reinterpret_cast<float*>(&light->getId()->x));
					ImGui::ColorEdit3("Is", reinterpret_cast<float*>(&light->getIs()->x));

					ImGui::SliderFloat("Intensity", light->getIntensity(), 0.0f, 1.0f);
					ImGui::SliderFloat("Constant", light->getConstant(), 0.0f, 1.0f);
					ImGui::SliderFloat("Linear", light->getLinear(), 0.0f, 1.0f);
					ImGui::SliderFloat("Quadratic", light->getQuadratic(), 0.0f, 1.0f);

					// You can also add sliders for position here if needed
					ImGui::SliderFloat3("Position", &light->getPosition()->x, -100.f, 100.f);
				}
			}

			ImGui::ColorEdit3("Global Light", reinterpret_cast<float*>(&this->globalLight.x));


			
		}

		if (ImGui::CollapsingHeader("Planes")) {
			ImGui::Checkbox("Show Planes", &showPlanes);
			ImGui::SliderFloat("Scale factor", pointClouds[currentPointCloud]->getScalePlaneFactor(), 0.0f, 30.0f);
			ImGui::ColorEdit3("Ambient", reinterpret_cast<float*>(&materials[0]->getAmbient()->x));
			ImGui::ColorEdit3("Diffuse", reinterpret_cast<float*>(&materials[0]->getDiffuse()->x));
			ImGui::ColorEdit3("Specular", reinterpret_cast<float*>(&materials[0]->getSpecular()->x));
			if (showPlanes) {
				planesMode = 1;
			}
			else {
				planesMode = 0;
			}
		}







		ImGuiIO& io = ImGui::GetIO();
		float fps = io.Framerate;
		ImGui::Text("FPS: %.1f", fps);
		ImGui::Text("Witdh: %d", this->framebufferWidth);
		ImGui::Text("Height: %d", this->framebufferHeight);
		ImGui::Text("CameraDistance: %.1f", this->camera.getCameraDistance());
		if (octreeModeSelected) 
		{
			ImGui::Text("Number Nodes: %d", this->pointClouds[currentPointCloud]->getOctreeSize());
			ImGui::Text("Number Octrees: %d", this->pointClouds[currentPointCloud]->getOctreeNumberOctrees());
			ImGui::Text("Max Depth: %d", this->pointClouds[currentPointCloud]->getOctreeMaxDepth());
			ImGui::Text("Max Points Per Node: %d", this->pointClouds[currentPointCloud]->getOctreeMaxPointsPerNode());
			ImGui::Text("Total Points: %d", this->pointClouds[currentPointCloud]->getOctreePointsSize());
			ImGui::Text("Octree1 Points: %d", this->pointClouds[currentPointCloud]->getOctree1PointsSize());
			ImGui::Text("Octree2 Points: %d", this->pointClouds[currentPointCloud]->getOctree2PointsSize());
			ImGui::Text("Octree3 Points: %d", this->pointClouds[currentPointCloud]->getOctree3PointsSize());
			ImGui::Text("Octree1 Nodes: %d", this->pointClouds[currentPointCloud]->getNodesPerOctree(0));
			ImGui::Text("Octree2 Nodes: %d", this->pointClouds[currentPointCloud]->getNodesPerOctree(1));
			ImGui::Text("Octree3 Nodes: %d", this->pointClouds[currentPointCloud]->getNodesPerOctree(2));
		}

	}
	

	ImGui::End();

	

	ImGui::Render();
}


void Game::updateUniforms()
{
	this->shaders[this->currentShader]->set1i(this->mode, "mode");
	this->shaders[this->currentShader]->set1i(this->octreeError, "octreeError");
	this->shaders[this->currentShader]->set1i(this->octreeGround, "octreeGround");
	this->shaders[this->currentShader]->set1i(this->octreeExtra, "octreeExtra");
	this->shaders[this->currentShader]->set1i(this->shadowMode, "shadowMode");
	this->shaders[this->currentShader]->set1i(this->shadingMode, "shadingMode");
	this->shaders[this->currentShader]->set1i(this->orderMode, "orderMode");
	this->shaders[this->currentShader]->set1i(this->specialShadowErrorMode, "specialShadowErrorMode");
	this->shaders[this->currentShader]->set1i(this->planesMode, "planesMode");
	this->shaders[this->currentShader]->set1i(this->pointLights.size(), "nLights");

	
	this->updateRandom();
	this->shaders[this->currentShader]->setVec2f(this->randomSeed, "randomSeed");

	if (this->voxelModeSelected) {
		this->shaders[this->currentShader]->set1i(7, "texture3DD");
	}
	else 
	{
		this->shaders[this->currentShader]->set1i(1, "u_multiNodesTextureFloat");
		this->shaders[this->currentShader]->set1i(2, "u_multiNodesTextureInt");
		this->shaders[this->currentShader]->set1i(3, "u_multiPointsTexture1");
		this->shaders[this->currentShader]->set1i(4, "u_multiPointsTexture2");
		this->shaders[this->currentShader]->set1i(5, "u_multiPointsTexture3");
	}

	this->shaders[this->currentShader]->setVec3f(this->globalLight, "globalLight");

	//Update view matrix
	this->ViewMatrix = this->camera.getViewMatrix();
	this->shaders[this->currentShader]->setMat4fv(inverse(this->ViewMatrix), "ViewMatrix");
	this->shaders[this->currentShader]->setVec3f(this->camera.getPosition(), "camPosition");

	//Send lights
	for (int i = 0; i < this->pointLights.size(); i++) {
		this->pointLights[i]->sendToShader(*this->shaders[this->currentShader], i);
	}


	//Update Projection Matrix
	glfwGetFramebufferSize(this->window, &this->framebufferWidth, &this->framebufferHeight);
	this->shaders[this->currentShader]->setVec2f(glm::vec2(this->framebufferWidth, this->framebufferHeight), "window_size");

	this->camera.updateFramebufferSize(this->framebufferWidth, this->framebufferHeight);
	this->ProjectionMatrix = this->camera.getProjectionMatrix();

	this->shaders[this->currentShader]->setMat4fv(inverse(this->ProjectionMatrix), "ProjectionMatrix");

}


Game::Game(const char* title,
	const int WINDOW_WIDTH, const int WINDOW_HEIGHT,
	const int GL_VERSION_MAJOR, const int GL_VERSION_MINOR,
	bool resizable) 
	: 
	WINDOW_WIDTH(WINDOW_WIDTH),
	WINDOW_HEIGHT(WINDOW_HEIGHT),
	GL_VERSION_MAJOR(GL_VERSION_MAJOR), 
	GL_VERSION_MINOR(GL_VERSION_MINOR),
	camera(glm::vec3(30.f, 30.f, 30.f), glm::vec3(1.f, 1.f, 1.f), glm::vec3(0.f, 1.f, 0.f), WINDOW_WIDTH, WINDOW_HEIGHT)
{
	//Init variables
	this->window = nullptr;
	this->framebufferWidth = this->WINDOW_WIDTH;
	this->framebufferHeight = this->WINDOW_HEIGHT;

	
	//Basic camera ViewMatrix
	this->camPosition = glm::vec3(0.f, 0.f, 1.f);
	this->worldUp = glm::vec3(0.f, 1.f, 0.f);
	this->camFront = glm::vec3(0.f, 0.f, -1.f);

	//ProjectionMatrix
	this->fov = 90.f;
	this->nearPlane = 0.1f;
	this->farPlane = 1000.f;
	this->ProjectionMatrix = glm::mat4(1.f);

	//Input mouse
	this->dt = 0.f;
	this->curTime = 0.f;
	this->lastTime = 0.f;

	this->lastMouseX = 0.0;
	this->lastMouseY = 0.0;
	this->mouseX = 0.0;
	this->mouseY = 0.0;
	this->mouseOffsetX = 0.0;
	this->mouseOffsetY = 0.0;
	this->firstMouse = true;

	this->currentShader = SHADER_OCTREE;
	this->currentPointCloud = 0;

	this->octreeError = 1;
	this->octreeGround = 1;
	this->octreeExtra = 1;
	this->shadowMode = 0;
	this->shadingMode = 1;
	this->orderMode = 1;
	this->planesMode = 1;
	this->specialShadowErrorMode = 1;

	this->initGLFW(); //OK
	this->initWindow(title, resizable); //OK
	this->initGLEW(); //OK
	this->initImGui(); //OK
	this->initOpenGLOptions(); //OK
	this->initMatrices(); //OK

	this->initShaders(); //OK
	this->initPointCloud(); //OK
	this->camera.setCenterPointCloud(this->pointClouds[0]->getCenter());
	this->initMaterials(); //OK
	this->initViewPort(); 
	this->initLights();
	this->initUniforms();

}


Game::~Game()
{
	glfwDestroyWindow(this->window);
	glfwTerminate();

	for (size_t i = 0; i < this->shaders.size(); i++)
		delete this->shaders[i];

	for (size_t i = 0; i < this->materials.size(); i++)
		delete this->materials[i];

	for (size_t i = 0; i < this->pointClouds.size(); i++)
		delete this->pointClouds[i];

	
	delete this->viewPort;

	for (size_t i = 0; i < this->pointLights.size(); i++)
		delete this->pointLights[i];

	// ImGui Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}


//Accessors
int Game::getWindowShouldClose()
{
	return glfwWindowShouldClose(this->window);
}

//Modifiers
void Game::setWindowShouldClose()
{
	glfwSetWindowShouldClose(this->window, GLFW_TRUE);
}


//Functions
void Game::updateDt()
{
	this->curTime = static_cast<float>(glfwGetTime());
	this->dt = this->curTime - this->lastTime;
	this->lastTime = this->curTime;
}

void Game::updateMouseInput()
{
	glfwGetCursorPos(this->window, &this->mouseX, &this->mouseY);

	if (this->firstMouse)
	{
		this->lastMouseX = this->mouseX;
		this->lastMouseY = this->mouseY;
		this->firstMouse = false;
	}

	//Calc offset
	this->mouseOffsetX = this->mouseX - this->lastMouseX;
	this->mouseOffsetY = this->lastMouseY - this->mouseY;


	//Set last X and Y
	this->lastMouseX = this->mouseX;
	this->lastMouseY = this->mouseY;

	//Move light
	if (glfwGetMouseButton(this->window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS)
	{
		//this->pointLights[0]->setPosition(this->camera.getPosition());
		this->pointLights[0]->setPosition(this->camera.getPosition());
	}
	if (glfwGetMouseButton(this->window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
	{
		//this->u_mouseX = (2.0f * mouseX) / this->WINDOW_WIDTH - 1.0f; TODO PARA EL RAYTREACING DE UN SOLO PUNTO
		//this->u_mouseY - (2.0f * mouseY) / this->WINDOW_HEIGHT; 
	}

	

	
}



void Game::updateKeyboardInput()
{
	//Program
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		setWindowShouldClose();
	}

	//Movement
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		this->camera.move(this->dt, FORWARD);
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		this->camera.move(this->dt, BACKWARD);
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		this->camera.move(this->dt, LEFT);
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		this->camera.move(this->dt, RIGHT);
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
		this->camera.move(this->dt, UP);
	}
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
		this->camera.move(this->dt, DOWN);
	}
	if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) {
		this->pointClouds[this->currentPointCloud]->addRadius(this->dt);
	}
	if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) {
		this->pointClouds[this->currentPointCloud]->addRadius(-this->dt);
	}

	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
		this->camera.setFov(0.05f);
	}
	if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
		this->camera.setFov(-0.05f);
	}
	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
		this->mode = 1;
	}
	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
		this->mode = 2;
	}
	if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
		this->mode = 3;
	}
	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
		this->camera.toggleCameraMode();
	}
}

void Game::updateInput()
{
	glfwPollEvents();
	this->updateKeyboardInput();
	this->updateMouseInput();
	bool leftMousePressed = (glfwGetMouseButton(this->window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
	this->camera.updateInput(dt, -1, this->mouseOffsetX, this->mouseOffsetY, leftMousePressed);
}

void Game::updateMeshPosition() 
{

}

void Game::update()
{
	//Update Input
	this->updateDt();
	this->updateInput();
	this->updateUniforms();
}

void Game::render()
{
	//Update
	

	//DRAW
	//clear
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	

	renderImGui();

	
	

	//Render models
	if (this->voxelModeSelected) {
		this->viewPort->render3DTexture(this->shaders[this->currentShader]);
	}
	else {
		this->viewPort->renderOctree(this->shaders[this->currentShader]);
	}
	

	this->pointClouds[this->currentPointCloud]->render(this->shaders[this->currentShader]);

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	//EndDraw
	glfwSwapBuffers(this->window);
	glFlush();

	//Resets
	glBindVertexArray(0);
	glUseProgram(0);
	glActiveTexture(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

//Static functions
void Game::framebuffer_resize_callback(GLFWwindow* window, int fbW, int fbH)
{
	glViewport(0, 0, fbW, fbH);
}
