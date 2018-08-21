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
#include <imgui.h>

namespace pcViewer {

    SSSPointCloudRenderer::SSSPointCloudRenderer(ApplicationNodeImplementation* appNode) :
        TBaseRenderer{ PCType::SUBSURFACE, appNode }
    {
        subsurfaceProgram_ = GetApp()->GetGPUProgramManager().GetResource("subsurface", std::vector<std::string>{ "showSubsurface.vert", "showPCResult.frag" });
        subsurfaceMVPLoc_ = subsurfaceProgram_->getUniformLocation("MVP");
        subsurfaceRenderTypeLoc_ = subsurfaceProgram_->getUniformLocation("renderType");
        subsurfaceBBRLoc_ = subsurfaceProgram_->getUniformLocation("bbRadius");

        distanceSumSubsurfaceProgram_ = GetApp()->GetGPUProgramManager().GetResource("distanceSumSubsurface", std::vector<std::string>{ "distanceSumSubsurface.vert", "distanceSum.frag" });
        distanceSumSubsurfaceUniformLocations_ = distanceSumSubsurfaceProgram_->GetUniformLocations({ "viewProjection", "positionTexture", "normalTexture", "distancePower", "pointSize" });
    }

    void SSSPointCloudRenderer::StorePointInPointCloud(const std::vector<std::string>& pointData)
    {
        if (pointData.size() < 19) return;

        auto& point = EditNextPoint();
        point.position_.x = static_cast<float>(std::atof(pointData[0].c_str()));
        point.position_.y = static_cast<float>(std::atof(pointData[1].c_str()));
        point.position_.z = static_cast<float>(std::atof(pointData[2].c_str()));
        GetApp()->AddToBoundingSphere(point.position_);

        point.normal_.x = static_cast<float>(std::atof(pointData[3].c_str()));
        point.normal_.y = static_cast<float>(std::atof(pointData[4].c_str()));
        point.normal_.z = static_cast<float>(std::atof(pointData[5].c_str()));

        point.directIllumination_.x = static_cast<float>(std::atof(pointData[6].c_str()));
        point.directIllumination_.y = static_cast<float>(std::atof(pointData[7].c_str()));
        point.directIllumination_.z = static_cast<float>(std::atof(pointData[8].c_str()));

        point.albedo_.x = static_cast<float>(std::atof(pointData[9].c_str()));
        point.albedo_.y = static_cast<float>(std::atof(pointData[10].c_str()));
        point.albedo_.z = static_cast<float>(std::atof(pointData[11].c_str()));

        point.sigma_tp_.x = static_cast<float>(std::atof(pointData[12].c_str()));
        point.sigma_tp_.y = static_cast<float>(std::atof(pointData[13].c_str()));
        point.sigma_tp_.z = static_cast<float>(std::atof(pointData[14].c_str()));

        point.eta_ = static_cast<float>(std::atof(pointData[15].c_str()));

        point.globalIllumination_.x = static_cast<float>(std::atof(pointData[16].c_str()));
        point.globalIllumination_.y = static_cast<float>(std::atof(pointData[17].c_str()));
        point.globalIllumination_.z = static_cast<float>(std::atof(pointData[18].c_str()));
    }

    void SSSPointCloudRenderer::PreparePointCloudVAO()
    {
        using PointCloudPoint = PointCloudPointSubsurface;

        gl::glEnableVertexAttribArray(0);
        gl::glVertexAttribPointer(0, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPoint), reinterpret_cast<GLvoid*>(offsetof(PointCloudPoint, position_)));
        gl::glEnableVertexAttribArray(1);
        gl::glVertexAttribPointer(1, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPoint), reinterpret_cast<GLvoid*>(offsetof(PointCloudPoint, normal_)));
        gl::glEnableVertexAttribArray(2);
        gl::glVertexAttribPointer(2, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPoint), reinterpret_cast<GLvoid*>(offsetof(PointCloudPoint, albedo_)));
        gl::glEnableVertexAttribArray(3);
        gl::glVertexAttribPointer(3, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPoint), reinterpret_cast<GLvoid*>(offsetof(PointCloudPoint, sigma_tp_)));
        gl::glEnableVertexAttribArray(4);
        gl::glVertexAttribPointer(4, 1, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPoint), reinterpret_cast<GLvoid*>(offsetof(PointCloudPoint, eta_)));
        gl::glEnableVertexAttribArray(5);
        gl::glVertexAttribPointer(5, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPoint), reinterpret_cast<GLvoid*>(offsetof(PointCloudPoint, directIllumination_)));
        gl::glEnableVertexAttribArray(6);
        gl::glVertexAttribPointer(6, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPoint), reinterpret_cast<GLvoid*>(offsetof(PointCloudPoint, globalIllumination_)));
    }

    void SSSPointCloudRenderer::DrawPointCloudPoints(const glm::mat4& MVP, const glm::vec3& camPos, bool batched)
    {
        gl::glUseProgram(subsurfaceProgram_->getProgramId());
        gl::glUniformMatrix4fv(subsurfaceMVPLoc_, 1, gl::GL_FALSE, glm::value_ptr(MVP));
        gl::glUniform1i(subsurfaceRenderTypeLoc_, GetApp()->GetSubsurfaceRenderType());
        gl::glUniform1f(subsurfaceBBRLoc_, batched ? GetBoundingSphereRadius() / 2.f : GetBoundingSphereRadius());
        gl::glUniform3fv(subsurfaceProgram_->getUniformLocation("camPos"), 1, glm::value_ptr(camPos));
        gl::glDrawArrays(gl::GL_POINTS, 0, static_cast<gl::GLsizei>(GetPointCloud().size()));
    }

    void SSSPointCloudRenderer::DrawPointCloudDistanceSum(const glm::mat4& MVP, const FrameBuffer& deferredFBO)
    {
        gl::glUseProgram(distanceSumSubsurfaceProgram_->getProgramId());
        gl::glUniformMatrix4fv(distanceSumSubsurfaceUniformLocations_[0], 1, gl::GL_FALSE, glm::value_ptr(MVP));

        gl::glActiveTexture(gl::GL_TEXTURE2);
        gl::glBindTexture(gl::GL_TEXTURE_2D, deferredFBO.GetTextures()[0]);
        gl::glUniform1i(distanceSumSubsurfaceUniformLocations_[1], 2);

        gl::glActiveTexture(gl::GL_TEXTURE3);
        gl::glBindTexture(gl::GL_TEXTURE_2D, deferredFBO.GetTextures()[1]);
        gl::glUniform1i(distanceSumSubsurfaceUniformLocations_[2], 3);
        gl::glUniform1f(distanceSumSubsurfaceUniformLocations_[3], GetDistancePower());
        gl::glUniform3fv(distanceSumSubsurfaceProgram_->getUniformLocation("camPos"), 1, glm::value_ptr(GetApp()->GetCameraEnh().GetPosition()));
        gl::glDrawArrays(gl::GL_POINTS, 0, static_cast<gl::GLsizei>(GetPointCloud().size()));
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
