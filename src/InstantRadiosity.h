#pragma once

#include <vector>
#include <random>
#include <glm/glm.hpp>
#include <embree2/rtcore.h>
#include <embree2/rtcore_ray.h>
#include "host_mesh.h"

struct LightData
{
    glm::vec3 position;
    glm::vec3 intensity;
    glm::vec3 direction;
};

struct AreaLightData
{
    glm::vec3 lightMin;
    glm::vec3 lightMax;
    glm::vec3 intensity;
    glm::vec3 direction;
};

class InstantRadiosityEmbree
{
    RTCScene scene;

public:
    InstantRadiosityEmbree()
    {
        randGen = std::default_random_engine();
        uniformReal = std::uniform_real_distribution<float>( -jitterEpsilon, jitterEpsilon );

        rtcInit( NULL );
        rtcSetErrorFunction( InstantRadiosityEmbree::error_handler );
        scene = rtcNewScene( RTC_SCENE_STATIC, RTC_INTERSECT1 );
    }

    ~InstantRadiosityEmbree()
    {
        rtcDeleteScene( scene );
        rtcExit();
    }

    void addMesh( host_mesh_t mesh );
    void commitScene()
    {
        rtcCommit( scene );
    }
    std::vector<LightData> getVPLpos( LightData light, unsigned int count, unsigned int recursionDepth );
    std::vector<LightData> getVPLpos( AreaLightData light, unsigned int sampleCount, unsigned int rayCount, unsigned int recursionDepth );

protected:
    std::vector<unsigned int> geomIDs;
    std::map<unsigned int, host_mesh_t> geomIDToMesh;
    std::default_random_engine randGen;
    std::uniform_real_distribution<float> uniformReal;
    const float jitterEpsilon = 0.2f;

    glm::vec3 stratifiedSampling( glm::vec3 normalVec, unsigned int current, unsigned int total );
    glm::vec3 stratifiedSampling( glm::vec3 bbMin, glm::vec3 bbMax, unsigned int current, unsigned int total );

    float radicalInverse_VdC( unsigned int bits );
    glm::vec2 hammersley2d( unsigned int i, unsigned int N );
    glm::vec2 hammersley2dJittered( unsigned int i, unsigned int N );

    static void error_handler( const RTCError code, const char *str )
    {
        printf( "Embree: " );
        switch( code )
        {
        case RTC_UNKNOWN_ERROR:
            printf( "RTC_UNKNOWN_ERROR" );
            break;
        case RTC_INVALID_ARGUMENT:
            printf( "RTC_INVALID_ARGUMENT" );
            break;
        case RTC_INVALID_OPERATION:
            printf( "RTC_INVALID_OPERATION" );
            break;
        case RTC_OUT_OF_MEMORY:
            printf( "RTC_OUT_OF_MEMORY" );
            break;
        case RTC_UNSUPPORTED_CPU:
            printf( "RTC_UNSUPPORTED_CPU" );
            break;
        case RTC_CANCELLED:
            printf( "RTC_CANCELLED" );
            break;
        default:
            printf( "invalid error code" );
            break;
        }
        if( str )
        {
            printf( " (" );
            while( *str ) putchar( *str++ );
            printf( ")\n" );
        }
        exit( 1 );
    }
};

glm::vec3 randDirHemisphere( glm::vec3 normal, float v1, float v2 );
