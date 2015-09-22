#include "main.h"

#include "Utility.h"
#include "Light.h"
#include "SystemContext.h"
#include "DeviceMesh.h"	// ad hoc

#include "SOIL.h"
#include <GL/glut.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_projection.hpp>
#include <glm/gtc/matrix_operation.hpp>
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

const float PI = 3.14159f;
const int MAX_LIGHTS_PER_TILE = 64;

bool forwardR = true, indirectON = false, DOFEnabled = false, DOFDebug = false;

int mouse_buttons = 0;
int mouse_old_x = 0, mouse_dof_x = 0;
int mouse_old_y = 0, mouse_dof_y = 0;

int nVPLs = 512;
int nLights = 0;
int nBounces = 1;

float FARP = 2000.f;
float NEARP = 0.1f;

std::list<LightData> lightList;
std::default_random_engine random_gen (time (NULL));

SystemContext *context;

glm::mat4 get_mesh_world ();

device_mesh2_t device_quad;
void initQuad()
{
    vertex2_t verts [] = {	{glm::vec3(-1,1,0),glm::vec2(0,1)},
							{glm::vec3(-1,-1,0),glm::vec2(0,0)},
							{glm::vec3(1,-1,0),glm::vec2(1,0)},
							{glm::vec3(1,1,0),glm::vec2(1,1)}		};

    unsigned short indices[] = { 0,1,2,0,2,3};

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
    glVertexAttribPointer(quad_attributes::POSITION, 3, GL_FLOAT, GL_FALSE,sizeof(vertex2_t),0);
    glVertexAttribPointer(quad_attributes::TEXCOORD, 2, GL_FLOAT, GL_FALSE,sizeof(vertex2_t),(void*)sizeof(glm::vec3));
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

void initShader() 
{
#ifdef WIN32
	// Common shaders
	const char * pass_vert = "../../../res/shaders/forward_vert.glsl";
	
	// Forward shaders
	const char * forward_frag = "../../../res/shaders/forward_frag.glsl";
#else
	// Common shaders
	const char * pass_vert = "../res/shaders/pass.vert";

	// Forward shaders
	const char * forward_frag = "../res/shaders/forward_frag.glsl";
#endif

	utility::shaders_t shaders = utility::loadShaders(pass_vert, forward_frag);
	forward_shading_prog = glCreateProgram();
	glBindAttribLocation(forward_shading_prog, mesh_attributes::POSITION, "Position");
    glBindAttribLocation(forward_shading_prog, mesh_attributes::NORMAL, "Normal");
    //glBindAttribLocation(forward_shading_prog, mesh_attributes::TEXCOORD, "Texcoord");
	utility::attachAndLinkProgram(forward_shading_prog, shaders);
}

GLuint random_normal_tex;
GLuint random_scalar_tex;
void initNoise() 
{ 

#ifdef WIN32
	const char * rand_norm_png = "../../../res/random_normal.png";
	const char * rand_png = "../../../res/random.png";
#else
	const char * rand_norm_png = "../res/random_normal.png";
	const char * rand_png = "../res/random.png";
#endif
	random_normal_tex = (unsigned int)SOIL_load_OGL_texture(rand_norm_png,0,0,0);
    glBindTexture(GL_TEXTURE_2D, random_normal_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);

	random_scalar_tex = (unsigned int)SOIL_load_OGL_texture(rand_png,0,0,0);
    glBindTexture(GL_TEXTURE_2D, random_scalar_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void setTextures() 
{
    glBindTexture(GL_TEXTURE_2D,0); 
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //glColorMask(true,true,true,true);
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);
}

glm::mat4 get_mesh_world() 
{
    glm::vec3 tilt(1.0f,0.0f,0.0f);
    //glm::mat4 translate_mat = glm::translate(glm::vec3(0.0f,.5f,0.0f));
    glm::mat4 tilt_mat = glm::rotate(glm::mat4(), 90.0f, tilt);
    glm::mat4 scale_mat = glm::scale(glm::mat4(), glm::vec3(0.01));
	return glm::mat4(1.0);
    //return tilt_mat * scale_mat; //translate_mat;
}

int lightIdx = 0;

void draw_mesh_forward () 
{
    glUseProgram(forward_shading_prog);

    glm::mat4 model = get_mesh_world();
	glm::mat4 view,lview, persp, lpersp;

	view = context->pCam.get_view(); // Camera view Matrix
	//view = glm::gtx::transform2::lookAt(glm::vec3(0, 10, -10), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	persp = glm::perspective(45.0f, (float)context->viewport.x / (float)context->viewport.y, NEARP, FARP);
	lpersp = glm::perspective(120.0f, (float)context->viewport.x / (float)context->viewport.y, NEARP, FARP);

	//    persp = perspective(45.0f,(float)width/(float)height,NEARP,FARP);
    glm::mat4 inverse_transposed = glm::transpose(glm::inverse(view*model));
	glm::mat4 view_inverse = glm::inverse (view);

	GLuint modelMatLoc = glGetUniformLocation(forward_shading_prog, "u_ModelMat");
	GLuint viewMatLoc = glGetUniformLocation(forward_shading_prog, "u_ViewMat");
	GLuint perspMatLoc = glGetUniformLocation(forward_shading_prog, "u_PerspMat");
	GLuint vplPosLoc = glGetUniformLocation(forward_shading_prog, "u_vplPosition");
	GLuint vplIntLoc = glGetUniformLocation(forward_shading_prog, "u_vplIntensity");
	GLuint vplDirLoc = glGetUniformLocation(forward_shading_prog, "u_vplDirection");
	GLuint numLightsLoc = glGetUniformLocation(forward_shading_prog, "u_numLights");
	GLuint ambiColorLoc = glGetUniformLocation(forward_shading_prog, "u_AmbientColor");
	GLuint diffColorLoc = glGetUniformLocation(forward_shading_prog, "u_DiffuseColor");
    
	//glUniform1i(numLightsLoc, context->VPLs.size());
	glUniform1i(numLightsLoc, 1);
	/*
	if (indirectON)
		glUniform1i(glGetUniformLocation(forward_shading_prog, "u_numVPLs"), nVPLs);
	else
		glUniform1i(glGetUniformLocation(forward_shading_prog, "u_numVPLs"), 0);
	*/
	{
		using namespace glm::gtc::type_ptr;

		glUniformMatrix4fv(modelMatLoc, 1, GL_FALSE, value_ptr(model));
		glUniformMatrix4fv(viewMatLoc, 1, GL_FALSE, value_ptr(view));
		glUniformMatrix4fv(perspMatLoc, 1, GL_FALSE, value_ptr(persp));
		glUniform3fv(vplPosLoc, 1, value_ptr(context->VPLs[lightIdx].position));
		glUniform3fv(vplIntLoc, 1, value_ptr(context->VPLs[lightIdx].intensity));
		glUniform3fv(vplDirLoc, 1, value_ptr(context->VPLs[lightIdx].direction));

		for (int i = 0; i < context->drawMeshes.size(); i++)
		{
			glUniform3fv(diffColorLoc, 1, value_ptr(context->drawMeshes[i].diffuseColor));
			glUniform3fv(ambiColorLoc, 1, value_ptr(context->drawMeshes[i].ambientColor));
			glBindVertexArray(context->drawMeshes[i].vertex_array);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, context->drawMeshes[i].vbo_indices);

			glDrawElements(GL_TRIANGLES, context->drawMeshes[i].num_indices, GL_UNSIGNED_SHORT, 0);
		}
	}
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
}

enum Display display_type = DISPLAY_TOTAL;
void draw_quad() 
{
    glBindVertexArray(device_quad.vertex_array);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, device_quad.vbo_indices);

    glDrawElements(GL_TRIANGLES, device_quad.num_indices, GL_UNSIGNED_SHORT,0);

    glBindVertexArray(0);
}

void updateDisplayText(char * disp) 
{
    switch(display_type) 
	{
        case(DISPLAY_DEPTH):
            sprintf(disp, "Displaying Depth");
            break; 
        case(DISPLAY_NORMAL):
            sprintf(disp, "Displaying Normal");
            break; 
        case(DISPLAY_COLOR):
            sprintf(disp, "Displaying Color");
            break;
        case(DISPLAY_POSITION):
            sprintf(disp, "Displaying Position");
            break;
        case(DISPLAY_TOTAL):
            sprintf(disp, "Displaying Diffuse");
            break;
        case(DISPLAY_LIGHTS):
            sprintf(disp, "Displaying Lights");
            break;
		case DISPLAY_GLOWMASK:
			sprintf(disp, "Displaying Glow Mask");
			break;
		case(DISPLAY_SHADOW):
            sprintf(disp, "Displaying ShadowMap");
            break;
    }
}

int frame = 0;
int currenttime = 0;
int timebase = 0;
char title[1024];
char disp[1024];
char occl[1024];
void updateTitle() 
{
    updateDisplayText(disp);
    //calculate the frames per second
    frame++;

    //get the current time
    currenttime = glutGet(GLUT_ELAPSED_TIME);

    //check if a second has passed
    if (currenttime - timebase > 1000) 
    {
		if (forwardR)
			strcat (disp, " Forward Rendering");
		else
			strcat (disp, " Deferred Rendering");

		if (indirectON)
			strcat (disp, " GI On");
		else
			strcat (disp, " No GI");

		if (DOFEnabled)
			strcat (disp, " DOF On");

        sprintf(title, "CIS565 OpenGL Frame | %s FPS: %4.2f", disp, frame*1000.0/(currenttime-timebase));
        //sprintf(title, "CIS565 OpenGL Frame | %4.2f FPS", frame*1000.0/(currenttime-timebase));
        glutSetWindowTitle(title);
        timebase = currenttime;		
        frame = 0;
    }
}

bool doIScissor = true;

void RenderForward ()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

	draw_mesh_forward ();
}

void display(void)
{
	if (forwardR)
		RenderForward ();

    updateTitle();

    glutPostRedisplay();
    glutSwapBuffers();
}

void reshape(int w, int h)
{
    context->viewport.x = w;
    context->viewport.y = h;
    glBindFramebuffer(GL_FRAMEBUFFER,0);
    glViewport(0,0,(GLsizei)w,(GLsizei)h);
}

void mouse(int button, int state, int x, int y)
{
    if (state == GLUT_DOWN) 
	{
        mouse_buttons |= 1<<button;
    } 
	else if (state == GLUT_UP) 
	{
        mouse_buttons = 0;
    }

    mouse_old_x = x;
    mouse_old_y = y;

	if (button == GLUT_RIGHT_BUTTON)
	{
		mouse_dof_x = mouse_old_x;
		mouse_dof_y = mouse_old_y;
	}
}

void motion(int x, int y)
{
    float dx, dy;
    dx = -(float)(x - mouse_old_x);
    dy = (float)(y - mouse_old_y);
	float sensitivity = 0.001f;

    if (mouse_buttons & 1<<GLUT_RIGHT_BUTTON) 
	{
		//context->pCam.adjust(0,0,dx,0,0,0);;
    }
    else 
	{
		context->pCam.rotate(glm::vec3(dy * sensitivity, 0, dx * sensitivity));
    }

    mouse_old_x = x;
    mouse_old_y = y;
}

void keyboard(unsigned char key, int x, int y) 
{
    float tx = 0;
    float ty = 0;
    float tz = 0;
	float speed = 10.f;
    switch(key) 
	{
        case(27):
            std::exit(0);
            break;
		case('w') :
			tz = speed;
            break;
		case('s') :
			tz = -speed;
            break;
		case('d') :
			tx = -speed;
            break;
		case('a') :
			tx = speed;
            break;
		case('q') :
			ty = speed;
            break;
		case('z') :
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
        case('x'):
            doIScissor ^= true;
            break;
        case('r'):
            initShader();
			break;
		case('7'):
            display_type = DISPLAY_SHADOW;
			break;
		case 'f':
		case 'F':
			forwardR = !forwardR;
			break;
		case 'G':
		case 'g':
			indirectON = !indirectON;
			break;
		case ' ':
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

void initVPL ()
{
	// TODO
}

int main (int argc, char* argv[])
{
	context = SystemContext::initialize(
		Camera(glm::vec3(300, 300, -500),
			glm::normalize(glm::vec3(0, 0, 1)),
			glm::normalize(glm::vec3(0, 1, 0))),
		glm::uvec2(1280, 720)
		);

    bool loadedScene = false;

	if (argc > 1) {
		try {
			context->loadObj(argv[1]);
		}
		catch(const std::exception &e) {
			std::cerr << e.what();
			return EXIT_FAILURE;
		}
	}
	else
	{
		std::cerr << utility::sprintfpp("Usage: %s mesh=[obj file]\n", argv[0]);
        return 0;
    }

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	context->pCam.set_perspective
		(
		glm::perspective
			(
			45.0f,
			(float)context->viewport.x/(float)context->viewport.y,
			NEARP,
			FARP
			)
		);
	glutInitWindowSize(context->viewport.x, context->viewport.y);
    glutCreateWindow("CIS565 OpenGL Frame");
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        /* Problem: glewInit failed, something is seriously wrong. */
        std::cout << "glewInit failed, aborting." << std::endl;
        exit (1);
    }

	if (!GLEW_VERSION_3_3)
		if (!GLEW_ARB_compute_shader)
		{
			std::cout << "This program requires OpenGL 3.3 class graphics card." << std::endl
				 << "Press any key to terminate...";
			std::cin.get ();
			exit (1);
		}
    std::cout << "Status: Using GLEW " << glewGetString(GLEW_VERSION) << std::endl;
    std::cout << "OpenGL version " << glGetString(GL_VERSION) << " supported" << std::endl;

    //initNoise();
    initShader();
    init();
    //initMesh();
	context->initMesh();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);	
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);

    glutMainLoop();
    return EXIT_SUCCESS;
}