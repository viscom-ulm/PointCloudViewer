/**
 * @file   PointCloudRenderer.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.23
 *
 * @brief  Implementation of the base point cloud renderer class.
 */

#include "PointCloudRenderer.h"
#include "app/ApplicationNodeImplementation.h"
#include "app/PointCloudContainer.h"
#include "enh/gfx/env/EnvironmentMapRenderer.h"

namespace pcViewer {


    PointCloudRenderer::PointCloudRenderer(PCType pcType, ApplicationNodeImplementation* appNode) :
        BaseRenderer{ pcType, "PointCloud", appNode }
    {
    }

    bool PointCloudRenderer::IsAvaialble() const
    {
        return (GetPointCloud() != nullptr);
    }

    void PointCloudRenderer::DrawPointCloudInternal(const FrameBuffer& fbo, const FrameBuffer& deferredFBO, bool batched)
    {
        fbo.DrawToFBO([this, batched]() {
            gl::glEnable(gl::GL_PROGRAM_POINT_SIZE);
            gl::glEnable(gl::GL_BLEND);
            gl::glBlendFunc(gl::GL_SRC_ALPHA, gl::GL_ONE_MINUS_SRC_ALPHA);

            gl::glBindVertexArray(GetPointCloud()->GetVAO());
            gl::glBindBuffer(gl::GL_ARRAY_BUFFER, GetPointCloud()->GetVBO());

            glm::mat4 modelMatrix(1.0f);
            auto MVP = GetApp()->GetCamera()->GetViewPerspectiveMatrix() * modelMatrix;

            DrawPointCloudPoints(MVP, GetApp()->GetCameraEnh().GetPosition(), batched);

            gl::glBindBuffer(gl::GL_ARRAY_BUFFER, 0);
            gl::glBindVertexArray(0);
            gl::glUseProgram(0);


            gl::glDisable(gl::GL_BLEND);
            gl::glDisable(gl::GL_PROGRAM_POINT_SIZE);
        });
    }

}
