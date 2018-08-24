/**
 * @file   GIPCOnMeshRenderer.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.17
 *
 * @brief  Declaration of the point cloud on mesh renderer for global illumination.
 */

#pragma once

#include "app/pcOnMeshRenderers/PCOnMeshRenderer.h"

namespace viscom {
    class GPUProgram;
}

namespace pcViewer {

    class GIPCOnMeshRenderer : public PCOnMeshRenderer
    {
    public:
        GIPCOnMeshRenderer(ApplicationNodeImplementation* appNode);

    protected:
        virtual void DrawPointCloudDistanceSum(const glm::mat4& MVP, const FrameBuffer& deferredFBO) override;
        virtual void RenderGUIByType() override;
        virtual void ExportScreenPointCloudScreen(const FrameBuffer& fbo, std::ostream& screenPoints) const override;

    private:
        /** Holds the program for summation of distances. */
        std::shared_ptr<GPUProgram> distanceSumMatteProgram_;
        /** Holds the uniform bindings for summation of distances. */
        std::vector<gl::GLint> distanceSumMatteUniformLocations_;
    };
}