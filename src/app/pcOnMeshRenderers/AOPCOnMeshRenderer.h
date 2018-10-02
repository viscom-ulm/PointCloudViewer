/**
 * @file   AOPCOnMeshRenderer.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.17
 *
 * @brief  Declaration of the point cloud renderer for ambient occlusion.
 */

#pragma once

#include "app/pcOnMeshRenderers/PCOnMeshRenderer.h"

namespace viscom {
    class GPUProgram;
}

namespace pcViewer {

    class AOPCOnMeshRenderer : public PCOnMeshRenderer
    {
    public:
        AOPCOnMeshRenderer(ApplicationNodeImplementation* appNode);

    protected:
        virtual void DrawPointCloudDistanceSum(const glm::mat4& VP, const glm::mat4& M, const FrameBuffer& deferredFBO) override;
        virtual void RenderGUIByType() override;
        // virtual void ExportScreenPointCloudScreen(const FrameBuffer& fbo, const std::string& namePrefix, std::ostream& screenPoints) const override;

        virtual bool IsAmbientOcclustion() const override { return true; }

    private:
        /** Holds the program for summation of distances. */
        std::shared_ptr<GPUProgram> distanceSumAOProgram_;
        /** Holds the uniform bindings for summation of distances. */
        std::vector<gl::GLint> distanceSumAOUniformLocations_;
    };
}
