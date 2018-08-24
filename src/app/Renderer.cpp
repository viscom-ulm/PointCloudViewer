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
#include "app/PointCloudContainer.h"
#include "app/MeshContainer.h"
#include <imgui.h>
#include <fstream>

namespace pcViewer {


    BaseRenderer::BaseRenderer(PCType pcType, const std::string& rendererName, ApplicationNodeImplementation* appNode) :
        pcType_{ pcType }, rendererName_{ rendererName }, appNode_{ appNode }, deferredExportFBO_{ std::make_unique<FrameBuffer>(1920, 1080, appNode->GetDeferredFBODesc()) }
    {
        envMapRenderer_ = std::make_unique<enh::EnvironmentMapRenderer>(appNode_);
    }

    void BaseRenderer::SetPointCloud(BasePointCloudContainer* pointCloud)
    {
        if (!pointCloud) {
            pointCloud_ = nullptr;
            return;
        }

        if (pointCloud->GetPointCloudType() != pcType_) throw std::runtime_error("PointCloudType mismatch!");
        pointCloud_ = pointCloud;
    }

    void BaseRenderer::DrawPointCloud(const FrameBuffer& fbo, const FrameBuffer& deferredFBO, bool batched)
    {
        if (envMap_) {
            fbo.DrawToFBO([this]() {
                envMapRenderer_->Draw(*appNode_->GetCamera(), envMap_->getTextureId());
            });
        }

        DrawPointCloudInternal(fbo, deferredFBO, batched);
    }

    void BaseRenderer::ExportScreenPointCloud(std::ostream& screenPoints, std::ostream& meshPoints)
    {
        GetApp()->ClearExportScreenPointCloud(GetDeferredExportFBO());

        GetDeferredExportFBO().DrawToFBO(GetApp()->GetDeferredDrawIndices(), [this]() {
            mesh_->DrawMeshDeferred();
        });

        ExportScreenPointCloudScreen(GetDeferredExportFBO(), screenPoints);

        gl::glBindTexture(gl::GL_TEXTURE_2D, 0);
        gl::glBindBuffer(gl::GL_PIXEL_PACK_BUFFER, 0);

        if (pointCloud_) pointCloud_->ExportScreenPointCloudMesh(meshPoints);
        else ExportScreenPointCloudMesh(meshPoints);
    }

    void BaseRenderer::RenderGUI()
    {
        RenderGUIByType();

        if (ImGui::RadioButton("Pass through", GetApp()->GetCompositeType() == 0)) GetApp()->SetCompositeType(0);
        if (ImGui::RadioButton("Direct Only", GetApp()->GetCompositeType() == 1)) GetApp()->SetCompositeType(1);
        if (ImGui::RadioButton("Combine all", GetApp()->GetCompositeType() == 2)) GetApp()->SetCompositeType(2);

        if (ImGui::Button("Export Screen")) {
            std::ofstream screenPoints{ pointCloud_->GetPointCloudName() + "_screen_export.txt" }, meshPoints{ pointCloud_->GetPointCloudName() + "mesh_export.txt" };
        
            ExportScreenPointCloud(screenPoints, meshPoints);
        }
    }

    void BaseRenderer::SetEnvironmentMap(std::shared_ptr<Texture> envMap)
    {
        envMap_ = std::move(envMap);

        gl::glBindTexture(gl::GL_TEXTURE_2D, envMap_->getTextureId());
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_S, gl::GL_REPEAT);
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_T, gl::GL_REPEAT);
        gl::glBindTexture(gl::GL_TEXTURE_2D, 0);
    }

    float BaseRenderer::GetBoundingSphereRadius() const
    {
        return appNode_->GetBoundingSphereRadius();
    }

    float BaseRenderer::GetDistancePower() const
    {
        return const_cast<const ApplicationNodeImplementation*>(appNode_)->GetDistancePower();
    }

    float BaseRenderer::GetPointSize() const
    {
        return const_cast<const ApplicationNodeImplementation*>(appNode_)->GetPointSize();
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

    std::size_t BaseRenderer::GetPointCloudSize() const
    {
        return pointCloud_->GetPointCloudSize();
    }

}
