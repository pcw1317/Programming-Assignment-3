
typedef struct
{
    glm::vec3 pt;
    glm::vec2 texcoord;
} vertex2_t;


namespace quad_attributes
{

enum
{
    POSITION,
    TEXCOORD
};

}

typedef struct
{
    unsigned int vertex_array;
    unsigned int vbo_indices;
    unsigned int num_indices;
    //Don't need these to get it working, but needed for deallocation
    unsigned int vbo_data;
} device_mesh2_t;


device_mesh2_t device_quad;

void init_quad()
{
    vertex2_t verts[] = { { glm::vec3( -1, 1, 0 ), glm::vec2( 0, 1 ) },
        { glm::vec3( -1, -1, 0 ), glm::vec2( 0, 0 ) },
        { glm::vec3( 1, -1, 0 ), glm::vec2( 1, 0 ) },
        { glm::vec3( 1, 1, 0 ), glm::vec2( 1, 1 ) }
    };

    unsigned short indices[] = { 0, 1, 2, 0, 2, 3 };

    //Allocate vertex array
    //Vertex arrays encapsulate a set of generic vertex attributes and the buffers they are bound too
    //Different vertex array per mesh.
    context->gls_vertex_arrays[kGlsVertexArrayQuad] = gls::vertex_array();
    context->gls_vertex_arrays[kGlsVertexArrayQuad].bind();

    //Allocate vbos for data and indices
    context->gls_buffers[kGlsBufferQuadVertexBuffer] = gls::buffer( GL_ARRAY_BUFFER, GL_STATIC_DRAW );
    context->gls_buffers[kGlsBufferQuadIndexBuffer] = gls::buffer( GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW );

    //Upload vertex data
    context->gls_buffers[kGlsBufferQuadVertexBuffer].bind();
    context->gls_buffers[kGlsBufferQuadVertexBuffer].set_data( verts, std::size( verts ), sizeof( vertex2_t ) );

    //Use of strided data, Array of Structures instead of Structures of Arrays
    context->gls_vertex_arrays[kGlsVertexArrayQuad].set_attribute( quad_attributes::POSITION, context->gls_buffers[kGlsBufferQuadVertexBuffer], 3, GL_FLOAT, false, sizeof( vertex2_t ), 0 );
    context->gls_vertex_arrays[kGlsVertexArrayQuad].set_attribute( quad_attributes::TEXCOORD, context->gls_buffers[kGlsBufferQuadVertexBuffer], 2, GL_FLOAT, false, sizeof( vertex2_t ), sizeof( glm::vec3 ) );

    //Upload index data
    context->gls_buffers[kGlsBufferQuadIndexBuffer].bind();
    context->gls_buffers[kGlsBufferQuadVertexBuffer].set_data( indices, std::size( indices ), sizeof( unsigned short ) );

    //Unplug Vertex Array
    context->gls_vertex_arrays[kGlsVertexArrayQuad].unbind();
}
void draw_quad()
{
    glBindVertexArray( device_quad.vertex_array );
    context->gls_buffers[kGlsBufferQuadIndexBuffer].bind();
    glDrawElements( GL_TRIANGLES, GLsizei( context->gls_buffers[kGlsBufferQuadIndexBuffer].num_elements() ), GL_UNSIGNED_SHORT, 0 );
    glBindVertexArray( 0 );
}
