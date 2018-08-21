/**
 * @file   AOPointCloudRenderer.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.17
 *
 * @brief  Definition of the point cloud renderer for ambient occlusion.
 */

#include "AOPointCloudRenderer.h"
#include <glm/gtc/type_ptr.hpp>
#include "app/ApplicationNodeImplementation.h"
#include "enh/gfx/env/EnvironmentMapRenderer.h"

namespace pcViewer {

    AOPointCloudRenderer::AOPointCloudRenderer(ApplicationNodeImplementation* appNode) :
        TBaseRenderer{ PCType::AO, appNode }
    {
        aoProgram_ = GetApp()->GetGPUProgramManager().GetResource("ao", std::vector<std::string>{ "showAO.vert", "showPCResult.frag" });
        aoMVPLoc_ = aoProgram_->getUniformLocation("MVP");
        aoBBRLoc_ = aoProgram_->getUniformLocation("bbRadius");

        distanceSumAOProgram_ = GetApp()->GetGPUProgramManager().GetResource("distanceSumAO", std::vector<std::string>{ "distanceSumAO.vert", "distanceSum.frag" });
        distanceSumAOUniformLocations_ = distanceSumAOProgram_->GetUniformLocations({ "viewProjection", "positionTexture", "normalTexture", "distancePower", "pointSize" });
    }

    void AOPointCloudRenderer::StorePointInPointCloud(const std::vector<std::string>& pointData)
    {
        if (pointData.size() < 7) return;

        auto& point = EditNextPoint();
        point.position_.x = static_cast<float>(std::atof(pointData[0].c_str()));
        point.position_.y = static_cast<float>(std::atof(pointData[1].c_str()));
        point.position_.z = static_cast<float>(std::atof(pointData[2].c_str()));
        GetApp()->AddToBoundingSphere(point.position_);

        point.normal_.x = static_cast<float>(std::atof(pointData[3].c_str()));
        point.normal_.y = static_cast<float>(std::atof(pointData[4].c_str()));
        point.normal_.z = static_cast<float>(std::atof(pointData[5].c_str()));

        point.ao_ = static_cast<float>(std::atof(pointData[6].c_str()));
    }

    void AOPointCloudRenderer::PreparePointCloudVAO()
    {
        using PointCloudPoint = PointCloudPointAO;

        gl::glEnableVertexAttribArray(0);
        gl::glVertexAttribPointer(0, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPoint), reinterpret_cast<GLvoid*>(offsetof(PointCloudPoint, position_)));
        gl::glEnableVertexAttribArray(1);
        gl::glVertexAttribPointer(1, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPoint), reinterpret_cast<GLvoid*>(offsetof(PointCloudPoint, normal_)));
        gl::glEnableVertexAttribArray(2);
        gl::glVertexAttribPointer(2, 1, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPoint), reinterpret_cast<GLvoid*>(offsetof(PointCloudPoint, ao_)));
    }

    void AOPointCloudRenderer::DrawPointCloudPoints(const glm::mat4& MVP, const glm::vec3& camPos, bool batched)
    {
        gl::glUseProgram(aoProgram_->getProgramId());
        gl::glUniformMatrix4fv(aoMVPLoc_, 1, gl::GL_FALSE, glm::value_ptr(MVP));
        gl::glUniform1f(aoBBRLoc_, batched ? GetBoundingSphereRadius() / 2.f : GetBoundingSphereRadius());
        gl::glUniform3fv(aoProgram_->getUniformLocation("camPos"), 1, glm::value_ptr(camPos));
        gl::glDrawArrays(gl::GL_POINTS, 0, static_cast<gl::GLsizei>(GetPointCloud().size()));
    }

    void AOPointCloudRenderer::DrawPointCloudDistanceSum(const glm::mat4& MVP, const FrameBuffer& deferredFBO)
    {
        gl::glUseProgram(distanceSumAOProgram_->getProgramId());
        gl::glUniformMatrix4fv(distanceSumAOUniformLocations_[0], 1, gl::GL_FALSE, glm::value_ptr(MVP));

        gl::glActiveTexture(gl::GL_TEXTURE2);
        gl::glBindTexture(gl::GL_TEXTURE_2D, deferredFBO.GetTextures()[0]);
        gl::glUniform1i(distanceSumAOUniformLocations_[1], 2);

        gl::glActiveTexture(gl::GL_TEXTURE3);
        gl::glBindTexture(gl::GL_TEXTURE_2D, deferredFBO.GetTextures()[1]);
        gl::glUniform1i(distanceSumAOUniformLocations_[2], 3);
        gl::glUniform1f(distanceSumAOUniformLocations_[3], GetDistancePower());
        gl::glUniform3fv(distanceSumAOProgram_->getUniformLocation("camPos"), 1, glm::value_ptr(GetApp()->GetCameraEnh().GetPosition()));
        gl::glDrawArrays(gl::GL_POINTS, 0, static_cast<gl::GLsizei>(GetPointCloud().size()));
    }

    void AOPointCloudRenderer::RenderGUIByType()
    {
    }

    void AOPointCloudRenderer::ExportScreenPointCloudScreen(const FrameBuffer& fbo, std::ostream& screenPoints) const
    {
        std::vector<glm::vec3> screenPositions, screenNormals;
        screenPositions.resize(static_cast<std::size_t>(fbo.GetWidth()) * fbo.GetHeight());
        screenNormals.resize(static_cast<std::size_t>(fbo.GetWidth()) * fbo.GetHeight());

        auto pboPos = CopyTextureToPixelBuffer3(fbo, 0);
        auto pboNormal = CopyTextureToPixelBuffer3(fbo, 1);

        gl::glMemoryBarrier(gl::GL_ALL_BARRIER_BITS);

        CopyPBOToVector3(pboPos, screenPositions);
        CopyPBOToVector3(pboNormal, screenNormals);

        for (std::size_t i = 0; i < screenPositions.size(); ++i) {
            screenPoints << screenPositions[i].x << ',' << screenPositions[i].y << ',' << screenPositions[i].z << ','
                << screenNormals[i].x << ',' << screenNormals[i].y << ',' << screenNormals[i].z << '\n';
        }
    }

}
