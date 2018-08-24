/**
 * @file   AOPCOnMeshRenderer.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.17
 *
 * @brief  Definition of the point cloud renderer for ambient occlusion.
 */

#include "AOPCOnMeshRenderer.h"
#include <glm/gtc/type_ptr.hpp>
#include "app/ApplicationNodeImplementation.h"
#include "enh/gfx/env/EnvironmentMapRenderer.h"
#include "app/PointCloudContainer.h"

namespace pcViewer {

    AOPCOnMeshRenderer::AOPCOnMeshRenderer(ApplicationNodeImplementation* appNode) :
        PCOnMeshRenderer{ PCType::AO, appNode }
    {
        distanceSumAOProgram_ = GetApp()->GetGPUProgramManager().GetResource("distanceSumAO", std::vector<std::string>{ "distanceSumAO.vert", "distanceSum.frag" });
        distanceSumAOUniformLocations_ = distanceSumAOProgram_->GetUniformLocations({ "viewProjection", "positionTexture", "normalTexture", "distancePower", "pointSize" });
    }

    void AOPCOnMeshRenderer::DrawPointCloudDistanceSum(const glm::mat4& MVP, const FrameBuffer& deferredFBO)
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
        gl::glUniform1f(distanceSumAOUniformLocations_[4], GetPointSize());
        gl::glUniform3fv(distanceSumAOProgram_->getUniformLocation("camPos"), 1, glm::value_ptr(GetApp()->GetCameraEnh().GetPosition()));
        gl::glDrawArrays(gl::GL_POINTS, 0, static_cast<gl::GLsizei>(GetPointCloudSize()));
    }

    void AOPCOnMeshRenderer::RenderGUIByType()
    {
    }

    void AOPCOnMeshRenderer::ExportScreenPointCloudScreen(const FrameBuffer& fbo, std::ostream& screenPoints) const
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
