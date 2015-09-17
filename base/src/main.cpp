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

#include <iostream>
#include <string>
#include <list>
#include <random>
#include <ctime>
#include <algorithm>

#include "InstantRadiosity.h"

using namespace std;
using namespace glm;

const float PI = 3.14159f;
const int MAX_LIGHTS_PER_TILE = 64;

bool forwardR = true, indirectON = false, DOFEnabled = false, DOFDebug = false;

int mouse_buttons = 0;
int mouse_old_x = 0, mouse_dof_x = 0;
int mouse_old_y = 0, mouse_dof_y = 0;

int nVPLs = 512;
int nLights = 0;
int nBounces = 1;

float FARP = 100.f;
float NEARP = 0.1f;

std::list<LightData> lightList;
std::default_random_engine random_gen (time (NULL));

SystemContext *context;

mat4x4 get_mesh_world ();

void initMesh() 
{
	mat4 modelMatrix = get_mesh_world ();
    for(vector<tinyobj::shape_t>::iterator it = context->shapesBeginIter();
            it != context->shapesEndIter(); ++it)
    {
        tinyobj::shape_t shape = *it;
		if (shape.material.name == "light")
		{
			LightData	new_light;
			new_light.position = vec3 (3.5, -2.5, 4.5);//(mesh.vertices [0] + mesh.vertices [1] + mesh.vertices [2]) / 3.0f; 2.5, -2.5, 4.3) vec3 (3.5, -2.5, 2.0)
			new_light.intensity = vec3(1.0f);
			lightList.push_back (new_light);
		}
    }
	nLights = lightList.size ();
	if (nLights == 0)
	{
		LightData	new_light;
		new_light.position = vec3 (3.5, -2.0, 4.0);//(mesh.vertices [0] + mesh.vertices [1] + mesh.vertices [2]) / 3.0f; 2.5, -2.5, 4.3) vec3 (3.5, -2.5, 2.0)
		new_light.intensity = vec3(1.0f);
		lightList.push_back (new_light);
		++nLights;
	}

}

device_mesh2_t device_quad;
void initQuad()
{
    vertex2_t verts [] = {	{vec3(-1,1,0),vec2(0,1)},
							{vec3(-1,-1,0),vec2(0,0)},
							{vec3(1,-1,0),vec2(1,0)},
							{vec3(1,1,0),vec2(1,1)}		};

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
    glVertexAttribPointer(quad_attributes::TEXCOORD, 2, GL_FLOAT, GL_FALSE,sizeof(vertex2_t),(void*)sizeof(vec3));
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
	const char * pass_vert = "../../../res/shaders/pass.vert";
	
	// Forward shaders
	const char * forward_frag = "../../../res/shaders/forward_frag.glsl";
#else
	// Common shaders
	const char * pass_vert = "../res/shaders/pass.vert";

	// Forward shaders
	const char * forward_frag = "../res/shaders/forward_frag.glsl";
#endif

	Utility::shaders_t shaders = Utility::loadShaders(pass_vert, forward_frag);
	forward_shading_prog = glCreateProgram();
	glBindAttribLocation(forward_shading_prog, mesh_attributes::POSITION, "Position");
    glBindAttribLocation(forward_shading_prog, mesh_attributes::NORMAL, "Normal");
    glBindAttribLocation(forward_shading_prog, mesh_attributes::TEXCOORD, "Texcoord");
	Utility::attachAndLinkProgram(forward_shading_prog, shaders);
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

Light lig(vec3(3.5, -4.0, 5.3),
        normalize(vec3(0,0,-1.0)),
        normalize(vec3(0,1,0)));

mat4x4 get_mesh_world() 
{
    vec3 tilt(1.0f,0.0f,0.0f);
    //mat4 translate_mat = glm::translate(glm::vec3(0.0f,.5f,0.0f));
    mat4 tilt_mat = glm::rotate(mat4(), 90.0f, tilt);
    mat4 scale_mat = glm::scale(mat4(), vec3(0.01));
    return tilt_mat * scale_mat; //translate_mat;
}

void draw_mesh_forward () 
{
    glUseProgram(forward_shading_prog);

    mat4 model = get_mesh_world();
	mat4 view,lview, persp, lpersp;

	view = context->pCam->get_view(); // Camera view Matrix
	lview = lig.get_light_view();
	persp = perspective(45.0f, (float)context->viewport.x / (float)context->viewport.y, NEARP, FARP);
	lpersp = perspective(120.0f, (float)context->viewport.x / (float)context->viewport.y, NEARP, FARP);

	//    persp = perspective(45.0f,(float)width/(float)height,NEARP,FARP);
    mat4 inverse_transposed = transpose(inverse(view*model));
	mat4 view_inverse = inverse (view);

    glUniform1f(glGetUniformLocation(forward_shading_prog, "u_Far"), FARP);
	glUniform1f(glGetUniformLocation(forward_shading_prog, "u_Near"), NEARP);
    
	glUniform1i(glGetUniformLocation(forward_shading_prog, "u_numLights"), nLights);
	if (indirectON)
		glUniform1i(glGetUniformLocation(forward_shading_prog, "u_numVPLs"), nVPLs);
	else
		glUniform1i(glGetUniformLocation(forward_shading_prog, "u_numVPLs"), 0);

	glUniformMatrix4fv(glGetUniformLocation(forward_shading_prog,"u_Model"),1,GL_FALSE,&model[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(forward_shading_prog,"u_View"),1,GL_FALSE,&view[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(forward_shading_prog,"u_lView"),1,GL_FALSE,&lview[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(forward_shading_prog,"u_Persp"),1,GL_FALSE,&persp[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(forward_shading_prog,"u_LPersp"),1,GL_FALSE,&lpersp[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(forward_shading_prog,"u_InvTrans") ,1,GL_FALSE,&inverse_transposed[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(forward_shading_prog,"u_ViewInverse") ,1,GL_FALSE,&view_inverse[0][0]);

    for(int i=0; i<context->drawMeshes.size(); i++)
	{
        glUniform3fv(glGetUniformLocation(forward_shading_prog, "u_Color"), 1, &(context->drawMeshes[i].color[0]));
        glBindVertexArray(context->drawMeshes[i].vertex_array);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, context->drawMeshes[i].vbo_indices);
        
		glDrawElements(GL_TRIANGLES, context->drawMeshes[i].num_indices, GL_UNSIGNED_SHORT,0);
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
			sprintf (disp, "Displaying Glow Mask");
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
    dx = (float)(x - mouse_old_x);
    dy = (float)(y - mouse_old_y);

    if (mouse_buttons & 1<<GLUT_RIGHT_BUTTON) 
	{
		context->pCam->adjust(0,0,dx,0,0,0);;
    }
    else 
	{
		context->pCam->adjust(-dx*0.2f,-dy*0.2f,0,0,0,0);
    }

    mouse_old_x = x;
    mouse_old_y = y;
}

void keyboard(unsigned char key, int x, int y) 
{
    float tx = 0;
    float ty = 0;
    float tz = 0;
    switch(key) 
	{
        case(27):
            exit(0.0);
            break;
        case('w'):
            tz = 0.1;
            break;
        case('s'):
            tz = -0.1;
            break;
        case('d'):
            tx = -0.1;
            break;
        case('a'):
            tx = 0.1;
            break;
        case('q'):
            ty = 0.1;
            break;
        case('z'):
            ty = -0.1;
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
    }

    if (abs(tx) > 0 ||  abs(tz) > 0 || abs(ty) > 0) 
	{
		context->pCam->adjust(0,0,0,tx,ty,tz);
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
	context = new SystemContext();

    bool loadedScene = false;
    for(int i=1; i<argc; i++)
	{
		string err = context->LoadObj(argv[i]);
		if (!err.empty())
		{
			cerr << err << endl;
			return -1;
		}
		loadedScene = true;
    }

    if(!loadedScene)
	{
        cout << "Usage: mesh=[obj file]" << endl; 
        std::cin.ignore( std::numeric_limits<std::streamsize>::max(), '\n' );
        return 0;
    }

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	context->viewport.x = 1280;
	context->viewport.y = 720;
	context->pCam->set_perspective
		(
		perspective
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
        cout << "glewInit failed, aborting." << endl;
        exit (1);
    }

	// Make sure only OpenGL 4.3 or Direct3D 11 cards are allowed to run the program.
	if (!GLEW_VERSION_4_3)
		if (!GLEW_ARB_compute_shader)
		{
			cout << "This program requires either a Direct3D 11 or OpenGL 4.3 class graphics card." << endl
				 << "Press any key to terminate...";
			cin.get ();
			exit (1);
		}
    cout << "Status: Using GLEW " << glewGetString(GLEW_VERSION) << endl;
    cout << "OpenGL version " << glGetString(GL_VERSION) << " supported" << endl;

    initNoise();
    initShader();
    init();
    initMesh();
	context->initMesh();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);	
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);

    glutMainLoop();
    return 0;
}