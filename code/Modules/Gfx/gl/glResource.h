#pragma once
//------------------------------------------------------------------------------
#include "Gfx/Resource/resourceBase.h"
#include "Core/Containers/StaticArray.h"
#include "Gfx/Core/GfxTypes.h"
#include "Gfx/gl/gl_decl.h"
#include "Gfx/gl/glVertexAttr.h"

namespace Oryol {
namespace _priv {

//------------------------------------------------------------------------------
/**
    @class Oryol::_priv::glMesh
    @ingroup _priv
    @brief GL implementation of mesh
*/
class glMesh : public meshBase {
public:
    /// destructor
    ~glMesh();

    /// clear the object (called from meshFactory::DestroyResource())
    void Clear();

    static const int MaxNumSlots = 2;
    struct buffer {
        buffer() : updateFrameIndex(-1), numSlots(1), activeSlot(0) {
            this->glBuffers.Fill(0);
        }
        int updateFrameIndex;
        uint8_t numSlots;
        uint8_t activeSlot;
        StaticArray<GLuint, MaxNumSlots> glBuffers;
    };
    static const int vb = 0;
    static const int ib = 1;
    StaticArray<buffer, 2> buffers;
};

//------------------------------------------------------------------------------
/**
    @class Oryol::_priv::glPipeline
    @ingroup _priv
    @brief GL implementation of pipeline
*/
class glPipeline : public pipelineBase {
public:
    /// clear the object (called from pipelineFactory::DestroyResource()
    void Clear();
    
    StaticArray<glVertexAttr, VertexAttr::NumVertexAttrs> glAttrs;
    GLenum glPrimType = 0;
};

//------------------------------------------------------------------------------
/**
    @class Oryol::_priv::glShader
    @ingroup _priv
    @brief private: GL implementation of shader
*/
class glShader : public shaderBase {
public:
    /// constructor
    glShader();
    /// destructor
    ~glShader();
    
    /// clear the object
    void Clear();
    
    /// bind a uniform location to a slot index
    void bindUniformBlock(ShaderStage::Code bindStage, int bindSlot, GLint glUniformLocation);
    /// bind a sampler uniform location to a slot index
    void bindSampler(ShaderStage::Code bindStage, int textureIndex, int samplerIndex);
    #if ORYOL_GL_USE_GETATTRIBLOCATION
    /// bind a vertex attribute location
    void bindAttribLocation(VertexAttr::Code attrib, GLint attribLocation);
    /// get a vertex attribute location
    GLint getAttribLocation(VertexAttr::Code attrib) const;
    #endif

    /// compute uniform block index
    static int uniformBlockArrayIndex(ShaderStage::Code bindStage, int bindSlot);
    /// get location of GL uniform block vec4 array
    GLint getUniformBlockLocation(ShaderStage::Code bindStage, int bindSlot) const;
    /// get the data size of an uniform block
    GLint getUniformBlockSize(ShaderStage::Code bindStage, int bindSlot) const;
    /// get sampler index (InvalidIndex if not exists)
    int getSamplerIndex(ShaderStage::Code bindStage, int bindSlot) const;
    /// compute sampler array index
    static int samplerArrayIndex(ShaderStage::Code bindStage, int bindSlot);

    /// the GL shader program
    GLuint glProgram = 0;

    static const int MaxTexturesPerBlock = GfxConfig::MaxNumTextureBlockLayoutComponents;
    static const int MaxUBsPerStage = GfxConfig::MaxNumUniformBlocksPerStage;
    static const int MaxStages = ShaderStage::NumShaderStages;

    StaticArray<GLint, MaxStages*MaxUBsPerStage> uniformBlockMappings;
    StaticArray<int, MaxStages*MaxTexturesPerBlock> samplerMappings;
    #if ORYOL_GL_USE_GETATTRIBLOCATION
    StaticArray<GLint,VertexAttr::NumVertexAttrs> attribMapping;
    #endif
};

//------------------------------------------------------------------------------
inline int
glShader::uniformBlockArrayIndex(ShaderStage::Code bindStage, int bindSlot) {
    return bindSlot + bindStage*MaxUBsPerStage;
}

//------------------------------------------------------------------------------
inline GLint
glShader::getUniformBlockLocation(ShaderStage::Code bindStage, int bindSlot) const {
    return this->uniformBlockMappings[uniformBlockArrayIndex(bindStage, bindSlot)];
}

//------------------------------------------------------------------------------
inline int
glShader::samplerArrayIndex(ShaderStage::Code bindStage, int bindSlot) {
    return bindSlot + bindStage*MaxTexturesPerBlock;
}

//------------------------------------------------------------------------------
inline int
glShader::getSamplerIndex(ShaderStage::Code bindStage, int bindSlot) const {
    return this->samplerMappings[samplerArrayIndex(bindStage, bindSlot)];
}

//------------------------------------------------------------------------------
#if ORYOL_GL_USE_GETATTRIBLOCATION
inline GLint
glShader::getAttribLocation(VertexAttr::Code attrib) const {
    return this->attribMapping[attrib];
}
#endif

//------------------------------------------------------------------------------
/**
    @class Oryol::_priv::glTexture
    @ingroup _priv
    @brief GL implementation of texture class
*/
class glTexture : public textureBase {
public:
    /// constructor
    glTexture();
    /// destructor
    ~glTexture();
    
    /// clear the object
    void Clear();

    GLenum glTarget = 0;
    GLuint glDepthRenderbuffer = 0;
    GLuint glMSAARenderbuffer = 0;

    static const int MaxNumSlots = 2;
    int updateFrameIndex = -1;
    uint8_t numSlots = 1;
    uint8_t activeSlot = 0;
    StaticArray<GLuint, MaxNumSlots> glTextures;
};

//------------------------------------------------------------------------------
/**
    @class Oryol::_priv::glRenderPass
    @ingroup _priv
    @brief GL implementation of renderPass
*/
class glRenderPass : public renderPassBase {
public:
    /// constructor
    glRenderPass();
    /// destructor
    ~glRenderPass();

    /// clear the object
    void Clear();

    GLuint glFramebuffer = 0;
    StaticArray<GLuint, GfxConfig::MaxNumColorAttachments> glMSAAResolveFramebuffers;
};

} // namespace _priv
} // namespace Oryol
