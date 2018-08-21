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
#include <imgui.h>

namespace pcViewer {

    GIPointCloudRenderer::GIPointCloudRenderer(ApplicationNodeImplementation* appNode) :
        TBaseRenderer{ PCType::MATTE, appNode }
    {
        matteProgram_ = GetApp()->GetGPUProgramManager().GetResource("matte", std::vector<std::string>{ "showMatte.vert", "showPCResult.frag" });
        matteMVPLoc_ = matteProgram_->getUniformLocation("MVP");
        matteRenderTypeLoc_ = matteProgram_->getUniformLocation("renderType");
        matteBBRLoc_ = matteProgram_->getUniformLocation("bbRadius");

        distanceSumMatteProgram_ = GetApp()->GetGPUProgramManager().GetResource("distanceSumMatte", std::vector<std::string>{ "distanceSumMatte.vert", "distanceSum.frag" });
        distanceSumMatteUniformLocations_ = distanceSumMatteProgram_->GetUniformLocations({ "viewProjection", "positionTexture", "normalTexture", "distancePower", "pointSize" });
    }

    void GIPointCloudRenderer::StorePointInPointCloud(const std::vector<std::string>& pointData)
    {
        if (pointData.size() < 15) return;

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

        point.globalIllumination_.x = static_cast<float>(std::atof(pointData[12].c_str()));
        point.globalIllumination_.y = static_cast<float>(std::atof(pointData[13].c_str()));
        point.globalIllumination_.z = static_cast<float>(std::atof(pointData[14].c_str()));
    }

    void GIPointCloudRenderer::PreparePointCloudVAO()
    {
        using PointCloudPoint = PointCloudPointMatte;

        gl::glEnableVertexAttribArray(0);
        gl::glVertexAttribPointer(0, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPoint), reinterpret_cast<GLvoid*>(offsetof(PointCloudPoint, position_)));
        gl::glEnableVertexAttribArray(1);
        gl::glVertexAttribPointer(1, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPoint), reinterpret_cast<GLvoid*>(offsetof(PointCloudPoint, normal_)));
        gl::glEnableVertexAttribArray(2);
        gl::glVertexAttribPointer(2, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPoint), reinterpret_cast<GLvoid*>(offsetof(PointCloudPoint, albedo_)));
        gl::glEnableVertexAttribArray(3);
        gl::glVertexAttribPointer(3, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPoint), reinterpret_cast<GLvoid*>(offsetof(PointCloudPoint, directIllumination_)));
        gl::glEnableVertexAttribArray(4);
        gl::glVertexAttribPointer(4, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPoint), reinterpret_cast<GLvoid*>(offsetof(PointCloudPoint, globalIllumination_)));
    }

    void GIPointCloudRenderer::DrawPointCloudPoints(const glm::mat4& MVP, const glm::vec3& camPos, bool batched)
    {
        gl::glUseProgram(matteProgram_->getProgramId());
        gl::glUniformMatrix4fv(matteMVPLoc_, 1, gl::GL_FALSE, glm::value_ptr(MVP));
        gl::glUniform1i(matteRenderTypeLoc_, GetApp()->GetMatteRenderType());
        gl::glUniform1f(matteBBRLoc_, batched ? GetBoundingSphereRadius() / 2.f : GetBoundingSphereRadius());
        gl::glUniform3fv(matteProgram_->getUniformLocation("camPos"), 1, glm::value_ptr(camPos));
        gl::glDrawArrays(gl::GL_POINTS, 0, static_cast<gl::GLsizei>(GetPointCloud().size()));
    }

    void GIPointCloudRenderer::DrawPointCloudDistanceSum(const glm::mat4& MVP, const FrameBuffer& deferredFBO)
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
        gl::glUniform3fv(distanceSumMatteProgram_->getUniformLocation("camPos"), 1, glm::value_ptr(GetApp()->GetCameraEnh().GetPosition()));
        gl::glDrawArrays(gl::GL_POINTS, 0, static_cast<gl::GLsizei>(GetPointCloud().size()));
    }

    void GIPointCloudRenderer::RenderGUIByType()
    {
        if (ImGui::RadioButton("Global Illumination", GetApp()->GetMatteRenderType() == 0)) GetApp()->SetMatteRenderType(0);
        if (ImGui::RadioButton("Matte Albedo", GetApp()->GetMatteRenderType() == 1)) GetApp()->SetMatteRenderType(1);
        if (ImGui::RadioButton("Direct Illumination", GetApp()->GetMatteRenderType() == 2)) GetApp()->SetMatteRenderType(2);
    }

    void GIPointCloudRenderer::ExportScreenPointCloudScreen(const FrameBuffer& fbo, std::ostream& screenPoints) const
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
