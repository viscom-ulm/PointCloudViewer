/**
 * @file   AOPointCloudRenderer.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.17
 *
 * @brief  Declaration of the point cloud renderer for ambient occlusion.
 */

#pragma once

#include "app/pcRenderers/PointCloudRenderer.h"

namespace viscom {
    class GPUProgram;
}

namespace pcViewer {

    class AOPointCloudRenderer : public PointCloudRenderer
    {
    public:
        AOPointCloudRenderer(ApplicationNodeImplementation* appNode);

    protected:
        virtual void DrawPointCloudPoints(const glm::mat4& MVP, const glm::vec3& camPos, bool batched) override;
        virtual void RenderGUIByType() override;
        virtual void ExportScreenPointCloudScreen(const FrameBuffer& fbo, const std::string& namePrefix, std::ostream& screenPoints) const override;

    private:
        /** Holds the shader program for drawing ambient occlusion. */
        std::shared_ptr<GPUProgram> aoProgram_;
        /** Holds the location of the MVP matrix. */
        gl::GLint aoMVPLoc_ = -1;
        gl::GLint aoBBRLoc_ = -1;
    };
}
