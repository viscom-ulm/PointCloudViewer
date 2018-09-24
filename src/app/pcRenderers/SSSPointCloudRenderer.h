/**
 * @file   SSSPointCloudRenderer.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.17
 *
 * @brief  Declaration of the point cloud renderer for subsurface scattering.
 */

#pragma once

#include "app/pcRenderers/PointCloudRenderer.h"

namespace viscom {
    class GPUProgram;
}

namespace pcViewer {

    class SSSPointCloudRenderer : public PointCloudRenderer
    {
    public:
        SSSPointCloudRenderer(ApplicationNodeImplementation* appNode);

    protected:
        virtual void DrawPointCloudPoints(const glm::mat4& MVP, const glm::vec3& camPos, bool batched) override;
        virtual void RenderGUIByType() override;
        virtual void ExportScreenPointCloudScreen(const FrameBuffer& fbo, const std::string& namePrefix, std::ostream& screenPoints) const override;

    private:
        /** Holds the shader program for drawing subsurface results. */
        std::shared_ptr<GPUProgram> subsurfaceProgram_;
        /** Holds the location of the MVP matrix. */
        gl::GLint subsurfaceMVPLoc_ = -1;
        /** Holds the location of render type. */
        gl::GLint subsurfaceRenderTypeLoc_ = -1;
        gl::GLint subsurfaceBBRLoc_ = -1;
    };
}
