/**
 * @file   Renderer.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.17
 *
 * @brief  Implementation of the base renderer class.
 */

#include "Renderer.h"
#include <glm/gtc/type_ptr.hpp>
#include "core/CameraHelper.h"
#include "core/gfx/FrameBuffer.h"
#include "enh/gfx/env/EnvironmentMapRenderer.h"
#include "app/camera/ArcballCameraEnhanced.h"
#include "app/ApplicationNodeImplementation.h"
#include <imgui.h>
#include <fstream>

namespace pcViewer {


    BaseRenderer::BaseRenderer(PCType pcType, ApplicationNodeImplementation* appNode) :
        pcType_{ pcType }, appNode_{ appNode }, deferredExportFBO_{ std::make_unique<FrameBuffer>(1920, 1080, appNode->GetDeferredFBODesc()) }
    {
        deferredProgram_ = appNode->GetGPUProgramManager().GetResource("deferredMesh", std::vector<std::string>{ "deferredMesh.vert", "deferredMesh.frag" });
        deferredUniformLocations_ = deferredProgram_->GetUniformLocations({ "viewProjection" });

        finalQuad_ = std::make_unique<FullscreenQuad>("finalComposite.frag", appNode_);
        finalUniformLocations_ = finalQuad_->GetGPUProgram()->GetUniformLocations({ "positionTexture", "normalTexture", "materialColorTexture", "directIlluminationTexture", "globalIlluminationTexture" });

        envMapRenderer_ = std::make_unique<enh::EnvironmentMapRenderer>(appNode_);
    }

    void BaseRenderer::LoadPointCloud(const std::string & pointCloud)
    {
        std::ifstream pc_in(pointCloud);
        ClearPointCloud();
        GetApp()->ClearRadius();

        std::string line;
        while (pc_in.good()) {
            std::getline(pc_in, line);

            auto splitPointData = utils::split(line, ',');

            StorePointInPointCloud(splitPointData);
        }

        LoadPointCloudGPU();
    }

    void BaseRenderer::DrawPointCloud(const FrameBuffer& fbo, const FrameBuffer& deferredFBO, bool batched)
    {
        if (envMap_) {
            fbo.DrawToFBO([this]() {
                envMapRenderer_->Draw(*appNode_->GetCamera(), envMap_->getTextureId());
            });
        }

        if (!renderModel_ || !mesh_) {
            fbo.DrawToFBO([this, batched]() {
                DrawPointCloudPointsInternal(batched);
            });
        }
        else {
            DrawMeshPointCloudPoints(fbo, deferredFBO);
        }
    }

    void BaseRenderer::ExportScreenPointCloud(std::ostream& screenPoints, std::ostream& meshPoints)
    {
        GetApp()->ClearExportScreenPointCloud(GetDeferredExportFBO());

        GetDeferredExportFBO().DrawToFBO(GetApp()->GetDeferredDrawIndices(), [this]() {
            DrawMeshDeferred();
        });

        ExportScreenPointCloudScreen(GetDeferredExportFBO(), screenPoints);

        gl::glBindTexture(gl::GL_TEXTURE_2D, 0);
        gl::glBindBuffer(gl::GL_PIXEL_PACK_BUFFER, 0);

        ExportScreenPointCloudMesh(meshPoints);
    }

    void BaseRenderer::RenderGUI()
    {
        if (HasMesh()) {
            ImGui::Checkbox("Render Model", &renderModel_);
        }

        RenderGUIByType();

        if (ImGui::Button("Export Screen")) {
            std::ofstream screenPoints{ "screen_export.txt" }, meshPoints{ "mesh_export.txt" };
        
            ExportScreenPointCloud(screenPoints, meshPoints);
        }
    }

    void BaseRenderer::DrawPointCloudPointsInternal(bool batched)
    {
        gl::glEnable(gl::GL_PROGRAM_POINT_SIZE);
        gl::glEnable(gl::GL_BLEND);
        gl::glBlendFunc(gl::GL_SRC_ALPHA, gl::GL_ONE_MINUS_SRC_ALPHA);

        gl::glBindVertexArray(vaoPointCloud_);
        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, vboPointCloud_);

        glm::mat4 modelMatrix(1.0f);
        auto MVP = appNode_->GetCamera()->GetViewPerspectiveMatrix() * modelMatrix;

        DrawPointCloudPoints(MVP, appNode_->GetCameraEnh().GetPosition(), batched);

        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, 0);
        gl::glBindVertexArray(0);
        gl::glUseProgram(0);


        gl::glDisable(gl::GL_BLEND);
        gl::glDisable(gl::GL_PROGRAM_POINT_SIZE);
    }

    void BaseRenderer::DrawMeshPointCloudPoints(const FrameBuffer& fbo, const FrameBuffer& deferredFBO)
    {
        deferredFBO.DrawToFBO(GetApp()->GetDeferredDrawIndices(), [this]() {
            DrawMeshDeferred();
        });

        deferredFBO.DrawToFBO(GetApp()->GetDistanceSumDrawIndices(), [this, &deferredFBO]() {
            DrawPointCloudDistanceSumInternal(deferredFBO);
        });

        fbo.DrawToFBO([this, &deferredFBO]() {
            DrawPointCloudOnMesh(deferredFBO);
        });
    }

    void BaseRenderer::DrawMeshDeferred()
    {
        auto VP = appNode_->GetCamera()->GetViewPerspectiveMatrix();

        gl::glDisable(gl::GL_CULL_FACE);
        glm::mat4 modelMatrix(10.f);
        modelMatrix[3][3] = 1.f;
        gl::glUseProgram(deferredProgram_->getProgramId());
        gl::glUniformMatrix4fv(deferredUniformLocations_[0], 1, gl::GL_FALSE, glm::value_ptr(VP));
        gl::glUniform3fv(deferredProgram_->getUniformLocation("camPos"), 1, glm::value_ptr(appNode_->GetCameraEnh().GetPosition()));
        meshRenderable_->Draw(meshModel_ * modelMatrix);

        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, 0);
        gl::glBindVertexArray(0);
        gl::glUseProgram(0);

        gl::glEnable(gl::GL_CULL_FACE);
    }

    void BaseRenderer::DrawPointCloudDistanceSumInternal(const FrameBuffer& deferredFBO)
    {
        gl::glEnable(gl::GL_PROGRAM_POINT_SIZE);

        gl::glBindVertexArray(vaoPointCloud_);
        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, vboPointCloud_);

        gl::glDisable(gl::GL_DEPTH_TEST);
        gl::glDepthMask(gl::GL_FALSE);
        gl::glEnable(gl::GL_BLEND);
        gl::glBlendEquationSeparate(gl::GL_FUNC_ADD, gl::GL_FUNC_ADD);
        gl::glBlendFuncSeparate(gl::GL_ONE, gl::GL_ONE, gl::GL_ONE, gl::GL_ONE);

        glm::mat4 modelMatrix(1.0f);
        auto MVP = appNode_->GetCamera()->GetViewPerspectiveMatrix() * modelMatrix;
        DrawPointCloudDistanceSum(MVP, deferredFBO);

        gl::glDisable(gl::GL_BLEND);
        gl::glDepthMask(gl::GL_TRUE);
        gl::glEnable(gl::GL_DEPTH_TEST);

        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, 0);
        gl::glBindVertexArray(0);
        gl::glUseProgram(0);

        gl::glDisable(gl::GL_PROGRAM_POINT_SIZE);
    }

    void BaseRenderer::DrawPointCloudOnMesh(const FrameBuffer & deferredFBO)
    {
        gl::glUseProgram(finalQuad_->GetGPUProgram()->getProgramId());

        for (int i = 0; i < 5; ++i) {
            gl::glActiveTexture(gl::GL_TEXTURE0 + i);
            gl::glBindTexture(gl::GL_TEXTURE_2D, deferredFBO.GetTextures()[i]);
            gl::glUniform1i(finalUniformLocations_[i], i);
        }

        gl::glEnable(gl::GL_BLEND);
        gl::glBlendEquationSeparate(gl::GL_FUNC_ADD, gl::GL_FUNC_ADD);
        gl::glBlendFuncSeparate(gl::GL_SRC_ALPHA, gl::GL_ONE_MINUS_SRC_ALPHA, gl::GL_ONE, gl::GL_ONE);
        // gl::glUniformMatrix4fv(deferredUniformLocations_[0], 1, gl::GL_FALSE, glm::value_ptr(VP));
        finalQuad_->Draw();

        gl::glDisable(gl::GL_BLEND);
        gl::glUseProgram(0);
    }

    void BaseRenderer::SetMesh(std::shared_ptr<Mesh> mesh, float theta, float phi)
    {
        mesh_ = std::move(mesh);
        if (mesh_) meshRenderable_ = enh::MeshRenderable::create<SimpleMeshVertex>(mesh_.get(), deferredProgram_.get());

        meshModel_ = glm::rotate(glm::mat4(1.0f), theta, glm::vec3(1.0f, 0.0f, 0.0f));
        meshModel_ = meshModel_ * glm::rotate(glm::mat4(1.0f), phi, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    void BaseRenderer::SetEnvironmentMap(std::shared_ptr<Texture> envMap)
    {
        envMap_ = std::move(envMap);

        gl::glBindTexture(gl::GL_TEXTURE_2D, envMap_->getTextureId());
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_S, gl::GL_REPEAT);
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_T, gl::GL_REPEAT);
        gl::glBindTexture(gl::GL_TEXTURE_2D, 0);
    }

    void BaseRenderer::PreparePointCloudVAOInternal()
    {
        gl::glBindVertexArray(vaoPointCloud_);
        PreparePointCloudVAO();
        gl::glBindVertexArray(0);
        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, 0);

        appNode_->GetCameraEnh().SetCameraPosition(appNode_->GetBoundingSphereRadius() * appNode_->GetCamera()->GetCentralPerspectiveMatrix()[1][1] * glm::normalize(appNode_->GetCameraEnh().GetPosition()));
    }

    float BaseRenderer::GetBoundingSphereRadius() const
    {
        return appNode_->GetBoundingSphereRadius();
    }

    float BaseRenderer::GetDistancePower() const
    {
        return const_cast<const ApplicationNodeImplementation*>(appNode_)->GetDistancePower();
    }

    enh::BufferRAII BaseRenderer::CopyTextureToPixelBuffer3(const FrameBuffer & deferredFBO, std::size_t tex)
    {
        enh::BufferRAII pbo;
        gl::glBindBuffer(gl::GL_PIXEL_PACK_BUFFER, pbo);
        gl::glBufferData(gl::GL_PIXEL_PACK_BUFFER, static_cast<std::size_t>(deferredFBO.GetWidth()) * deferredFBO.GetHeight() * sizeof(glm::vec3), nullptr, gl::GL_STREAM_READ);

        gl::glBindTexture(gl::GL_TEXTURE_2D, deferredFBO.GetTextures()[tex]);
        gl::glGetTexImage(gl::GL_TEXTURE_2D, 0, gl::GL_RGB, gl::GL_FLOAT, nullptr);
        return std::move(pbo);
    }

    enh::BufferRAII BaseRenderer::CopyTextureToPixelBuffer4(const FrameBuffer & deferredFBO, std::size_t tex)
    {
        enh::BufferRAII pbo;
        gl::glBindBuffer(gl::GL_PIXEL_PACK_BUFFER, pbo);
        gl::glBufferData(gl::GL_PIXEL_PACK_BUFFER, static_cast<std::size_t>(deferredFBO.GetWidth()) * deferredFBO.GetHeight() * sizeof(glm::vec4), nullptr, gl::GL_STREAM_READ);

        gl::glBindTexture(gl::GL_TEXTURE_2D, deferredFBO.GetTextures()[tex]);
        gl::glGetTexImage(gl::GL_TEXTURE_2D, 0, gl::GL_RGBA, gl::GL_FLOAT, nullptr);
        return std::move(pbo);
    }

    void BaseRenderer::CopyPBOToVector3(gl::GLuint pbo, std::vector<glm::vec3>& content)
    {
        gl::glBindBuffer(gl::GL_PIXEL_PACK_BUFFER, pbo);
        auto gpuMem = gl::glMapBuffer(gl::GL_PIXEL_PACK_BUFFER, gl::GL_READ_ONLY);
        if (gpuMem) {
            memcpy(content.data(), gpuMem, content.size() * sizeof(glm::vec3));
            gl::glUnmapBuffer(gl::GL_PIXEL_PACK_BUFFER);
        }
    }

    void BaseRenderer::CopyPBOToVector4(gl::GLuint pbo, std::vector<glm::vec4>& content)
    {
        gl::glBindBuffer(gl::GL_PIXEL_PACK_BUFFER, pbo);
        auto gpuMem = gl::glMapBuffer(gl::GL_PIXEL_PACK_BUFFER, gl::GL_READ_ONLY);
        if (gpuMem) {
            memcpy(content.data(), gpuMem, content.size() * sizeof(glm::vec4));
            gl::glUnmapBuffer(gl::GL_PIXEL_PACK_BUFFER);
        }
    }

}
