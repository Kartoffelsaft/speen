#include <bx/math.h>

#include "modelInstance.h"
#include "rendererState.h"

ModelInstance ModelInstance::fromModelPtr(std::weak_ptr<Model const> const nModel) {
    Mat4 nOrientation;
    bx::mtxIdentity(nOrientation.data());

    return ModelInstance{
        .model = nModel,
        .orientation = nOrientation,
    };
}

void ModelInstance::draw() const {

    for(auto const & prim: model.lock()->primitives) {       
        bgfx::setUniform(rendererState.uniforms.u_modelMtx, orientation.data());
        bgfx::setState(
            BGFX_STATE_WRITE_RGB
          | BGFX_STATE_WRITE_A
          | BGFX_STATE_WRITE_Z
          | BGFX_STATE_CULL_CCW
          | BGFX_STATE_DEPTH_TEST_LESS
        );

        bgfx::setTransform(orientation.data());

        bgfx::setVertexBuffer(0, prim.vertexBuffer);
        bgfx::setIndexBuffer(prim.indexBuffer);

        bgfx::submit(rendererState.RENDER_SHADOW_ID, rendererState.shadowProgram);

        bgfx::setUniform(rendererState.uniforms.u_modelMtx, orientation.data());
        bgfx::setUniform(rendererState.uniforms.u_lightmapMtx, rendererState.lightmapMtx.data());
        bgfx::setState(
            BGFX_STATE_WRITE_RGB
          | BGFX_STATE_WRITE_A
          | BGFX_STATE_WRITE_Z
          | BGFX_STATE_CULL_CCW
          | BGFX_STATE_DEPTH_TEST_LESS
          | BGFX_STATE_MSAA
        );

        bgfx::setTransform(orientation.data());

        bgfx::setVertexBuffer(0, prim.vertexBuffer);
        bgfx::setIndexBuffer(prim.indexBuffer);

        bgfx::submit(rendererState.RENDER_SCENE_ID, rendererState.sceneProgram);
    }

    bgfx::setTexture(0, rendererState.uniforms.u_shadowmap, rendererState.shadowMap);
}
