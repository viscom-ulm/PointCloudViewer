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
        BaseRenderer{ pcType, RenderType::POINTCLOUD, "PointCloud", appNode }
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

            auto MVP = GetApp()->GetCameraEnh().GetViewProjMatrixEnh() *  GetPointCloud()->GetModelMatrix();

            DrawPointCloudPoints(MVP, GetApp()->GetCameraEnh().GetPosition(), batched);

            gl::glBindBuffer(gl::GL_ARRAY_BUFFER, 0);
            gl::glBindVertexArray(0);
            gl::glUseProgram(0);


            gl::glDisable(gl::GL_BLEND);
            gl::glDisable(gl::GL_PROGRAM_POINT_SIZE);
        });
    }

    double PointCloudRenderer::DoPerformanceMeasureInternal(const FrameBuffer& fbo, const FrameBuffer& deferredFBO, bool batched)
    {
        auto start = std::chrono::high_resolution_clock::now();
        for (std::size_t i = 0; i < 100; ++i) {
            DrawPointCloudInternal(fbo, deferredFBO, true);
            gl::glFlush();
            gl::glFinish();
        }

        auto stop = std::chrono::high_resolution_clock::now();
        return static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count()) / 100.0;
    }
    // void PointCloudRenderer::ExportScreenPointCloudMesh(std::ostream& meshPoints) const
    // {
    //     GetPointCloud()->ExportScreenPointCloudMesh(meshPoints);
    // }

}
