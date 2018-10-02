/**
 * @file   HBAORenderer.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.23
 *
 * @brief  Implementation of the base point cloud on mesh renderer class.
 */

#include "HBAORenderer.h"
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


    HBAORenderer::HBAORenderer(ApplicationNodeImplementation* appNode) :
        BaseRenderer{ PCType::AO, RenderType::PC_ON_MESH, "HBAO", appNode }
    {
        hbaoQuad_ = std::make_unique<FullscreenQuad>("comparison/hbao.frag", GetApp());
        hbaoUniformLocations_ = hbaoQuad_->GetGPUProgram()->GetUniformLocations({ "positionTexture", "normalTexture", "materialColorTexture", "directIlluminationTexture", "noiseTexture", "camPos", "view" });

        gl::glBindTexture(gl::GL_TEXTURE_2D, noiseTexture_);

        const std::size_t noiseWidth = 4;
        const std::size_t noiseHeight = 4;
        std::vector<glm::vec4> noise; noise.resize(noiseWidth * noiseHeight);

        std::mt19937 rand_engine;
        std::uniform_real_distribution<float> uniform_dist;

        for (auto& value : noise) {
            auto alpha = uniform_dist(rand_engine) * glm::two_pi<float>();
            value = glm::vec4(glm::cos(alpha), glm::sin(alpha), uniform_dist(rand_engine), uniform_dist(rand_engine));
        }

        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_S, gl::GL_REPEAT);
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_T, gl::GL_REPEAT);
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MAG_FILTER, gl::GL_NEAREST);
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MIN_FILTER, gl::GL_NEAREST);

        gl::glTexImage2D(gl::GL_TEXTURE_2D, 0, gl::GL_RGBA32F, noiseWidth, noiseHeight, 0, gl::GL_RGBA, gl::GL_FLOAT, noise.data());
    }

    bool HBAORenderer::IsAvaialble() const
    {
        return (GetMesh() != nullptr);
    }

    void HBAORenderer::DrawPointCloudInternal(const FrameBuffer& fbo, const FrameBuffer& deferredFBO, bool batched)
    {
        GetMesh()->DrawShadowMap();
        deferredFBO.DrawToFBO(GetApp()->GetDeferredDrawIndices(), [this]() {
            GetMesh()->DrawMeshDeferred(true);
        });

        fbo.DrawToFBO([this, &deferredFBO]() {
            DrawHBAO(deferredFBO);
        });
    }

    void HBAORenderer::DrawHBAO(const FrameBuffer & deferredFBO)
    {
        gl::glUseProgram(hbaoQuad_->GetGPUProgram()->getProgramId());

        std::array<std::size_t, 4> textureIndices{ 0, 1, 2, 4 };
        for (int i = 0; i < 4; ++i) {
            gl::glActiveTexture(gl::GL_TEXTURE0 + i);
            gl::glBindTexture(gl::GL_TEXTURE_2D, deferredFBO.GetTextures()[textureIndices[i]]);
            gl::glUniform1i(hbaoUniformLocations_[i], i);
        }

        gl::glActiveTexture(gl::GL_TEXTURE0 + 5);
        gl::glBindTexture(gl::GL_TEXTURE_2D, noiseTexture_);
        gl::glUniform1i(hbaoUniformLocations_[4], 5);
        gl::glUniform3fv(hbaoUniformLocations_[5], 1, glm::value_ptr(GetApp()->GetCameraEnh().GetPosition()));
        gl::glUniformMatrix4fv(hbaoUniformLocations_[6], 1, false, glm::value_ptr(GetApp()->GetCameraEnh().GetViewMatrix()));

        gl::glEnable(gl::GL_BLEND);
        gl::glBlendEquationSeparate(gl::GL_FUNC_ADD, gl::GL_FUNC_ADD);
        gl::glBlendFuncSeparate(gl::GL_SRC_ALPHA, gl::GL_ONE_MINUS_SRC_ALPHA, gl::GL_ONE, gl::GL_ONE);
        // gl::glUniformMatrix4fv(deferredUniformLocations_[0], 1, gl::GL_FALSE, glm::value_ptr(VP));
        hbaoQuad_->Draw();

        gl::glDisable(gl::GL_BLEND);
        gl::glUseProgram(0);

        // GetMesh()->DrawMeshDeferredAndExport(true);
    }

    // void HBAORenderer::ExportScreenPointCloudMesh(std::ostream& meshPoints) const
    // {
    //     throw std::runtime_error("ExportScreenPointCloudMesh: Not implemented!!");
    // }

    void HBAORenderer::RenderGUIByType()
    {
        if (ImGui::Button("Export HBAO PBRT")) {
            glm::uvec2 size{ GetApp()->GetDeferredExportFBO().GetWidth(), GetApp()->GetDeferredExportFBO().GetHeight() };

            auto outputName = GetMesh()->GetMeshName() + "_ao";
            std::ofstream pbrtOut{ outputName + ".pbrt" };
            std::ofstream pbrtOut_do{ outputName + "_direct_only.pbrt" };
            ExportPBRT(outputName + "_pbrt", size, pbrtOut, pbrtOut_do);

            GetApp()->SaveImageAllTechniques(outputName);
        }
    }

    // void HBAORenderer::ExportScreenPointCloudScreen(const FrameBuffer & fbo, const std::string & namePrefix, std::ostream & screenPoints) const
    // {
    //     throw std::runtime_error("ExportScreenPointCloudScreen: Not implemented!!");
    // }

}
