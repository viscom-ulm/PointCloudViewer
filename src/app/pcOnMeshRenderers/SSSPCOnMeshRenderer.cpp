/**
 * @file   SSSPCOnMeshRenderer.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.17
 *
 * @brief  Definition of the point cloud renderer for subsurface scattering.
 */

#include "SSSPCOnMeshRenderer.h"
#include <glm/gtc/type_ptr.hpp>
#include "app/ApplicationNodeImplementation.h"
#include "enh/gfx/env/EnvironmentMapRenderer.h"
#include "app/PointCloudContainer.h"
#include <imgui.h>

namespace pcViewer {

    SSSPCOnMeshRenderer::SSSPCOnMeshRenderer(ApplicationNodeImplementation* appNode) :
        PCOnMeshRenderer{ PCType::SUBSURFACE, appNode }
    {
        distanceSumSubsurfaceProgram_ = GetApp()->GetGPUProgramManager().GetResource("distanceSumSubsurface", std::vector<std::string>{ "distanceSumSubsurface.vert", "distanceSum.frag" });
        distanceSumSubsurfaceUniformLocations_ = distanceSumSubsurfaceProgram_->GetUniformLocations({ "viewProjection", "positionTexture", "normalTexture", "distancePower", "pointSize", "renderType", "modelMatrix" });
    }

    void SSSPCOnMeshRenderer::DrawPointCloudDistanceSum(const glm::mat4& VP, const glm::mat4& M, const FrameBuffer& deferredFBO)
    {
        gl::glUseProgram(distanceSumSubsurfaceProgram_->getProgramId());
        gl::glUniformMatrix4fv(distanceSumSubsurfaceUniformLocations_[0], 1, gl::GL_FALSE, glm::value_ptr(VP));
        gl::glUniformMatrix4fv(distanceSumSubsurfaceUniformLocations_[6], 1, gl::GL_FALSE, glm::value_ptr(M));

        gl::glActiveTexture(gl::GL_TEXTURE2);
        gl::glBindTexture(gl::GL_TEXTURE_2D, deferredFBO.GetTextures()[0]);
        gl::glUniform1i(distanceSumSubsurfaceUniformLocations_[1], 2);

        gl::glActiveTexture(gl::GL_TEXTURE3);
        gl::glBindTexture(gl::GL_TEXTURE_2D, deferredFBO.GetTextures()[1]);
        gl::glUniform1i(distanceSumSubsurfaceUniformLocations_[2], 3);
        gl::glUniform1f(distanceSumSubsurfaceUniformLocations_[3], GetDistancePower());
        gl::glUniform1f(distanceSumSubsurfaceUniformLocations_[4], GetPointSize());
        gl::glUniform1i(distanceSumSubsurfaceUniformLocations_[5], GetApp()->GetSubsurfaceRenderType());
        gl::glUniform3fv(distanceSumSubsurfaceProgram_->getUniformLocation("camPos"), 1, glm::value_ptr(GetApp()->GetCameraEnh().GetPosition()));
        gl::glDrawArrays(gl::GL_POINTS, 0, static_cast<gl::GLsizei>(GetPointCloudSize()));
    }

    void SSSPCOnMeshRenderer::RenderGUIByType()
    {
        if (ImGui::RadioButton("Global Illumination", GetApp()->GetSubsurfaceRenderType() == 0)) GetApp()->SetSubsurfaceRenderType(0);
        if (ImGui::RadioButton("Subsurface Albedo", GetApp()->GetSubsurfaceRenderType() == 1)) GetApp()->SetSubsurfaceRenderType(1);
        if (ImGui::RadioButton("Sigma_t'", GetApp()->GetSubsurfaceRenderType() == 2)) GetApp()->SetSubsurfaceRenderType(2);
        if (ImGui::RadioButton("Eta", GetApp()->GetSubsurfaceRenderType() == 3)) GetApp()->SetSubsurfaceRenderType(3);
        if (ImGui::RadioButton("Direct Illumination", GetApp()->GetSubsurfaceRenderType() == 4)) GetApp()->SetSubsurfaceRenderType(4);
    }

    // void SSSPCOnMeshRenderer::ExportScreenPointCloudScreen(const FrameBuffer & fbo, const std::string& namePrefix, std::ostream & screenPoints) const
    // {
    //     std::vector<glm::vec3> screenPositions, screenNormals, screenAlbedo, screenDirectIllumination;
    //     std::vector<glm::vec4> screenScattering;
    //     screenPositions.resize(static_cast<std::size_t>(fbo.GetWidth()) * fbo.GetHeight());
    //     screenNormals.resize(static_cast<std::size_t>(fbo.GetWidth()) * fbo.GetHeight());
    //     screenAlbedo.resize(static_cast<std::size_t>(fbo.GetWidth()) * fbo.GetHeight());
    //     screenScattering.resize(static_cast<std::size_t>(fbo.GetWidth()) * fbo.GetHeight());
    //     screenDirectIllumination.resize(static_cast<std::size_t>(fbo.GetWidth()) * fbo.GetHeight());
    // 
    //     auto pboPos = CopyTextureToPixelBuffer3(fbo, 0);
    //     auto pboNormal = CopyTextureToPixelBuffer3(fbo, 1);
    //     auto pboAlbedo = CopyTextureToPixelBuffer3(fbo, 2);
    //     auto pboScattering = CopyTextureToPixelBuffer4(fbo, 3);
    //     auto pboDirectIllumination = CopyTextureToPixelBuffer3(fbo, 4);
    // 
    //     gl::glMemoryBarrier(gl::GL_ALL_BARRIER_BITS);
    // 
    //     CopyPBOToVector3(pboPos, screenPositions);
    //     CopyPBOToVector3(pboNormal, screenNormals);
    //     CopyPBOToVector3(pboAlbedo, screenAlbedo);
    //     CopyPBOToVector4(pboScattering, screenScattering);
    //     CopyPBOToVector3(pboDirectIllumination, screenDirectIllumination);
    // 
    //     for (std::size_t i = 0; i < screenPositions.size(); ++i) {
    //         screenPoints << screenPositions[i].x << ',' << screenPositions[i].y << ',' << screenPositions[i].z << ','
    //             << screenNormals[i].x << ',' << screenNormals[i].y << ',' << screenNormals[i].z << ','
    //             << screenAlbedo[i].r << ',' << screenAlbedo[i].g << ',' << screenAlbedo[i].b << ','
    //             << screenScattering[i].r << ',' << screenScattering[i].g << ',' << screenScattering[i].b << ',' << screenScattering[i].a << ','
    //             << screenDirectIllumination[i].r << ',' << screenDirectIllumination[i].g << ',' << screenDirectIllumination[i].b << '\n';
    //     }
    // }

}
