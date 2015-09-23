#include "main.h"

#include "Utility.h"
#include "Light.h"
#include "system_context.h"
#include "DeviceMesh.h"	// ad hoc
#include "gl_snippets.h"

#include <SOIL/SOIL.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform2.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/verbose_operator.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>
#include <list>
#include <random>
#include <ctime>
#include <algorithm>

#include "InstantRadiosity.h"

constexpr float PI = 3.14159f;
constexpr float kFarPlane = 2000.f;
constexpr float kNearPlane = 0.1f;

int mouse_buttons = 0;
int mouse_old_x = 0, mouse_dof_x = 0;
int mouse_old_y = 0, mouse_dof_y = 0;


std::list<LightData> lightList;
std::default_random_engine random_gen ((unsigned int)(time (NULL)));

system_context *context;

glm::mat4 get_mesh_world ();

device_mesh2_t device_quad;

void initQuad()
{
	vertex2_t verts[] = { {glm::vec3(-1, 1,0), glm::vec2(0,1)},
						  {glm::vec3(-1,-1,0), glm::vec2(0,0)},
						  {glm::vec3( 1,-1,0), glm::vec2(1,0)},
						  {glm::vec3( 1, 1,0), glm::vec2(1,1)} };

    unsigned short indices[] = { 0, 1, 2, 0, 2, 3 };

    //Allocate vertex array
    //Vertex arrays encapsulate a set of generic vertex attributes and the buffers they are bound too
    //Different vertex array per mesh.
    glGenVertexArrays(1, &(device_quad.vertex_array));
    glBindVertexArray(device_quad.vertex_array);

    //Allocate vbos for data
    glGenBuffers(1,&(device_quad.vbo_data));
    glGenBuffers(1,&(device_quad.vbo_indices));

    //Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, device_quad.vbo_data);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    //Use of strided data, Array of Structures instead of Structures of Arrays
    glVertexAttribPointer(quad_attributes::POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(vertex2_t), 0);
    glVertexAttribPointer(quad_attributes::TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(vertex2_t), (void*)sizeof(glm::vec3));
    glEnableVertexAttribArray(quad_attributes::POSITION);
    glEnableVertexAttribArray(quad_attributes::TEXCOORD);

    //indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, device_quad.vbo_indices);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6*sizeof(GLushort), indices, GL_STATIC_DRAW);
    device_quad.num_indices = 6;

    //Unplug Vertex Array
    glBindVertexArray(0);
}

GLuint forward_shading_prog = 0;

void shaderInit() 
{
	// Vertex shaders
	const char *pass_vert = "res/shaders/forward_vert.glsl";
	const char *post_vert = "res/shaders/post.vert";
	
	// Fragment shaders
	const char *forward_frag = "res/shaders/forward_frag.glsl";
	const char *post_frag = "res/shaders/post.frag";

	std::unique_ptr<gls::program> programs[PROG_MAX];
	programs[PROG_SCENEDRAW].reset(new gls::program(kProgramSceneDraw));

	{
		utility::shaders_t shaders = utility::loadShaders(pass_vert, forward_frag);
		progs[PROG_SCENEDRAW] = glCreateProgram();
		glBindAttribLocation(progs[PROG_SCENEDRAW], mesh_attributes::POSITION, "Position");
		glBindAttribLocation(progs[PROG_SCENEDRAW], mesh_attributes::NORMAL, "Normal");
		utility::attachAndLinkProgram(progs[PROG_SCENEDRAW], shaders);
		glBindFragDataLocation(progs[PROG_SCENEDRAW], 0, "outColor");

	}
	{
		utility::shaders_t shaders = utility::loadShaders(post_vert, post_frag);
		progs[PROG_QUADDRAW] = glCreateProgram();
		glBindAttribLocation(progs[PROG_QUADDRAW], quad_attributes::POSITION, "Position");
		glBindAttribLocation(progs[PROG_QUADDRAW], quad_attributes::TEXCOORD, "Texcoord");
		utility::attachAndLinkProgram(progs[PROG_QUADDRAW], shaders);
		glBindFragDataLocation(progs[PROG_QUADDRAW], 0, "outColor");
	}
}

void texInit()
{
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	int fbo_width, fbo_height;
	fbo_width = viewport[2];
	fbo_height = viewport[3];

	glGenTextures(TEX_MAX, textures);
	for (int i = 0; i < TEX_MAX; ++i)
	{
		glBindTexture(GL_TEXTURE_2D, textures[(Textures)i]);

		// Set texture parameters for each texture
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		if (formats[i] == GL_DEPTH_COMPONENT)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
		}
		if (formats[i] == GL_RED)
		{
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		}

		glTexImage2D(GL_TEXTURE_2D, 0, internalFormats[i],
			fbo_width, fbo_height, 0, formats[i], types[i], NULL);
	}

	glBindTexture(GL_TEXTURE_2D, NULL);
}

void fboInit()
{
	glGenFramebuffers(FBO_MAX, fbo);

	texInit();

	GLenum status;

	glBindFramebuffer(GL_FRAMEBUFFER, fbo[FBO_SCENEDRAW]);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
		GL_TEXTURE_2D, textures[TEX_SCENE], 0);
	status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	assert(status == GL_FRAMEBUFFER_COMPLETE);
	{
		GLuint attachments[1] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, attachments);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, fbo[FBO_ACCUMULATE]);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, textures[TEX_ACCUM], 0);
	status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	assert(status == GL_FRAMEBUFFER_COMPLETE);
	{
		GLuint attachments[1] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, attachments);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 get_mesh_world() 
{
	return glm::mat4(1.0);
}

int lightIdx = 0;

void draw_mesh_forward () 
{
    glm::mat4 model = get_mesh_world();
	glm::mat4 view,lview, persp, lpersp;

	view = context->pCam.get_view(); // Camera view Matrix
	persp = glm::perspective(45.0f, (float)context->viewport.x / (float)context->viewport.y, kNearPlane, kFarPlane);
	lpersp = glm::perspective(120.0f, (float)context->viewport.x / (float)context->viewport.y, kNearPlane, kFarPlane);

	GLuint modelMatLoc = glGetUniformLocation(progs[PROG_SCENEDRAW], "u_ModelMat");
	GLuint viewMatLoc = glGetUniformLocation(progs[PROG_SCENEDRAW], "u_ViewMat");
	GLuint perspMatLoc = glGetUniformLocation(progs[PROG_SCENEDRAW], "u_PerspMat");
	GLuint vplPosLoc = glGetUniformLocation(progs[PROG_SCENEDRAW], "u_vplPosition");
	GLuint vplIntLoc = glGetUniformLocation(progs[PROG_SCENEDRAW], "u_vplIntensity");
	GLuint vplDirLoc = glGetUniformLocation(progs[PROG_SCENEDRAW], "u_vplDirection");
	GLuint numLightsLoc = glGetUniformLocation(progs[PROG_SCENEDRAW], "u_numLights");
	GLuint ambiColorLoc = glGetUniformLocation(progs[PROG_SCENEDRAW], "u_AmbientColor");
	GLuint diffColorLoc = glGetUniformLocation(progs[PROG_SCENEDRAW], "u_DiffuseColor");
	GLuint textureLoc = glGetUniformLocation(progs[PROG_QUADDRAW], "u_Tex");
    
	glUniform1i(numLightsLoc, context->VPLs.size());
	//glUniform1i(numLightsLoc, 1);
	/*
	if (indirectON)
		glUniform1i(glGetUniformLocation(forward_shading_prog, "u_numVPLs"), nVPLs);
	else
		glUniform1i(glGetUniformLocation(forward_shading_prog, "u_numVPLs"), 0);
	*/
	{
		using glm::value_ptr;

		glUniformMatrix4fv(modelMatLoc, 1, GL_FALSE, value_ptr(model));
		glUniformMatrix4fv(viewMatLoc, 1, GL_FALSE, value_ptr(view));
		glUniformMatrix4fv(perspMatLoc, 1, GL_FALSE, value_ptr(persp));
		/*
		glUniform3fv(vplPosLoc, 1, value_ptr(context->VPLs[lightIdx].position));
		glUniform3fv(vplIntLoc, 1, value_ptr(context->VPLs[lightIdx].intensity));
		glUniform3fv(vplDirLoc, 1, value_ptr(context->VPLs[lightIdx].direction));
		*/
		glBindFramebuffer(GL_FRAMEBUFFER, fbo[FBO_ACCUMULATE]);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		for (int lightIter = 0; lightIter < context->VPLs.size(); ++lightIter)
		{
			glUniform3fv(vplPosLoc, 1, value_ptr(context->VPLs[lightIter].position));
			glUniform3fv(vplIntLoc, 1, value_ptr(context->VPLs[lightIter].intensity));
			glUniform3fv(vplDirLoc, 1, value_ptr(context->VPLs[lightIter].direction));

			glUseProgram(progs[PROG_SCENEDRAW]);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			for (int i = 0; i < context->drawMeshes.size(); i++)
			{
				glUniform3fv(diffColorLoc, 1, value_ptr(context->drawMeshes[i].diffuseColor));
				glUniform3fv(ambiColorLoc, 1, value_ptr(context->drawMeshes[i].ambientColor));
				glBindVertexArray(context->drawMeshes[i].vertex_array);
				glDrawElements(GL_TRIANGLES, context->drawMeshes[i].num_indices, GL_UNSIGNED_SHORT, 0);
			}

			/*
			glUseProgram(progs[PROG_QUADDRAW]);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			// bind accumulation texture
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, textures[TEX_SCENE]);
			glUniform1i(textureLoc, 0);
			// draw screenquad with accumulation texture
			glBindVertexArray(device_quad.vertex_array);
			glDrawElements(GL_TRIANGLES, device_quad.num_indices, GL_UNSIGNED_SHORT, 0);
			*/
			/*
			glUseProgram(progs[PROG_QUADDRAW]);
			glBindFramebuffer(GL_FRAMEBUFFER, fbo[FBO_ACCUMULATE]);
			// bind scenedraw texture
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, textures[TEX_SCENE]);
			glUniform1i(textureLoc, 0);
			// enable blend
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			// draw screenquad with scenedraw texture
			glBindVertexArray(device_quad.vertex_array);
			glDrawElements(GL_TRIANGLES, device_quad.num_indices, GL_UNSIGNED_SHORT, 0);
			// disable blend
			glDisable(GL_BLEND);
			*/
		}
		/*
		glUseProgram(progs[PROG_QUADDRAW]);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		// bind accumulation texture
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textures[TEX_ACCUM]);
		glUniform1i(textureLoc, 0);
		// draw screenquad with accumulation texture
		glBindVertexArray(device_quad.vertex_array);
		glDrawElements(GL_TRIANGLES, device_quad.num_indices, GL_UNSIGNED_SHORT, 0);
		*/
	}
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
}

Display display_type = DISPLAY_TOTAL;
void draw_quad() 
{
    glBindVertexArray(device_quad.vertex_array);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, device_quad.vbo_indices);

    glDrawElements(GL_TRIANGLES, device_quad.num_indices, GL_UNSIGNED_SHORT,0);

    glBindVertexArray(0);
}

void update_title() 
{
	//FPS calculation
	static unsigned long frame = 0;
	static double time_base = glfwGetTime();
	++frame;
	double time_now = glfwGetTime();

	if (time_now - time_base > 1.0) {//update title if a second passes
		const char *displaying;
		switch (display_type) {
		case(DISPLAY_DEPTH) :
			displaying = "Depth";
			break;
		case(DISPLAY_NORMAL) :
			displaying = "Normal";
			break;
		case(DISPLAY_COLOR) :
			displaying = "Color";
			break;
		case(DISPLAY_POSITION) :
			displaying = "Position";
			break;
		case(DISPLAY_TOTAL) :
			displaying = "Diffuse";
			break;
		case(DISPLAY_LIGHTS) :
			displaying = "Lights";
			break;
		case DISPLAY_GLOWMASK:
			displaying = "Glow Mask";
			break;
		case(DISPLAY_SHADOW) :
			displaying = "ShadowMap";
			break;
		}

		std::string title = utility::sprintfpp("CS482 Instant Radiosity | Displaying <%s> | FPS: %4.2f",
			displaying,
			frame / (time_now - time_base)
			);

		glfwSetWindowTitle(context->window, title.c_str());
		//reset per-second frame statistics for next update
		time_base = time_now;
		frame = 0;
	}
}

void render_forward ()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

	draw_mesh_forward ();
}

void window_callback_size(GLFWwindow *window, int w, int h)
{
    context->viewport.x = w;
    context->viewport.y = h;
    glBindFramebuffer(GL_FRAMEBUFFER,0);
    glViewport(0,0,(GLsizei)w,(GLsizei)h);
}

void window_callback_mouse_button(GLFWwindow *window, int button, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		mouse_buttons |= 1 << button;
	}
	else if (action == GLFW_RELEASE)
	{
		mouse_buttons = 0;
	}
	{
		double x, y;
		glfwGetCursorPos(window, &x, &y);

		mouse_old_x = int(x);
		mouse_old_y = int(y);
	}
	if (button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		mouse_dof_x = mouse_old_x;
		mouse_dof_y = mouse_old_y;
	}
}

void window_callback_cursor_pos(GLFWwindow *window, double x, double y)
{
	float dx, dy;
	dx = -(float)(x - mouse_old_x);
	dy = (float)(y - mouse_old_y);
	float sensitivity = 0.001f;

	if (mouse_buttons & 1 << GLFW_MOUSE_BUTTON_RIGHT)
	{
		//context->pCam.adjust(0,0,dx,0,0,0);;
	}
	else if (mouse_buttons & 1 << GLFW_MOUSE_BUTTON_LEFT)
	{
		context->pCam.rotate(glm::vec3(dy * sensitivity, 0, dx * sensitivity));
	}

	mouse_old_x = int(x);
	mouse_old_y = int(y);
}

void window_callback_key(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    float tx = 0;
    float ty = 0;
    float tz = 0;
	if (action == GLFW_RELEASE) //no need to process key up events
		return;
	float speed = 10.f;
    switch(key) 
	{
	case GLFW_KEY_ESCAPE:
		glfwSetWindowShouldClose(window, true);
        break;
	case('W') :
		tz = speed;
        break;
	case('S') :
		tz = -speed;
        break;
	case('D') :
		tx = -speed;
        break;
	case('A') :
		tx = speed;
        break;
	case('Q') :
		ty = speed;
        break;
	case('Z') :
		ty = -speed;
        break;
    case('1'):
        display_type = DISPLAY_DEPTH;
        break;
    case('2'):
        display_type = DISPLAY_NORMAL;
        break;
    case('3'):
        display_type = DISPLAY_COLOR;
        break;
    case('4'):
        display_type = DISPLAY_POSITION;
        break;
    case('5'):
        display_type = DISPLAY_LIGHTS;
        break;
    case('6'):
        display_type = DISPLAY_GLOWMASK;
        break;
    case('0'):
        display_type = DISPLAY_TOTAL;
        break;
    case('R'):
        shaderInit();
		break;
	case('7'):
        display_type = DISPLAY_SHADOW;
		break;
	case(' '):
		lightIdx = (lightIdx + 1) % context->VPLs.size();
		break;
    }

    if (abs(tx) > 0 ||  abs(tz) > 0 || abs(ty) > 0) 
	{
		context->pCam.translate(glm::vec3(tx, ty, tz));
    }
}

void init() 
{
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f,1.0f);
}

namespace {
	//opengl initialization: GLFW, GLEW and our application window
	class opengl_initializer_t {
	public:
		opengl_initializer_t();
		opengl_initializer_t(const opengl_initializer_t&) = delete;
		opengl_initializer_t& operator=(const opengl_initializer_t&) = delete;
		~opengl_initializer_t();
		opengl_initializer_t(opengl_initializer_t&&) = delete;
		opengl_initializer_t& operator=(opengl_initializer_t&&) = delete;
	};

	opengl_initializer_t::opengl_initializer_t() {
		//initialize glfw
		if (!glfwInit())
			throw std::runtime_error("glfwInit() failed");

		try {
			//create window
			if (!(context->window = glfwCreateWindow(context->viewport.x, context->viewport.y, "InstantRadiosity", NULL, NULL)))
				throw std::runtime_error("glfw window creation failed");
			glfwMakeContextCurrent(context->window);

			//set callbacks
			glfwSetWindowSizeCallback(context->window, window_callback_size);
			glfwSetKeyCallback(context->window, window_callback_key);
			glfwSetCursorPosCallback(context->window, window_callback_cursor_pos);
			glfwSetMouseButtonCallback(context->window, window_callback_mouse_button);

			//initialize glew
			if (glewInit() != GLEW_OK)
				throw std::runtime_error("glewInit() failed");

			//check version requirement
			//TODO: update correct requirement later
			if (!GLEW_VERSION_3_3 || !GLEW_ARB_compute_shader)
				throw std::runtime_error("This program requires OpenGL 3.3 class graphics card.");
			else {
				std::cerr << "Status: Using GLEW " << glewGetString(GLEW_VERSION) << std::endl;
				std::cerr << "OpenGL version " << glGetString(GL_VERSION) << " supported" << std::endl;
			}
		}
		catch (...) {
			glfwTerminate();
			throw;
		}
	}
	opengl_initializer_t::~opengl_initializer_t() {
		glfwTerminate();
	}
}
int main(int argc, char* argv[]) {
	//Step 0: Initialize our system context
	{
		glm::uvec2 viewport(1280, 720);
		Camera default_camera(
			glm::vec3(300, 300, -500),
			glm::vec3(0, 0, 1),
			glm::vec3(0, 1, 0),
			glm::perspective(
				45.0f,
				float(viewport.x) / float(viewport.y),
				kNearPlane,
				kFarPlane
				)
			);
		context = system_context::initialize(default_camera, viewport);
	}

	//Step 1: Load mesh into memory
	if (argc > 1) {
		try {
			context->loadObj(argv[1]);
		}
		catch (const std::exception &e) {
			std::cerr << "Mesh load failed. Reason: " << e.what() << "\nAborting.\n";
			return EXIT_FAILURE;
		}
	}
	else {
		std::cerr << utility::sprintfpp("Usage: %s mesh=[obj file]\n", argv[0]);
		return EXIT_SUCCESS;
	}

	//Step 2: Initialize GLFW & GLEW
	//RAII initialization of GLFW, GLEW and our application window
	std::unique_ptr<opengl_initializer_t> opengl_initializer;
	try {
		opengl_initializer.reset(new opengl_initializer_t);
	}
	catch(const std::exception &e) {
		std::cerr << "OpenGL initialization failed. Reason: " << e.what() << "\nAborting.\n";
		return EXIT_FAILURE;
	}
	
	//Step 3: Initialize objects
	//initNoise();
	shaderInit();
    init();
	initQuad();
	fboInit();
	context->initMesh();

	//Step 4: Main loop
	while (!glfwWindowShouldClose(context->window)) {
		render_forward();
		update_title();

		glfwSwapBuffers(context->window);
		glfwPollEvents();
	}

    return EXIT_SUCCESS;
}