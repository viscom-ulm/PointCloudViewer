/**
 * @file   GIPCOnMeshRenderer.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.17
 *
 * @brief  Definition of the point cloud renderer for global illumination.
 */

#include "GIPCOnMeshRenderer.h"
#include <glm/gtc/type_ptr.hpp>
#include "app/ApplicationNodeImplementation.h"
#include "enh/gfx/env/EnvironmentMapRenderer.h"
#include "app/PointCloudContainer.h"
#include <imgui.h>

namespace pcViewer {

    GIPCOnMeshRenderer::GIPCOnMeshRenderer(ApplicationNodeImplementation* appNode) :
        PCOnMeshRenderer{ PCType::MATTE, appNode }
    {
        distanceSumMatteProgram_ = GetApp()->GetGPUProgramManager().GetResource("distanceSumMatte", std::vector<std::string>{ "distanceSumMatte.vert", "distanceSum.frag" });
        distanceSumMatteUniformLocations_ = distanceSumMatteProgram_->GetUniformLocations({ "viewProjection", "positionTexture", "normalTexture", "distancePower", "pointSize", "renderType" });
    }

    void GIPCOnMeshRenderer::DrawPointCloudDistanceSum(const glm::mat4& MVP, const FrameBuffer& deferredFBO)
    {
        gl::glUseProgram(distanceSumMatteProgram_->getProgramId());
        gl::glUniformMatrix4fv(distanceSumMatteUniformLocations_[0], 1, gl::GL_FALSE, glm::value_ptr(MVP));

        gl::glActiveTexture(gl::GL_TEXTURE2);
        gl::glBindTexture(gl::GL_TEXTURE_2D, deferredFBO.GetTextures()[0]);
        gl::glUniform1i(distanceSumMatteUniformLocations_[1], 2);

        gl::glActiveTexture(gl::GL_TEXTURE3);
        gl::glBindTexture(gl::GL_TEXTURE_2D, deferredFBO.GetTextures()[1]);
        gl::glUniform1i(distanceSumMatteUniformLocations_[2], 3);
        gl::glUniform1f(distanceSumMatteUniformLocations_[3], GetDistancePower());
        gl::glUniform1f(distanceSumMatteUniformLocations_[4], GetPointSize());
        gl::glUniform1i(distanceSumMatteUniformLocations_[5], GetApp()->GetMatteRenderType());
        gl::glUniform3fv(distanceSumMatteProgram_->getUniformLocation("camPos"), 1, glm::value_ptr(GetApp()->GetCameraEnh().GetPosition()));
        gl::glDrawArrays(gl::GL_POINTS, 0, static_cast<gl::GLsizei>(GetPointCloudSize()));
    }

    void GIPCOnMeshRenderer::RenderGUIByType()
    {
        if (ImGui::RadioButton("Global Illumination", GetApp()->GetMatteRenderType() == 0)) GetApp()->SetMatteRenderType(0);
        if (ImGui::RadioButton("Matte Albedo", GetApp()->GetMatteRenderType() == 1)) GetApp()->SetMatteRenderType(1);
        if (ImGui::RadioButton("Direct Illumination", GetApp()->GetMatteRenderType() == 2)) GetApp()->SetMatteRenderType(2);
    }

    void GIPCOnMeshRenderer::ExportScreenPointCloudScreen(const FrameBuffer& fbo, std::ostream& screenPoints) const
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
                << screenNormals[i].x << ',' << screenNormals[i].y << ',' << screenNormals[i].z
                << screenAlbedo[i].r << ',' << screenAlbedo[i].g << ',' << screenAlbedo[i].b
                << screenDirectIllumination[i].r << ',' << screenDirectIllumination[i].g << ',' << screenDirectIllumination[i].b << '\n';
        }
    }

}