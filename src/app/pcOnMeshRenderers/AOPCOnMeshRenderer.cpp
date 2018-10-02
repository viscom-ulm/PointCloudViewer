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

#define STBI_MSC_SECURE_CRT
#include <stb_image_write.h>

namespace pcViewer {

    AOPCOnMeshRenderer::AOPCOnMeshRenderer(ApplicationNodeImplementation* appNode) :
        PCOnMeshRenderer{ PCType::AO, appNode }
    {
        distanceSumAOProgram_ = GetApp()->GetGPUProgramManager().GetResource("distanceSumAO", std::vector<std::string>{ "distanceSumAO.vert", "distanceSum.frag" });
        distanceSumAOUniformLocations_ = distanceSumAOProgram_->GetUniformLocations({ "viewProjection", "positionTexture", "normalTexture", "distancePower", "pointSize", "outputDirectLight", "modelMatrix" });
    }

    void AOPCOnMeshRenderer::DrawPointCloudDistanceSum(const glm::mat4& VP, const glm::mat4& M, const FrameBuffer& deferredFBO)
    {
        gl::glUseProgram(distanceSumAOProgram_->getProgramId());
        gl::glUniformMatrix4fv(distanceSumAOUniformLocations_[0], 1, gl::GL_FALSE, glm::value_ptr(VP));
        gl::glUniformMatrix4fv(distanceSumAOUniformLocations_[6], 1, gl::GL_FALSE, glm::value_ptr(M));

        gl::glActiveTexture(gl::GL_TEXTURE2);
        gl::glBindTexture(gl::GL_TEXTURE_2D, deferredFBO.GetTextures()[0]);
        gl::glUniform1i(distanceSumAOUniformLocations_[1], 2);

        gl::glActiveTexture(gl::GL_TEXTURE3);
        gl::glBindTexture(gl::GL_TEXTURE_2D, deferredFBO.GetTextures()[1]);
        gl::glUniform1i(distanceSumAOUniformLocations_[2], 3);
        gl::glUniform1f(distanceSumAOUniformLocations_[3], GetDistancePower());
        gl::glUniform1f(distanceSumAOUniformLocations_[4], GetPointSize());
        gl::glUniform1i(distanceSumAOUniformLocations_[5], 0);
        gl::glUniform3fv(distanceSumAOProgram_->getUniformLocation("camPos"), 1, glm::value_ptr(GetApp()->GetCameraEnh().GetPosition()));
        gl::glDrawArrays(gl::GL_POINTS, 0, static_cast<gl::GLsizei>(GetPointCloudSize()));
    }

    void AOPCOnMeshRenderer::RenderGUIByType()
    {
    }

    // void AOPCOnMeshRenderer::ExportScreenPointCloudScreen(const FrameBuffer& fbo, const std::string& namePrefix, std::ostream& screenPoints) const
    // {
    //     std::vector<glm::vec3> screenPositions, screenNormals, screenAlbedo, screenDirectIllumination;
    //     screenPositions.resize(static_cast<std::size_t>(fbo.GetWidth()) * fbo.GetHeight());
    //     screenNormals.resize(static_cast<std::size_t>(fbo.GetWidth()) * fbo.GetHeight());
    //     screenAlbedo.resize(static_cast<std::size_t>(fbo.GetWidth()) * fbo.GetHeight());
    //     screenDirectIllumination.resize(static_cast<std::size_t>(fbo.GetWidth()) * fbo.GetHeight());
    // 
    //     auto pboPos = CopyTextureToPixelBuffer3(fbo, 0);
    //     auto pboNormal = CopyTextureToPixelBuffer3(fbo, 1);
    //     auto pboAlbedo = CopyTextureToPixelBuffer3(fbo, 2);
    //     auto pboDirectIllumination = CopyTextureToPixelBuffer3(fbo, 4);
    // 
    //     gl::glMemoryBarrier(gl::GL_ALL_BARRIER_BITS);
    // 
    //     CopyPBOToVector3(pboPos, screenPositions);
    //     CopyPBOToVector3(pboNormal, screenNormals);
    //     CopyPBOToVector3(pboAlbedo, screenAlbedo);
    //     CopyPBOToVector3(pboDirectIllumination, screenDirectIllumination);
    // 
    //     stbi_write_hdr((namePrefix + "_pos.hdr").c_str(), fbo.GetWidth(), fbo.GetHeight(), 3, reinterpret_cast<float*>(screenNormals.data()));
    // 
    //     for (std::size_t i = 0; i < screenPositions.size(); ++i) {
    //         screenPoints << screenPositions[i].x << ',' << screenPositions[i].y << ',' << screenPositions[i].z << ','
    //             << screenNormals[i].x << ',' << screenNormals[i].y << ',' << screenNormals[i].z << ','
    //             << screenAlbedo[i].r << ',' << screenAlbedo[i].g << ',' << screenAlbedo[i].b << ','
    //             << screenDirectIllumination[i].r << ',' << screenDirectIllumination[i].g << ',' << screenDirectIllumination[i].b << '\n';
    //     }
    // }

}
