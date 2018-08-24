/**
 * @file   PCOnMeshRenderer.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.23
 *
 * @brief  Declaration of the point cloud on mesh renderer base class.
 */

#pragma once

#include "app/Renderer.h"

namespace pcViewer {

    class PCOnMeshRenderer : public BaseRenderer
    {
    public:
        PCOnMeshRenderer(PCType pcType, ApplicationNodeImplementation* appNode);

        virtual bool IsAvaialble() const override;

    protected:
        virtual void DrawPointCloudInternal(const FrameBuffer& fbo, const FrameBuffer& deferredFBO, bool batched) override;

        void DrawPointCloudDistanceSumInternal(const FrameBuffer& deferredFBO);
        virtual void DrawPointCloudDistanceSum(const glm::mat4& MVP, const FrameBuffer& deferredFBO) = 0;
        void DrawPointCloudOnMesh(const FrameBuffer& deferredFBO);

    private:
        /** Holds the program for final rendering. */
        std::unique_ptr<FullscreenQuad> finalQuad_;
        /** Holds the uniform bindings for final rendering. */
        std::vector<gl::GLint> finalUniformLocations_;
    };
}
