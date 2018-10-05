/**
 * @file   PCOnMeshRenderer.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.23
 *
 * @brief  Implementation of the base point cloud on mesh renderer class.
 */

#include "PCOnMeshRenderer.h"
#include <glm/gtc/type_ptr.hpp>
#include "core/CameraHelper.h"
#include "core/gfx/FrameBuffer.h"
#include "enh/gfx/env/EnvironmentMapRenderer.h"
#include "app/camera/ArcballCameraEnhanced.h"
#include "app/ApplicationNodeImplementation.h"
#include "app/PointCloudContainer.h"
#include "app/MeshContainer.h"
#include <imgui.h>
#include <fstream>

namespace pcViewer {


    PCOnMeshRenderer::PCOnMeshRenderer(PCType pcType, ApplicationNodeImplementation* appNode) :
        BaseRenderer{ pcType, RenderType::PC_ON_MESH, "PointCloud on Mesh", appNode }
    {
        finalQuad_ = std::make_unique<FullscreenQuad>("finalComposite.frag", GetApp());
        finalUniformLocations_ = finalQuad_->GetGPUProgram()->GetUniformLocations({ "positionTexture", "normalTexture", "materialColorTexture", "directIlluminationTexture", "globalIlluminationTexture", "compositionType", "isAmbientOcclusion" });
    }

    bool PCOnMeshRenderer::IsAvaialble() const
    {
        return (GetPointCloud() != nullptr && GetMesh() != nullptr);
    }

    void PCOnMeshRenderer::DrawPointCloudInternal(const FrameBuffer& fbo, const FrameBuffer& deferredFBO, bool batched)
    {
        GetMesh()->DrawShadowMap();
        deferredFBO.DrawToFBO(GetApp()->GetDeferredDrawIndices(), [this]() {
            if (GetPointCloud() && GetPointCloud()->HasDirectLight()) GetMesh()->DrawMeshDeferred(false);
            else GetMesh()->DrawMeshDeferred(true);
        });

        deferredFBO.DrawToFBO(GetApp()->GetDistanceSumDrawIndices(), [this, &deferredFBO]() {
            DrawPointCloudDistanceSumInternal(deferredFBO);
        });

        fbo.DrawToFBO([this, &deferredFBO]() {
            DrawPointCloudOnMesh(deferredFBO);
        });
    }

    double PCOnMeshRenderer::DoPerformanceMeasureInternal(const FrameBuffer& fbo, const FrameBuffer& deferredFBO, bool batched)
    {
        GetMesh()->DrawShadowMap();
        deferredFBO.DrawToFBO(GetApp()->GetDeferredDrawIndices(), [this]() {
            if (GetPointCloud() && GetPointCloud()->HasDirectLight()) GetMesh()->DrawMeshDeferred(false);
            else GetMesh()->DrawMeshDeferred(true);
        });

        auto start = std::chrono::high_resolution_clock::now();
        for (std::size_t i = 0; i < 100; ++i) {
            deferredFBO.DrawToFBO(GetApp()->GetDistanceSumDrawIndices(), [this, &deferredFBO]() {
                DrawPointCloudDistanceSumInternal(deferredFBO);
            });

            fbo.DrawToFBO([this, &deferredFBO]() {
                DrawPointCloudOnMesh(deferredFBO);
            });
            gl::glFlush();
            gl::glFinish();
        }

        auto stop = std::chrono::high_resolution_clock::now();
        return static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count()) / 100.0;
    }

    void PCOnMeshRenderer::DrawPointCloudDistanceSumInternal(const FrameBuffer& deferredFBO)
    {
        gl::glEnable(gl::GL_PROGRAM_POINT_SIZE);

        gl::glBindVertexArray(GetPointCloud()->GetVAO());
        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, GetPointCloud()->GetVBO());

        gl::glDisable(gl::GL_DEPTH_TEST);
        gl::glDepthMask(gl::GL_FALSE);
        gl::glEnable(gl::GL_BLEND);
        gl::glBlendEquationSeparate(gl::GL_FUNC_ADD, gl::GL_FUNC_ADD);
        gl::glBlendFuncSeparate(gl::GL_ONE, gl::GL_ONE, gl::GL_ONE, gl::GL_ONE);

        auto VP = GetApp()->GetCameraEnh().GetViewProjMatrixEnh();
        auto M = GetPointCloud()->GetModelMatrix();
        DrawPointCloudDistanceSum(VP, M, deferredFBO);

        gl::glDisable(gl::GL_BLEND);
        gl::glDepthMask(gl::GL_TRUE);
        gl::glEnable(gl::GL_DEPTH_TEST);

        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, 0);
        gl::glBindVertexArray(0);
        gl::glUseProgram(0);

        gl::glDisable(gl::GL_PROGRAM_POINT_SIZE);
    }

    void PCOnMeshRenderer::DrawPointCloudOnMesh(const FrameBuffer & deferredFBO)
    {
        gl::glUseProgram(finalQuad_->GetGPUProgram()->getProgramId());

        std::array<std::size_t, 5> textureIndices{ 0, 1, 2, 4, 5 };
        for (int i = 0; i < 5; ++i) {
            gl::glActiveTexture(gl::GL_TEXTURE0 + i);
            gl::glBindTexture(gl::GL_TEXTURE_2D, deferredFBO.GetTextures()[textureIndices[i]]);
            gl::glUniform1i(finalUniformLocations_[i], i);
        }

        gl::glUniform1i(finalUniformLocations_[5], GetApp()->GetCompositeType());
        gl::glUniform1i(finalUniformLocations_[6], IsAmbientOcclustion() ? 1 : 0);

        gl::glEnable(gl::GL_BLEND);
        gl::glBlendEquationSeparate(gl::GL_FUNC_ADD, gl::GL_FUNC_ADD);
        gl::glBlendFuncSeparate(gl::GL_SRC_ALPHA, gl::GL_ONE_MINUS_SRC_ALPHA, gl::GL_ONE, gl::GL_ONE);
        // gl::glUniformMatrix4fv(deferredUniformLocations_[0], 1, gl::GL_FALSE, glm::value_ptr(VP));
        finalQuad_->Draw();

        gl::glDisable(gl::GL_BLEND);
        gl::glUseProgram(0);

        // GetMesh()->DrawMeshDeferredAndExport(true);
    }

    // void PCOnMeshRenderer::ExportScreenPointCloudMesh(std::ostream& meshPoints) const
    // {
    //     if (DoExportPointCloud()) GetPointCloud()->ExportScreenPointCloudMesh(meshPoints);
    //     else {
    //         using PointCloudPoint = PointCloudPointSubsurface;
    //         auto& pointCloud = GetMesh()->GetExportedPointCloud();
    // 
    //         std::copy(pointCloud.begin(), pointCloud.end(), std::ostream_iterator<PointCloudPoint>(meshPoints, "\n"));
    //     }
    // }

}
