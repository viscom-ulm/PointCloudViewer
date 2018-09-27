/**
 * @file   SSGIRenderer.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.23
 *
 * @brief  Implementation of the base point cloud on mesh renderer class.
 */

#include "SSGIRenderer.h"
#include <glm/gtc/type_ptr.hpp>
#include "core/CameraHelper.h"
#include "core/gfx/FrameBuffer.h"
#include "enh/gfx/env/EnvironmentMapRenderer.h"
#include "app/camera/ArcballCameraEnhanced.h"
#include "app/ApplicationNodeImplementation.h"
#include "app/PointCloudContainer.h"
#include "app/MeshContainer.h"
#include <imgui.h>
#include <fstream>

#include <glm/gtc/random.hpp>
#include <random>

namespace pcViewer {


    SSGIRenderer::SSGIRenderer(ApplicationNodeImplementation* appNode) :
        BaseRenderer{ PCType::MATTE, RenderType::PC_ON_MESH, "SSGI", appNode }
    {
        ssgiQuad_ = std::make_unique<FullscreenQuad>("comparison/ssgi.frag", GetApp());
        ssgiUniformLocations_ = ssgiQuad_->GetGPUProgram()->GetUniformLocations({ "positionTexture", "normalTexture", "materialColorTexture", "directIlluminationTexture", "camPos" });
    }

    bool SSGIRenderer::IsAvaialble() const
    {
        return (GetMesh() != nullptr);
    }

    void SSGIRenderer::DrawPointCloudInternal(const FrameBuffer& fbo, const FrameBuffer& deferredFBO, bool batched)
    {
        deferredFBO.DrawToFBO(GetApp()->GetDeferredDrawIndices(), [this]() {
            GetMesh()->DrawMeshDeferred(true);
        });

        fbo.DrawToFBO([this, &deferredFBO]() {
            DrawHBAO(deferredFBO);
        });
    }

    void SSGIRenderer::DrawHBAO(const FrameBuffer & deferredFBO)
    {
        gl::glUseProgram(ssgiQuad_->GetGPUProgram()->getProgramId());

        std::array<std::size_t, 4> textureIndices{ 0, 1, 2, 4 };
        for (int i = 0; i < 4; ++i) {
            gl::glActiveTexture(gl::GL_TEXTURE0 + i);
            gl::glBindTexture(gl::GL_TEXTURE_2D, deferredFBO.GetTextures()[textureIndices[i]]);
            gl::glUniform1i(ssgiUniformLocations_[i], i);
        }

        gl::glUniform3fv(ssgiUniformLocations_[4], 1, glm::value_ptr(GetApp()->GetCameraEnh().GetPosition()));

        gl::glEnable(gl::GL_BLEND);
        gl::glBlendEquationSeparate(gl::GL_FUNC_ADD, gl::GL_FUNC_ADD);
        gl::glBlendFuncSeparate(gl::GL_SRC_ALPHA, gl::GL_ONE_MINUS_SRC_ALPHA, gl::GL_ONE, gl::GL_ONE);
        // gl::glUniformMatrix4fv(deferredUniformLocations_[0], 1, gl::GL_FALSE, glm::value_ptr(VP));
        ssgiQuad_->Draw();

        gl::glDisable(gl::GL_BLEND);
        gl::glUseProgram(0);

        // GetMesh()->DrawMeshDeferredAndExport(true);
    }

    void SSGIRenderer::ExportScreenPointCloudMesh(std::ostream& meshPoints) const
    {
        throw std::runtime_error("ExportScreenPointCloudMesh: Not implemented!!");
    }

    void SSGIRenderer::ExportScreenPointCloudScreen(const FrameBuffer & fbo, const std::string & namePrefix, std::ostream & screenPoints) const
    {
        throw std::runtime_error("ExportScreenPointCloudScreen: Not implemented!!");
    }

}
