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
#include "app/PointCloudContainer.h"

namespace pcViewer {

    AOPointCloudRenderer::AOPointCloudRenderer(ApplicationNodeImplementation* appNode) :
        PointCloudRenderer{ PCType::AO, appNode }
    {
        aoProgram_ = GetApp()->GetGPUProgramManager().GetResource("ao", std::vector<std::string>{ "showAO.vert", "showPCResult.frag" });
        aoMVPLoc_ = aoProgram_->getUniformLocation("MVP");
        aoBBRLoc_ = aoProgram_->getUniformLocation("bbRadius");
    }

    void AOPointCloudRenderer::DrawPointCloudPoints(const glm::mat4& MVP, const glm::vec3& camPos, bool batched)
    {
        gl::glUseProgram(aoProgram_->getProgramId());
        gl::glUniformMatrix4fv(aoMVPLoc_, 1, gl::GL_FALSE, glm::value_ptr(MVP));
        gl::glUniform1f(aoBBRLoc_, batched ? GetBoundingSphereRadius() / 2.f : GetBoundingSphereRadius());
        gl::glUniform3fv(aoProgram_->getUniformLocation("camPos"), 1, glm::value_ptr(camPos));
        gl::glDrawArrays(gl::GL_POINTS, 0, static_cast<gl::GLsizei>(GetPointCloudSize()));
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
