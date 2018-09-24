/**
 * @file   GIPointCloudRenderer.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.17
 *
 * @brief  Definition of the point cloud renderer for global illumination.
 */

#include "GIPointCloudRenderer.h"
#include <glm/gtc/type_ptr.hpp>
#include "app/ApplicationNodeImplementation.h"
#include "enh/gfx/env/EnvironmentMapRenderer.h"
#include "app/PointCloudContainer.h"
#include <imgui.h>

namespace pcViewer {

    GIPointCloudRenderer::GIPointCloudRenderer(ApplicationNodeImplementation* appNode) :
        PointCloudRenderer{ PCType::MATTE, appNode }
    {
        matteProgram_ = GetApp()->GetGPUProgramManager().GetResource("matte", std::vector<std::string>{ "showMatte.vert", "showPCResult.geom", "showPCResult.frag" });
        matteMVPLoc_ = matteProgram_->getUniformLocation("MVP");
        matteRenderTypeLoc_ = matteProgram_->getUniformLocation("renderType");
        matteBBRLoc_ = matteProgram_->getUniformLocation("bbRadius");
    }

    void GIPointCloudRenderer::DrawPointCloudPoints(const glm::mat4& MVP, const glm::vec3& camPos, bool batched)
    {
        gl::glUseProgram(matteProgram_->getProgramId());
        gl::glUniformMatrix4fv(matteMVPLoc_, 1, gl::GL_FALSE, glm::value_ptr(MVP));
        gl::glUniform1i(matteRenderTypeLoc_, GetApp()->GetMatteRenderType());
        gl::glUniform1f(matteBBRLoc_, batched ? GetBoundingSphereRadius() / 2.f : GetBoundingSphereRadius());
        gl::glUniform3fv(matteProgram_->getUniformLocation("camPos"), 1, glm::value_ptr(camPos));
        gl::glDrawArrays(gl::GL_POINTS, 0, static_cast<gl::GLsizei>(GetPointCloudSize()));
    }

    void GIPointCloudRenderer::RenderGUIByType()
    {
        if (ImGui::RadioButton("Global Illumination", GetApp()->GetMatteRenderType() == 0)) GetApp()->SetMatteRenderType(0);
        if (ImGui::RadioButton("Matte Albedo", GetApp()->GetMatteRenderType() == 1)) GetApp()->SetMatteRenderType(1);
        if (ImGui::RadioButton("Direct Illumination", GetApp()->GetMatteRenderType() == 2)) GetApp()->SetMatteRenderType(2);
    }

    void GIPointCloudRenderer::ExportScreenPointCloudScreen(const FrameBuffer& fbo, const std::string& namePrefix, std::ostream& screenPoints) const
    {
        std::vector<glm::vec3> screenPositions, screenNormals, screenAlbedo, screenDirectIllumination;
        screenPositions.resize(static_cast<std::size_t>(fbo.GetWidth()) * fbo.GetHeight());
        screenNormals.resize(static_cast<std::size_t>(fbo.GetWidth()) * fbo.GetHeight());
        screenAlbedo.resize(static_cast<std::size_t>(fbo.GetWidth()) * fbo.GetHeight());
        screenDirectIllumination.resize(static_cast<std::size_t>(fbo.GetWidth()) * fbo.GetHeight());

        auto pboPos = CopyTextureToPixelBuffer3(fbo, 0);
        auto pboNormal = CopyTextureToPixelBuffer3(fbo, 1);
        auto pboAlbedo = CopyTextureToPixelBuffer3(fbo, 2);
        auto pboDirectIllumination = CopyTextureToPixelBuffer3(fbo, 4);

        gl::glMemoryBarrier(gl::GL_ALL_BARRIER_BITS);

        CopyPBOToVector3(pboPos, screenPositions);
        CopyPBOToVector3(pboNormal, screenNormals);
        CopyPBOToVector3(pboAlbedo, screenAlbedo);
        CopyPBOToVector3(pboDirectIllumination, screenDirectIllumination);

        for (std::size_t i = 0; i < screenPositions.size(); ++i) {
            screenPoints << screenPositions[i].x << ',' << screenPositions[i].y << ',' << screenPositions[i].z << ','
                << screenNormals[i].x << ',' << screenNormals[i].y << ',' << screenNormals[i].z << ','
                << screenAlbedo[i].r << ',' << screenAlbedo[i].g << ',' << screenAlbedo[i].b << ','
                << screenDirectIllumination[i].r << ',' << screenDirectIllumination[i].g << ',' << screenDirectIllumination[i].b << '\n';
        }
    }

}
