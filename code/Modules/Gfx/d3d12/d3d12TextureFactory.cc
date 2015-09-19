//------------------------------------------------------------------------------
//  d3d12TextureFactory.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "d3d12TextureFactory.h"
#include "Core/Assertion.h"
#include "Gfx/Resource/texture.h"
#include "Gfx/Core/renderer.h"
#include "Gfx/Core/displayMgr.h"
#include "Gfx/Resource/resourcePools.h"
#include "d3d12_impl.h"

namespace Oryol {
namespace _priv {

//------------------------------------------------------------------------------
d3d12TextureFactory::d3d12TextureFactory() :
isValid(false) {
    // empty
}

//------------------------------------------------------------------------------
d3d12TextureFactory::~d3d12TextureFactory() {
    o_assert_dbg(!this->isValid);
}

//------------------------------------------------------------------------------
void
d3d12TextureFactory::Setup(const gfxPointers& ptrs) {
    o_assert_dbg(!this->isValid);
    this->isValid = true;
    this->pointers = ptrs;
}

//------------------------------------------------------------------------------
void
d3d12TextureFactory::Discard() {
    o_assert_dbg(this->isValid);
    this->isValid = false;
    this->pointers = gfxPointers();
}

//------------------------------------------------------------------------------
bool
d3d12TextureFactory::IsValid() const {
    return this->isValid;
}

//------------------------------------------------------------------------------
ResourceState::Code
d3d12TextureFactory::SetupResource(texture& tex) {
    o_assert_dbg(this->isValid);
    o_assert_dbg(!tex.Setup.ShouldSetupFromPixelData());
    o_assert_dbg(!tex.Setup.ShouldSetupFromFile());

    if (tex.Setup.ShouldSetupAsRenderTarget()) {
        return this->createRenderTarget(tex);
    }
    else {
        return ResourceState::InvalidState;
    }
}

//------------------------------------------------------------------------------
ResourceState::Code
d3d12TextureFactory::SetupResource(texture& tex, const void* data, int32 size) {
    o_assert_dbg(this->isValid);
    o_assert_dbg(!tex.Setup.ShouldSetupAsRenderTarget());
    o_assert_dbg(!tex.Setup.ShouldSetupFromFile());

    if (tex.Setup.ShouldSetupFromPixelData()) {
        return this->createFromPixelData(tex, data, size);
    }
    return ResourceState::InvalidState;
}

//------------------------------------------------------------------------------
void
d3d12TextureFactory::DestroyResource(texture& tex) {
    o_assert_dbg(this->isValid);
    d3d12ResAllocator& resAllocator = this->pointers.renderer->resAllocator;
    d3d12DescAllocator& descAllocator = this->pointers.renderer->descAllocator;
    const uint64 frameIndex = this->pointers.renderer->frameIndex;

    this->pointers.renderer->invalidateTextureState();

    if (tex.d3d12TextureRes) {
        resAllocator.ReleaseDeferred(frameIndex, tex.d3d12TextureRes);
    }
    if (tex.d3d12DepthBufferRes) {
        resAllocator.ReleaseDeferred(frameIndex, tex.d3d12DepthBufferRes);
    }
    if (tex.renderTargetView.IsValid()) {
        descAllocator.ReleaseDeferred(frameIndex, tex.renderTargetView);
    }
    if (tex.depthStencilView.IsValid()) {
        descAllocator.ReleaseDeferred(frameIndex, tex.depthStencilView);
    }
    tex.Clear();
}

//------------------------------------------------------------------------------
ResourceState::Code
d3d12TextureFactory::createRenderTarget(texture& tex) {
    o_assert_dbg(nullptr == tex.d3d12TextureRes);
    o_assert_dbg(nullptr == tex.d3d12DepthBufferRes);
    o_assert_dbg(!tex.renderTargetView.IsValid());
    o_assert_dbg(!tex.depthStencilView.IsValid());

    const TextureSetup& setup = tex.Setup;
    o_assert_dbg(setup.ShouldSetupAsRenderTarget());
    o_assert_dbg(setup.NumMipMaps == 1);
    o_assert_dbg(setup.Type == TextureType::Texture2D);
    o_assert_dbg(PixelFormat::IsValidRenderTargetColorFormat(setup.ColorFormat));

    d3d12ResAllocator& resAllocator = this->pointers.renderer->resAllocator;
    d3d12DescAllocator& descAllocator = this->pointers.renderer->descAllocator;
    ID3D12Device* d3d12Device = this->pointers.renderer->d3d12Device;
    const uint64 frameIndex = this->pointers.renderer->frameIndex;

    // get size of new render target
    int32 width, height;
    texture* sharedDepthProvider = nullptr;
    if (setup.IsRelSizeRenderTarget()) {
        const DisplayAttrs& dispAttrs = this->pointers.displayMgr->GetDisplayAttrs();
        width = int32(dispAttrs.FramebufferWidth * setup.RelWidth);
        height = int32(dispAttrs.FramebufferHeight * setup.RelHeight);
    }
    else if (setup.HasSharedDepth()) {
        sharedDepthProvider = this->pointers.texturePool->Lookup(setup.DepthRenderTarget);
        o_assert_dbg(nullptr != sharedDepthProvider);
        width = sharedDepthProvider->textureAttrs.Width;
        height = sharedDepthProvider->textureAttrs.Height;
    }
    else {
        width = setup.Width;
        height = setup.Height;
    }
    o_assert_dbg((width > 0) && (height > 0));

    // create the color buffer and render-target-view
    tex.d3d12TextureRes = resAllocator.AllocRenderTarget(d3d12Device, width, height, setup.ColorFormat, 1);
    tex.renderTargetView = descAllocator.Allocate(d3d12DescAllocator::RenderTargetView);
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
    descAllocator.CPUHandle(tex.renderTargetView, rtvHandle);
    d3d12Device->CreateRenderTargetView(tex.d3d12TextureRes, nullptr, rtvHandle);

    // create optional depth-buffer
    if (setup.HasDepth()) {
        if (setup.HasSharedDepth()) {
            o_assert_dbg(sharedDepthProvider->d3d12DepthBufferRes);
            tex.d3d12DepthBufferRes = sharedDepthProvider->d3d12DepthBufferRes;
            tex.d3d12DepthBufferRes->AddRef();
        }
        else {
            tex.d3d12DepthBufferRes = resAllocator.AllocRenderTarget(d3d12Device, width, height, setup.DepthFormat, 1);
        }
        tex.depthStencilView = descAllocator.Allocate(d3d12DescAllocator::DepthStencilView);
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle;
        descAllocator.CPUHandle(tex.depthStencilView, dsvHandle);
        d3d12Device->CreateDepthStencilView(tex.d3d12DepthBufferRes, nullptr, dsvHandle);
    }
    return ResourceState::Valid;
}

//------------------------------------------------------------------------------
ResourceState::Code 
d3d12TextureFactory::createFromPixelData(texture& tex, const void* data, int32 size) {
    return ResourceState::InvalidState;
}

} // namespace _priv
} // namespace Oryol
