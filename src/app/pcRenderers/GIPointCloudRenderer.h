/**
 * @file   GIPointCloudRenderer.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.17
 *
 * @brief  Declaration of the point cloud renderer for global illumination.
 */

#pragma once

#include "app/pcRenderers/PointCloudRenderer.h"

namespace viscom {
    class GPUProgram;
}

namespace pcViewer {

    class GIPointCloudRenderer : public  PointCloudRenderer
    {
    public:
        GIPointCloudRenderer(ApplicationNodeImplementation* appNode);

    protected:
        virtual void DrawPointCloudPoints(const glm::mat4& MVP, const glm::vec3& camPos, bool batched) override;
        virtual void RenderGUIByType() override;
        virtual void ExportScreenPointCloudScreen(const FrameBuffer& fbo, const std::string& namePrefix, std::ostream& screenPoints) const override;

    private:
        /** Holds the shader program for drawing matte results. */
        std::shared_ptr<GPUProgram> matteProgram_;
        /** Holds the location of the MVP matrix. */
        gl::GLint matteMVPLoc_ = -1;
        /** Holds the location of render type. */
        gl::GLint matteRenderTypeLoc_ = -1;
        gl::GLint matteBBRLoc_ = -1;
    };
}
