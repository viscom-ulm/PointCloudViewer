/**
 * @file   SSGIRenderer.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.23
 *
 * @brief  Implementation of the base point cloud on mesh renderer class.
 */

#include "SSGIRenderer.h"
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

#include <glm/gtc/random.hpp>
#include <random>

namespace pcViewer {


    SSGIRenderer::SSGIRenderer(ApplicationNodeImplementation* appNode) :
        BaseRenderer{ PCType::MATTE, RenderType::PC_ON_MESH, "SSGI", appNode }
    {
        ssgiQuad_ = std::make_unique<FullscreenQuad>("comparison/ssgi.frag", GetApp());
        ssgiUniformLocations_ = ssgiQuad_->GetGPUProgram()->GetUniformLocations({ "positionTexture", "normalTexture", "materialColorTexture", "directIlluminationTexture", "camPos" });
    }

    bool SSGIRenderer::IsAvaialble() const
    {
        return (GetMesh() != nullptr);
    }

    void SSGIRenderer::DrawPointCloudInternal(const FrameBuffer& fbo, const FrameBuffer& deferredFBO, bool batched)
    {
        GetMesh()->DrawShadowMap();
        deferredFBO.DrawToFBO(GetApp()->GetDeferredDrawIndices(), [this]() {
            GetMesh()->DrawMeshDeferred(true);
        });

        fbo.DrawToFBO([this, &deferredFBO]() {
            DrawHBAO(deferredFBO);
        });
    }

    void SSGIRenderer::DrawHBAO(const FrameBuffer & deferredFBO)
    {
        gl::glUseProgram(ssgiQuad_->GetGPUProgram()->getProgramId());

        std::array<std::size_t, 4> textureIndices{ 0, 1, 2, 4 };
        for (int i = 0; i < 4; ++i) {
            gl::glActiveTexture(gl::GL_TEXTURE0 + i);
            gl::glBindTexture(gl::GL_TEXTURE_2D, deferredFBO.GetTextures()[textureIndices[i]]);
            gl::glUniform1i(ssgiUniformLocations_[i], i);
        }

        gl::glUniform3fv(ssgiUniformLocations_[4], 1, glm::value_ptr(GetApp()->GetCameraEnh().GetPosition()));

        gl::glEnable(gl::GL_BLEND);
        gl::glBlendEquationSeparate(gl::GL_FUNC_ADD, gl::GL_FUNC_ADD);
        gl::glBlendFuncSeparate(gl::GL_SRC_ALPHA, gl::GL_ONE_MINUS_SRC_ALPHA, gl::GL_ONE, gl::GL_ONE);
        // gl::glUniformMatrix4fv(deferredUniformLocations_[0], 1, gl::GL_FALSE, glm::value_ptr(VP));
        ssgiQuad_->Draw();

        gl::glDisable(gl::GL_BLEND);
        gl::glUseProgram(0);

        // GetMesh()->DrawMeshDeferredAndExport(true);
    }

    // void SSGIRenderer::ExportScreenPointCloudMesh(std::ostream& meshPoints) const
    // {
    //     throw std::runtime_error("ExportScreenPointCloudMesh: Not implemented!!");
    // }

    void SSGIRenderer::RenderGUIByType()
    {
        if (ImGui::Button("Export SSGI PBRT")) {
            glm::uvec2 size{ GetApp()->GetDeferredExportFBO().GetWidth(), GetApp()->GetDeferredExportFBO().GetHeight() };

            auto outputName = GetMesh()->GetMeshName() + "_gi";
            std::ofstream pbrtOut{ outputName + ".pbrt" };
            std::ofstream pbrtOut_do{ outputName + "_direct_only.pbrt" };
            ExportPBRT(outputName + "_pbrt", size, pbrtOut, pbrtOut_do);

            GetApp()->SaveImageAllTechniques(outputName);
        }
    }

    double SSGIRenderer::DoPerformanceMeasureInternal(const FrameBuffer& fbo, const FrameBuffer& deferredFBO, bool batched)
    {
        GetMesh()->DrawShadowMap();
        deferredFBO.DrawToFBO(GetApp()->GetDeferredDrawIndices(), [this]() {
            GetMesh()->DrawMeshDeferred(true);
        });

        auto start = std::chrono::high_resolution_clock::now();
        for (std::size_t i = 0; i < 100; ++i) {
            fbo.DrawToFBO([this, &deferredFBO]() {
                DrawHBAO(deferredFBO);
            });
            gl::glFlush();
            gl::glFinish();
        }

        auto stop = std::chrono::high_resolution_clock::now();
        return static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count()) / 100.0;
    }

    // void SSGIRenderer::ExportScreenPointCloudScreen(const FrameBuffer & fbo, const std::string & namePrefix, std::ostream & screenPoints) const
    // {
    //     throw std::runtime_error("ExportScreenPointCloudScreen: Not implemented!!");
    // }

}
