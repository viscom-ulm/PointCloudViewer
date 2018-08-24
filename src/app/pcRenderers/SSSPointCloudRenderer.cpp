/**
 * @file   SSSPointCloudRenderer.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.17
 *
 * @brief  Definition of the point cloud renderer for subsurface scattering.
 */

#include "SSSPointCloudRenderer.h"
#include <glm/gtc/type_ptr.hpp>
#include "app/ApplicationNodeImplementation.h"
#include "enh/gfx/env/EnvironmentMapRenderer.h"
#include "app/PointCloudContainer.h"
#include <imgui.h>

namespace pcViewer {

    SSSPointCloudRenderer::SSSPointCloudRenderer(ApplicationNodeImplementation* appNode) :
        PointCloudRenderer{ PCType::SUBSURFACE, appNode }
    {
        subsurfaceProgram_ = GetApp()->GetGPUProgramManager().GetResource("subsurface", std::vector<std::string>{ "showSubsurface.vert", "showPCResult.frag" });
        subsurfaceMVPLoc_ = subsurfaceProgram_->getUniformLocation("MVP");
        subsurfaceRenderTypeLoc_ = subsurfaceProgram_->getUniformLocation("renderType");
        subsurfaceBBRLoc_ = subsurfaceProgram_->getUniformLocation("bbRadius");
    }

    void SSSPointCloudRenderer::DrawPointCloudPoints(const glm::mat4& MVP, const glm::vec3& camPos, bool batched)
    {
        gl::glUseProgram(subsurfaceProgram_->getProgramId());
        gl::glUniformMatrix4fv(subsurfaceMVPLoc_, 1, gl::GL_FALSE, glm::value_ptr(MVP));
        gl::glUniform1i(subsurfaceRenderTypeLoc_, GetApp()->GetSubsurfaceRenderType());
        gl::glUniform1f(subsurfaceBBRLoc_, batched ? GetBoundingSphereRadius() / 2.f : GetBoundingSphereRadius());
        gl::glUniform3fv(subsurfaceProgram_->getUniformLocation("camPos"), 1, glm::value_ptr(camPos));
        gl::glDrawArrays(gl::GL_POINTS, 0, static_cast<gl::GLsizei>(GetPointCloudSize()));
    }

    void SSSPointCloudRenderer::RenderGUIByType()
    {
        if (ImGui::RadioButton("Global Illumination", GetApp()->GetSubsurfaceRenderType() == 0)) GetApp()->SetSubsurfaceRenderType(0);
        if (ImGui::RadioButton("Subsurface Albedo", GetApp()->GetSubsurfaceRenderType() == 1)) GetApp()->SetSubsurfaceRenderType(1);
        if (ImGui::RadioButton("Sigma_t'", GetApp()->GetSubsurfaceRenderType() == 2)) GetApp()->SetSubsurfaceRenderType(2);
        if (ImGui::RadioButton("Eta", GetApp()->GetSubsurfaceRenderType() == 3)) GetApp()->SetSubsurfaceRenderType(3);
        if (ImGui::RadioButton("Direct Illumination", GetApp()->GetSubsurfaceRenderType() == 4)) GetApp()->SetSubsurfaceRenderType(4);
    }

    void SSSPointCloudRenderer::ExportScreenPointCloudScreen(const FrameBuffer & fbo, std::ostream & screenPoints) const
    {
        std::vector<glm::vec3> screenPositions, screenNormals, screenAlbedo, screenDirectIllumination;
        std::vector<glm::vec4> screenScattering;
        screenPositions.resize(static_cast<std::size_t>(fbo.GetWidth()) * fbo.GetHeight());
        screenNormals.resize(static_cast<std::size_t>(fbo.GetWidth()) * fbo.GetHeight());
        screenAlbedo.resize(static_cast<std::size_t>(fbo.GetWidth()) * fbo.GetHeight());
        screenScattering.resize(static_cast<std::size_t>(fbo.GetWidth()) * fbo.GetHeight());
        screenDirectIllumination.resize(static_cast<std::size_t>(fbo.GetWidth()) * fbo.GetHeight());

        auto pboPos = CopyTextureToPixelBuffer3(fbo, 0);
        auto pboNormal = CopyTextureToPixelBuffer3(fbo, 1);
        auto pboAlbedo = CopyTextureToPixelBuffer3(fbo, 2);
        auto pboScattering = CopyTextureToPixelBuffer4(fbo, 3);
        auto pboDirectIllumination = CopyTextureToPixelBuffer3(fbo, 4);

        gl::glMemoryBarrier(gl::GL_ALL_BARRIER_BITS);

        CopyPBOToVector3(pboPos, screenPositions);
        CopyPBOToVector3(pboNormal, screenNormals);
        CopyPBOToVector3(pboAlbedo, screenAlbedo);
        CopyPBOToVector4(pboScattering, screenScattering);
        CopyPBOToVector3(pboDirectIllumination, screenDirectIllumination);

        for (std::size_t i = 0; i < screenPositions.size(); ++i) {
            screenPoints << screenPositions[i].x << ',' << screenPositions[i].y << ',' << screenPositions[i].z << ','
                << screenNormals[i].x << ',' << screenNormals[i].y << ',' << screenNormals[i].z
                << screenAlbedo[i].r << ',' << screenAlbedo[i].g << ',' << screenAlbedo[i].b
                << screenScattering[i].r << ',' << screenScattering[i].g << ',' << screenScattering[i].b << ',' << screenScattering[i].a
                << screenDirectIllumination[i].r << ',' << screenDirectIllumination[i].g << ',' << screenDirectIllumination[i].b << '\n';
        }
    }

}