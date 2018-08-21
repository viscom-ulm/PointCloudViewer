/**
 * @file   AOPointCloudRenderer.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.17
 *
 * @brief  Declaration of the point cloud renderer for ambient occlusion.
 */

#pragma once

#include "app/Renderer.h"

namespace pcViewer {

    class AOPointCloudRenderer : public TBaseRenderer<PointCloudPointAO>
    {
    public:
        AOPointCloudRenderer(ApplicationNodeImplementation* appNode);

    protected:
        virtual void StorePointInPointCloud(const std::vector<std::string>& pointData) override;
        virtual void PreparePointCloudVAO() override;
        virtual void DrawPointCloudPoints(const glm::mat4& MVP, const glm::vec3& camPos, bool batched) override;
        virtual void DrawPointCloudDistanceSum(const glm::mat4& MVP, const FrameBuffer& deferredFBO) override;
        virtual void RenderGUIByType() override;
        virtual void ExportScreenPointCloudScreen(const FrameBuffer& fbo, std::ostream& screenPoints) const override;

    private:
        /** Holds the shader program for drawing ambient occlusion. */
        std::shared_ptr<GPUProgram> aoProgram_;
        /** Holds the location of the MVP matrix. */
        gl::GLint aoMVPLoc_ = -1;
        gl::GLint aoBBRLoc_ = -1;

        /** Holds the program for summation of distances. */
        std::shared_ptr<GPUProgram> distanceSumAOProgram_;
        /** Holds the uniform bindings for summation of distances. */
        std::vector<gl::GLint> distanceSumAOUniformLocations_;
    };
}
