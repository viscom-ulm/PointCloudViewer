/**
 * @file   MeshRenderer.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.24
 *
 * @brief  Implementation of the base mesh renderer class.
 */

#include "app/meshRenderers/MeshRenderer.h"
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

namespace pcViewer {


    MeshRenderer::MeshRenderer(PCType pcType, ApplicationNodeImplementation* appNode) :
        BaseRenderer{ pcType, "Mesh", appNode }
    {
        finalQuad_ = std::make_unique<FullscreenQuad>("finalComposite.frag", GetApp());
        finalUniformLocations_ = finalQuad_->GetGPUProgram()->GetUniformLocations({ "positionTexture", "normalTexture", "materialColorTexture", "directIlluminationTexture", "globalIlluminationTexture", "compositionType" });
    }

    void MeshRenderer::DrawPointCloudInternal(const FrameBuffer& fbo, const FrameBuffer& deferredFBO, bool batched)
    {
        deferredFBO.DrawToFBO(GetApp()->GetDeferredDrawIndices(), [this]() {
            GetMesh()->DrawMeshDeferred();
        });

        fbo.DrawToFBO([this, &deferredFBO]() {
            gl::glUseProgram(finalQuad_->GetGPUProgram()->getProgramId());

            for (int i = 0; i < 5; ++i) {
                gl::glActiveTexture(gl::GL_TEXTURE0 + i);
                gl::glBindTexture(gl::GL_TEXTURE_2D, deferredFBO.GetTextures()[i]);
                gl::glUniform1i(finalUniformLocations_[i], i);
            }

            gl::glUniform1i(finalUniformLocations_[5], GetApp()->GetCompositeType());

            gl::glEnable(gl::GL_BLEND);
            gl::glBlendEquationSeparate(gl::GL_FUNC_ADD, gl::GL_FUNC_ADD);
            gl::glBlendFuncSeparate(gl::GL_SRC_ALPHA, gl::GL_ONE_MINUS_SRC_ALPHA, gl::GL_ONE, gl::GL_ONE);
            // gl::glUniformMatrix4fv(deferredUniformLocations_[0], 1, gl::GL_FALSE, glm::value_ptr(VP));
            finalQuad_->Draw();

            gl::glDisable(gl::GL_BLEND);
            gl::glUseProgram(0);
        });
    }

}
