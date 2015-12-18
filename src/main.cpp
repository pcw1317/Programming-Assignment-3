#include "main.h"
#include "save_image.h"

#include <iostream>
#include <string>
#include <list>
#include <random>
#include <ctime>
#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform2.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/verbose_operator.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "utility.h"
#include "system_context.h"
#include "device_mesh.h"
#include "gl_snippets.h"
#include "raytracer.h"

#include "IlluminationCut.h"
#include <vector>

#define IC_ON	true


int mouse_buttons = 0;
int mouse_old_x = 0;
int mouse_old_y = 0;

system_context *context;
LightTree *light_tree;
PointTree *point_tree;

glm::mat4 get_global_mesh_world()
{
    return glm::mat4( 1.0 );
}

void save_to_bmp(){
	int w = 1280, h = 720;

	unsigned char* pixels = new unsigned char[3 * w * h];
	glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels);

	FILE *f;
	unsigned char *img = NULL;
	int filesize = 54 + 3 * 1280 * 720;  //w is your image width, h is image height, both int
	if (img)
	free(img);
	img = (unsigned char *)malloc(3 * w * h);
	memset(img, 0, sizeof(img));

	for (int i = 0; i<w; i++)
	{
		for (int j = 0; j<h; j++)
		{
			/*int x = i; int y = (h - 1) - j;
			unsigned char r = pixels[w*j+i];
			unsigned char g = pixels[w*j+i+1];
			unsigned char b = pixels[w*j+i+2];
			if (r > 255) r = 255;
			if (g > 255) g = 255;
			if (b > 255) b = 255;
			img[(x + y*w) * 3 + 2] = (unsigned char)(r);
			img[(x + y*w) * 3 + 1] = (unsigned char)(g);
			img[(x + y*w) * 3 + 0] = (unsigned char)(b);*/
			unsigned char r = pixels[w*j + i];
			unsigned char g = pixels[w*j + i + 1];
			unsigned char b = pixels[w*j + i + 2];
			img[(i + j*w) * 3 + 0] = (unsigned char)(r);
			img[(i + j*w) * 3 + 1] = (unsigned char)(g);
			img[(i + j*w) * 3 + 2] = (unsigned char)(b);
		}
	}

	unsigned char bmpfileheader[14] = { 'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0 };
	unsigned char bmpinfoheader[40] = { 40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 24,0 };
	unsigned char bmppad[3] = { 0,0,0 };

	bmpfileheader[2] = (unsigned char)(filesize);
	bmpfileheader[3] = (unsigned char)(filesize >> 8);
	bmpfileheader[4] = (unsigned char)(filesize >> 16);
	bmpfileheader[5] = (unsigned char)(filesize >> 24);

	bmpinfoheader[4] = (unsigned char)(w);
	bmpinfoheader[5] = (unsigned char)(w >> 8);
	bmpinfoheader[6] = (unsigned char)(w >> 16);
	bmpinfoheader[7] = (unsigned char)(w >> 24);
	bmpinfoheader[8] = (unsigned char)(h);
	bmpinfoheader[9] = (unsigned char)(h >> 8);
	bmpinfoheader[10] = (unsigned char)(h >> 16);
	bmpinfoheader[11] = (unsigned char)(h >> 24);

	f = fopen("img.bmp", "wb");
	fwrite(bmpfileheader, 1, 14, f);
	fwrite(bmpinfoheader, 1, 40, f);
	for (int i = 0; i<h; i++)
	{
		//fwrite(img + (w*(h - i - 1) * 3), 3, w, f);
		fwrite(img + 3*w*i, 3, w, f);
		fwrite(bmppad, 1, (4 - (w * 3) % 4) % 4, f);
	}
	fclose(f);
}

bool isIllumAwarePair(PointNode* point_cluster, LightNode* light_cluster) {
	const float delta = 0.01;
	float max = std::numeric_limits<float>::min();
	float rep;

	float rep_radiance = glm::length((*(light_cluster->vpls))[light_cluster->lights[light_cluster->rep_light]].intensity) * light_cluster->lights.size();

	for (int i = 0; i < point_cluster->points.size(); i++) {
		float rep_distance = glm::length(point_cluster->points[i] - (*(light_cluster->vpls))[light_cluster->lights[light_cluster->rep_light]].position);
		rep_distance *= rep_distance;
		float sum = 0.0;
		for (int j = 0; j < light_cluster->lights.size(); j++) {
			if (context->vpl_raytracer->raycast(point_cluster->points[i], (*(light_cluster->vpls))[light_cluster->lights[j]].position - point_cluster->points[i]) >= glm::length((*(light_cluster->vpls))[light_cluster->lights[j]].position - point_cluster->points[i])) {
				float distance = glm::length((*(light_cluster->vpls))[light_cluster->lights[j]].position - point_cluster->points[i]);
				distance *= distance;
				sum += glm::length((*(light_cluster->vpls))[light_cluster->lights[j]].intensity) / distance;
			}
		}
		rep = (context->vpl_raytracer->raycast(point_cluster->points[i], (*(light_cluster->vpls))[light_cluster->lights[light_cluster->rep_light]].position - point_cluster->points[i])) >= rep_distance ? rep_radiance/rep_distance : 0.0;
		max = abs(rep - sum) > max ? abs(rep - sum) : max;
	}

	if (max < delta*glm::length((*light_cluster->vpls)[light_cluster->lights[light_cluster->min_light]].intensity)) {
		//printf("true\n");
		return true;
	}
	else {
		//printf("false\n");
		return false;
	}
}

void update_title()
{
    //FPS calculation
    static unsigned long frame = 0;
    static double time_base = glfwGetTime();
    ++frame;
    double time_now = glfwGetTime();

    if( time_now - time_base > 1.0 ) //update title if a second passes
    {

        std::string title = utility::sprintfpp( "CS482 Instant Radiosity | FPS: %4.2f",
                                                frame / ( time_now - time_base )
                                              );

        glfwSetWindowTitle( context->window, title.c_str() );
        //reset per-second frame statistics for next update
        time_base = time_now;
        frame = 0;
    }
}


//from https://github.com/cforfang/opengl-shadowmapping/blob/master/src/vsmcube/main.cpp
std::pair<glm::mat4, glm::mat4> get_shadow_matrices(glm::vec3 light_pos, int dir)
{
	glm::mat4 v, p;
	p = glm::perspective(90.0f, 1.0f, 0.1f, 1000.0f);
	switch (dir)
	{
	case 0:
		// +X
		v = glm::lookAt(light_pos, light_pos + glm::vec3(+1, +0, 0), glm::vec3(0, -1, 0));
		p *= v;
		break;
	case 1:
		// -X
		v = glm::lookAt(light_pos, light_pos + glm::vec3(-1, +0, 0), glm::vec3(0, -1, 0));
		p *= v;
		break;
	case 2:
		// +Y
		v = glm::lookAt(light_pos, light_pos + glm::vec3(0, +1, 0), glm::vec3(0, 0, -1));
		p *= v;
		break;
	case 3:
		// -Y
		v = glm::lookAt(light_pos, light_pos + glm::vec3(0, -1, 0), glm::vec3(0, 0, -1));
		p *= v;
		break;
	case 4:
		// +Z
		v = glm::lookAt(light_pos, light_pos + glm::vec3(0, 0, +1), glm::vec3(0, -1, 0));
		p *= v;
		break;
	case 5:
		// -Z
		v = glm::lookAt(light_pos, light_pos + glm::vec3(0, 0, -1), glm::vec3(0, -1, 0));
		p *= v;
		break;
	default:
		// Do nothing
		break;
	}
	return std::make_pair(v, p);
}


void perlight_generate_shadow_map( int light_index )
{
    glDisable( GL_BLEND );
    glEnable( GL_DEPTH_TEST );
    context->gls_programs[kGlsProgramShadowMapping].bind();
    context->gls_cubemap_framebuffers[kGlsCubemapFramebufferShadow].set_viewport();

    for( int i = 0; i < 6; ++i )
    {
        context->gls_cubemap_framebuffers[kGlsCubemapFramebufferShadow].bind( i );
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
        auto vp = get_shadow_matrices( context->vpls[light_index].position, i );
        context->gls_programs[kGlsProgramShadowMapping].set_uniforms(
            //"u_model", "u_cameraToShadowView" , "u_cameraToShadowProjector"
            gls::no_change,
            vp.first,
            vp.second );
        for( int i = 0; i < context->scene_meshes.size(); i++ )
        {
            context->scene_meshes[i].draw();
        }
    }
};

void perlight_draw(int light_index, int count)
{
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	context->gls_programs[kGlsProgramSceneDraw].bind();
	context->gls_programs[kGlsProgramSceneDraw].set_uniforms(
		//"u_modelMat", "u_viewMat" , "u_perspMat", "u_vplPosition", "u_vplIntensity", "u_vplDirection", "u_numLights", "u_ambientColor", "u_diffuseColor", "u_shadowTex"
		gls::no_change,
		gls::no_change,
		gls::no_change,
		context->vpls[light_index].position,
		context->vpls[light_index].intensity * (float)count,
		context->vpls[light_index].direction,
		gls::no_change,
		gls::no_change,
		gls::no_change,
		gls::no_change);
	context->gls_framebuffers[kGlsFramebufferSceneDraw].bind();
	context->gls_framebuffers[kGlsFramebufferSceneDraw].set_viewport();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glActiveTexture(GL_TEXTURE0);
	context->gls_cubemap_framebuffers[kGlsCubemapFramebufferShadow].get_color_map().bind();

	for (int i = 0; i < context->scene_meshes.size(); i++)
	{
		context->gls_programs[kGlsProgramSceneDraw].set_uniforms(
			//"u_modelMat", "u_viewMat" , "u_perspMat", "u_vplPosition", "u_vplIntensity", "u_vplDirection", "u_numLights", "u_ambientColor", "u_diffuseColor", "u_shadowTex"
			gls::no_change,
			gls::no_change,
			gls::no_change,
			gls::no_change,
			gls::no_change,
			gls::no_change,
			gls::no_change,
			context->scene_meshes[i].ambient_color,
			context->scene_meshes[i].diffuse_color,
			gls::no_change);
		context->scene_meshes[i].draw();
	}
};

void perlight_accumulate( int light_index )
{
    glEnable( GL_BLEND );
    glDisable( GL_DEPTH_TEST );
    glBlendFunc( GL_ONE, GL_ONE ); //perform additive blending

    context->gls_programs[kGlsProgramQuadDraw].bind();
    context->gls_framebuffers[kGlsFramebufferAccumulate].bind();
    context->gls_framebuffers[kGlsFramebufferAccumulate].set_viewport();
    glActiveTexture( GL_TEXTURE0 );
    context->gls_framebuffers[kGlsFramebufferSceneDraw].get_color_map().bind();
    context->quad_mesh.draw();
};

void perlight_accumulate2(int light_index)
{
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_ONE, GL_ONE); //perform additive blending

	context->gls_programs[kGlsProgramQuadDraw].bind();
	context->gls_framebuffers[kGlsFramebufferIlluminationCut].bind();
	context->gls_framebuffers[kGlsFramebufferIlluminationCut].set_viewport();
	glActiveTexture(GL_TEXTURE0);
	context->gls_framebuffers[kGlsFramebufferSceneDraw].get_color_map().bind();
	context->quad_mesh.draw();
};

void visualize_accumulation(gls_framebuffer_t mode)
{
    glDisable( GL_BLEND );
    glDisable( GL_DEPTH_TEST );

    context->gls_programs[kGlsProgramQuadDraw].bind();
    context->gls_framebuffers[kGlsFramebufferScreen].bind();
    context->gls_framebuffers[kGlsFramebufferScreen].set_viewport();
    glActiveTexture( GL_TEXTURE0 );
    context->gls_framebuffers[mode].get_color_map().bind();
    context->quad_mesh.draw();
};

void render()
{
	//if (IC_ON) {
	//	//clear illuminationcut framebuffer first
	//	context->gls_framebuffers[kGlsFramebufferIlluminationCut].bind();
	//	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//}
	//else {
	//	//clear accumulation framebuffer first
	//	context->gls_framebuffers[kGlsFramebufferAccumulate].bind();
	//	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//}
	context->gls_framebuffers[kGlsFramebufferAccumulate].bind();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 model = get_global_mesh_world(); //assume that model matrix is constant across models!
    glm::mat4 view = context->camera.get_view();
    glm::mat4 perspective = context->camera.get_perspective();

    //set global parameters
    context->gls_programs[kGlsProgramSceneDraw].bind();
    context->gls_programs[kGlsProgramSceneDraw].set_uniforms(
        //"u_modelMat", "u_viewMat" , "u_perspMat", "u_vplPosition", "u_vplIntensity", "u_vplDirection", "u_numLights", "u_ambientColor", "u_diffuseColor", "u_shadowTex"
        model,
        view,
        perspective,
        gls::no_change,
        gls::no_change,
        gls::no_change,
        context->shown_vpl_index == context->vpls.size() ? int( context->vpls.size() ) : 1,
        gls::no_change,
        gls::no_change,
        0 );
    context->gls_programs[kGlsProgramQuadDraw].bind();
    context->gls_programs[kGlsProgramQuadDraw].set_uniforms(
        //"u_Tex"
        0 );
    context->gls_programs[kGlsProgramShadowMapping].bind();
    context->gls_programs[kGlsProgramShadowMapping].set_uniforms(
        //"u_model", "u_cameraToShadowView" , "u_cameraToShadowProjector"
        model,
        gls::no_change,
        gls::no_change );

	if (IC_ON) {
		if (context->shown_vpl_index == context->vpls.size())
		{
			std::vector<PointNode*>point_stack = std::vector<PointNode*>();
			std::vector<LightNode*>light_stack = std::vector<LightNode*>();

			point_stack.push_back(point_tree->root);
			light_stack.push_back(light_tree->root);

			while (point_stack.size() > 0) {
				PointNode* point_cluster = point_stack.back();
				LightNode* light_cluster = light_stack.back();
				point_stack.pop_back();
				light_stack.pop_back();
				if (isIllumAwarePair(point_cluster, light_cluster)) {
					perlight_generate_shadow_map(light_cluster->lights[light_cluster->rep_light]);
					perlight_draw(light_cluster->lights[light_cluster->rep_light], light_cluster->lights.size());
					perlight_accumulate(light_cluster->lights[light_cluster->rep_light]);
				}
				else {
					if (light_cluster->radius > point_cluster->radius) {
						if (light_cluster->left_child == NULL && light_cluster->right_child == NULL) {
							perlight_generate_shadow_map(light_cluster->lights[light_cluster->rep_light]);
							perlight_draw(light_cluster->lights[light_cluster->rep_light], light_cluster->lights.size());
							perlight_accumulate(light_cluster->lights[light_cluster->rep_light]);
						}
						else {
							point_stack.push_back(point_cluster);
							light_stack.push_back(light_cluster->left_child);
							point_stack.push_back(point_cluster);
							light_stack.push_back(light_cluster->right_child);
						}
					}
					else {
						bool has_child = false;
						for (int i = 0; i < 8; i++)
							if (point_cluster->children[i] != NULL) {
								has_child = true;
								break;
							}
						if (!has_child) {
							perlight_generate_shadow_map(light_cluster->lights[light_cluster->rep_light]);
							perlight_draw(light_cluster->lights[light_cluster->rep_light], light_cluster->lights.size());
							perlight_accumulate(light_cluster->lights[light_cluster->rep_light]);
						}
						else {
							for (int i = 0; i < 8; i++) {
								if (point_cluster->children[i] != NULL) {
									point_stack.push_back(point_cluster->children[i]);
									light_stack.push_back(light_cluster);
								}
							}
						}
					}
				}
			}
		}
		else
		{
			perlight_generate_shadow_map(context->shown_vpl_index);
			perlight_draw(context->shown_vpl_index, 1);
			perlight_accumulate(context->shown_vpl_index);
		}
		visualize_accumulation(kGlsFramebufferAccumulate);
	}
	else {
		if (context->shown_vpl_index == context->vpls.size())
		{
			for (int light_index = 0; light_index < context->vpls.size(); ++light_index)
			{
				perlight_generate_shadow_map(light_index);
				perlight_draw(light_index, 1);
				perlight_accumulate(light_index);
			}
		}
		else
		{
			perlight_generate_shadow_map(context->shown_vpl_index);
			perlight_draw(context->shown_vpl_index, 1);
			perlight_accumulate(context->shown_vpl_index);
		}
		visualize_accumulation(kGlsFramebufferAccumulate);
	}
}

void window_callback_mouse_button( GLFWwindow *window, int button, int action, int mods )
{
    if( action == GLFW_PRESS )
    {
        mouse_buttons |= 1 << button;
    }
    else if( action == GLFW_RELEASE )
    {
        mouse_buttons = 0;
    }
    {
        double x, y;
        glfwGetCursorPos( window, &x, &y );

        mouse_old_x = int( x );
        mouse_old_y = int( y );
    }
}

void window_callback_cursor_pos( GLFWwindow *window, double x, double y )
{
    float dx, dy;
    dx = -( float )( x - mouse_old_x );
    dy = ( float )( y - mouse_old_y );
    float sensitivity = 0.001f;

    if( mouse_buttons & 1 << GLFW_MOUSE_BUTTON_LEFT )
    {
        context->camera.rotate( glm::vec3( dy * sensitivity, 0, dx * sensitivity ) );
    }

    mouse_old_x = int( x );
    mouse_old_y = int( y );
}

void window_callback_key( GLFWwindow *window, int key, int scancode, int action, int mods )
{
    float tx = 0;
    float ty = 0;
    float tz = 0;
    if( action == GLFW_RELEASE ) //no need to process key up events
        return;
    float speed = 10.f;
    switch( key )
    {
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose( window, true );
        break;
    case( 'W' ) :
        tz = speed;
        break;
    case( 'S' ) :
        tz = -speed;
        break;
    case( 'D' ) :
        tx = -speed;
        break;
    case( 'A' ) :
        tx = speed;
        break;
    case( 'Q' ) :
        ty = speed;
        break;
    case( 'Z' ) :
        ty = -speed;
        break;
    case( ' ' ):
        context->shown_vpl_index = ( context->shown_vpl_index + 1 ) % ( context->vpls.size() + 1 );
        break;
    }

    if( abs( tx ) > 0 ||  abs( tz ) > 0 || abs( ty ) > 0 )
    {
        context->camera.translate( glm::vec3( tx, ty, tz ) );
    }
}

device_mesh_t init_quad_mesh()
{
    std::vector<glm::vec3> positions
    {
        glm::vec3( -1, 1, 0 ),
        glm::vec3( -1, -1, 0 ),
        glm::vec3( 1, -1, 0 ),
        glm::vec3( 1, 1, 0 )
    };

    std::vector<glm::vec3> normals
    {
        glm::vec3( 0, 0, 0 ),
        glm::vec3( 0, 0, 0 ),
        glm::vec3( 0, 0, 0 ),
        glm::vec3( 0, 0, 0 )
    };

    std::vector<glm::vec2> texcoords
    {
        glm::vec2( 0, 1 ),
        glm::vec2( 0, 0 ),
        glm::vec2( 1, 0 ),
        glm::vec2( 1, 1 )
    };

    std::vector<unsigned short> indices{ 0, 1, 2, 0, 2, 3 };

    host_mesh_t hm( positions, normals, texcoords, indices, "", glm::vec3( 0, 0, 0 ), glm::vec3( 0, 0, 0 ) );
    return device_mesh_t( hm );
}

void init()
{
    //GL parameter initialization
    glEnable( GL_TEXTURE_CUBE_MAP_SEAMLESS );
    glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );

    //context gls object initialization
    context->gls_programs.resize( kGlsProgramMax );
    context->gls_buffers.resize( kGlsBufferMax );
    context->gls_vertex_arrays.resize( kGlsVertexArrayMax );
    context->gls_framebuffers.resize( kGlsFramebufferMax );
    context->gls_cubemap_framebuffers.resize( kGlsCubemapFramebufferMax );

    //shaders
    context->gls_programs[kGlsProgramSceneDraw] = gls::program( kProgramSceneDraw );
    context->gls_programs[kGlsProgramQuadDraw] = gls::program( kProgramQuadDraw );
    context->gls_programs[kGlsProgramShadowMapping] = gls::program( kProgramShadowMapping );

    //framebuffers
    context->gls_framebuffers[kGlsFramebufferScreen] = gls::framebuffer<gls::texture, gls::texture>( context->viewport.x, context->viewport.y, true ); //default screen framebuffer
    context->gls_framebuffers[kGlsFramebufferSceneDraw] = gls::framebuffer<gls::texture, gls::texture>( context->viewport.x, context->viewport.y );
    context->gls_framebuffers[kGlsFramebufferAccumulate] = gls::framebuffer<gls::texture, gls::texture>( context->viewport.x, context->viewport.y );
	context->gls_framebuffers[kGlsFramebufferIlluminationCut] = gls::framebuffer<gls::texture, gls::texture>(context->viewport.x, context->viewport.y);

    //cubemap framebuffers
    context->gls_cubemap_framebuffers[kGlsCubemapFramebufferShadow] = gls::cubemap_framebuffer<gls::texture, gls::texture>( kShadowSize ); //default screen framebuffer

    //quad geometry; used for various texture-to-texture operations
    context->quad_mesh = std::move( init_quad_mesh() );
}

namespace
{
//opengl initialization: GLFW, GLEW and our application window
class opengl_initializer_t
{
public:
    opengl_initializer_t();
    opengl_initializer_t( const opengl_initializer_t & ) = delete;
    opengl_initializer_t &operator=( const opengl_initializer_t & ) = delete;
    ~opengl_initializer_t();
    opengl_initializer_t( opengl_initializer_t && ) = delete;
    opengl_initializer_t &operator=( opengl_initializer_t && ) = delete;
};

opengl_initializer_t::opengl_initializer_t()
{
    //initialize glfw
    if( !glfwInit() )
        throw std::runtime_error( "glfwInit() failed" );

    try
    {
        //create window
        glfwWindowHint( GLFW_RESIZABLE, GL_FALSE );
        if( !( context->window = glfwCreateWindow( context->viewport.x, context->viewport.y, "InstantRadiosity", NULL, NULL ) ) )
            throw std::runtime_error( "glfw window creation failed" );


        glfwMakeContextCurrent( context->window );

        //set callbacks
        glfwSetKeyCallback( context->window, window_callback_key );
        glfwSetCursorPosCallback( context->window, window_callback_cursor_pos );
        glfwSetMouseButtonCallback( context->window, window_callback_mouse_button );

        //initialize glew
        glewExperimental = GL_TRUE;
        if( glewInit() != GLEW_OK )
            throw std::runtime_error( "glewInit() failed" );

        //check version requirement
        //TODO: update correct requirement later
        if( !GLEW_VERSION_3_3 )
            throw std::runtime_error( "This program requires OpenGL 3.3 class graphics card." );
        else
        {
            std::cerr << "Status: Using GLEW " << glewGetString( GLEW_VERSION ) << std::endl;
            std::cerr << "OpenGL version " << glGetString( GL_VERSION ) << " supported" << std::endl;
        }
    }
    catch( ... )
    {
        glfwTerminate();
        throw;
    }
}
opengl_initializer_t::~opengl_initializer_t()
{
    glfwTerminate();
}
}
int main( int argc, char *argv[] )
{
	std::clock_t start;
	start = std::clock();

    //Step 0: Initialize our system context
    {
        glm::uvec2 viewport( 1280, 720 );
        camera_t default_camera(
            glm::vec3( 300, 300, -500 ),
            glm::vec3( 0, 0, 1 ),
            glm::vec3( 0, 1, 0 ),
            glm::perspective(
                45.0f,
                float( viewport.x ) / float( viewport.y ),
                kNearPlane,
                kFarPlane
            )
        );
        context = system_context::initialize( default_camera, viewport );
    }

    //Step 1: Initialize GLFW & GLEW
    //RAII initialization of GLFW, GLEW and our application window
    std::unique_ptr<opengl_initializer_t> opengl_initializer;
    try
    {
        opengl_initializer.reset( new opengl_initializer_t );
    }
    catch( const std::exception &e )
    {
        std::cerr << "OpenGL initialization failed. Reason: " << e.what() << "\nAborting.\n";
        return EXIT_FAILURE;
    }

    //Step 2: Load mesh into memory & Construct tree structures for IlluminationCut
    if( argc > 1 )
    {
        try
        {
			point_tree = new PointTree();
            context->load_mesh( argv[1], point_tree);
        }
        catch( const std::exception &e )
        {
            std::cerr << "Mesh load failed. Reason: " << e.what() << "\nAborting.\n";
            return EXIT_FAILURE;
        }
		srand(time(NULL));
		light_tree = new LightTree(context->vpls);
		light_tree->cluster();

		point_tree->cluster();
    }
    else
    {
        std::cerr << utility::sprintfpp( "Usage: %s mesh=[obj file]\n", argv[0] );
        return EXIT_SUCCESS;
    }

    //Step 3: Initialize objects
    try
    {
        init();
    }
    catch( const std::exception &e )
    {
        std::cerr << "Object initialization failed. Reason: " << e.what() << "\nAborting.\n";
        return EXIT_FAILURE;
    }

	std::cout << "Time: " << (std::clock() - start) / (double)(CLOCKS_PER_SEC / 1000) << " ms" << std::endl;
    //Step 4: Main loop
    while( !glfwWindowShouldClose( context->window ) )
    {
        render();
        update_title();

        glfwSwapBuffers( context->window );

		/*int width = 1280, height = 720;
		BYTE* pixels = new BYTE[3 * width * height];
		BYTE* buf = new BYTE[3 * width * height];

		glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);
		
		unsigned long imageSize = 0;
		unsigned long padding = 0;

		int c = 0;
		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				buf[c + 0] = pixels[width*i + j];
				buf[c + 1] = pixels[width*i + j + 1];
				buf[c + 2] = pixels[width*i + j + 2];

				c += 3;
			}
		}

		// Use the new array data to create the new bitmap file
		SaveBitmapToFile((BYTE*)buf,
			width,
			height,
			24,
			0,
			"img.bmp");
		*/
        glfwPollEvents();
		
    }

    return EXIT_SUCCESS;
}