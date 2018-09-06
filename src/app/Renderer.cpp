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


    BaseRenderer::BaseRenderer(PCType pcType, RenderType renderType, const std::string& rendererName, ApplicationNodeImplementation* appNode) :
        pcType_{ pcType }, rendererType_{ renderType }, rendererName_{ rendererName }, appNode_{ appNode }
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

    void BaseRenderer::ExportScreenPointCloud(const FrameBuffer& deferredExportFBO, std::ostream& info, std::ostream& screenPoints, std::ostream& meshPoints)
    {
        GetApp()->ClearExportScreenPointCloud(deferredExportFBO);

        deferredExportFBO.DrawToFBO(GetApp()->GetDeferredDrawIndices(), [this]() {
            // if (pointCloud_ && pointCloud_->HasDirectLight()) mesh_->DrawMeshDeferredAndExport(false);
            // else mesh_->DrawMeshDeferredAndExport(true);
            mesh_->DrawMeshDeferredAndExport(true);
        });

        ExportScreenPointCloudScreen(deferredExportFBO, screenPoints);

        // camera position
        auto camPos = appNode_->GetCamera()->GetPosition();
        info << camPos.x << ',' << camPos.y << ',' << camPos.z << '\n';
        // camera orientation
        auto camOrient = appNode_->GetCamera()->GetOrientation();
        info << camOrient.x << ',' << camOrient.y << ',' << camOrient.z << ',' << camOrient.w << '\n';
        // environment map
        if (!envMap_) info << '\n';
        else info << envMap_->GetId();
        // light position
        auto lightPos = GetLightPosition();
        info << lightPos.x << ',' << lightPos.y << ',' << lightPos.z << '\n';
        // light color
        auto lightColor = GetLightColor();
        info << lightColor.x << ',' << lightColor.y << ',' << lightColor.z << '\n';
        // light multiplicator
        auto lightMultiplicator = GetLightMultiplicator();
        info << lightMultiplicator;
        // screen size
        info << deferredExportFBO.GetWidth() << ',' << deferredExportFBO.GetHeight() << '\n';

        gl::glBindTexture(gl::GL_TEXTURE_2D, 0);
        gl::glBindBuffer(gl::GL_PIXEL_PACK_BUFFER, 0);

        ExportScreenPointCloudMesh(meshPoints);
    }

    void BaseRenderer::RenderGUI()
    {
        RenderGUIByType();

        if (ImGui::RadioButton("Pass through", GetApp()->GetCompositeType() == 0)) GetApp()->SetCompositeType(0);
        if (ImGui::RadioButton("Direct Only", GetApp()->GetCompositeType() == 1)) GetApp()->SetCompositeType(1);
        if (ImGui::RadioButton("Combine all", GetApp()->GetCompositeType() == 2)) GetApp()->SetCompositeType(2);

        if (mesh_ && pointCloud_) {
            ImGui::Spacing();

            if (ImGui::RadioButton("Export Mesh", !exportPointCloud)) exportPointCloud = false;
            if (ImGui::RadioButton("Export Pointcloud", exportPointCloud)) exportPointCloud = true;
        }

        if (mesh_ && ImGui::Button("Export Screen")) {
            std::string namePrefix;
            if (pointCloud_) namePrefix = pointCloud_->GetPointCloudName();
            else namePrefix = mesh_->GetMeshName();
            std::ofstream infoOut{ namePrefix + "_info.txt" }, screenPoints{ namePrefix + "_screen_export.txt" }, meshPoints{ namePrefix + "_mesh_export.txt" };
        
            ExportScreenPointCloud(appNode_->GetDeferredExportFBO(), infoOut, screenPoints, meshPoints);
        }
    }

    void BaseRenderer::SetEnvironmentMap(std::shared_ptr<Texture> envMap)
    {
        envMap_ = std::move(envMap);
        if (!envMap_) return;

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

    const glm::vec3& BaseRenderer::GetLightPosition() const
    {
        return const_cast<const ApplicationNodeImplementation*>(appNode_)->GetLightPosition();
    }

    const glm::vec3& BaseRenderer::GetLightColor() const
    {
        return const_cast<const ApplicationNodeImplementation*>(appNode_)->GetLightColor();
    }

    float BaseRenderer::GetLightMultiplicator() const
    {
        return const_cast<const ApplicationNodeImplementation*>(appNode_)->GetLightMultiplicator();
    }

    const glm::vec3& BaseRenderer::GetSigmaT() const
    {
        return const_cast<const ApplicationNodeImplementation*>(appNode_)->GetSigmaT();
    }

    float BaseRenderer::GetEta() const
    {
        return const_cast<const ApplicationNodeImplementation*>(appNode_)->GetEta();
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
