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
#include <regex>
#include <filesystem>

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

    void BaseRenderer::ExportPBRT(const std::string& pbrtOutName, const glm::uvec2& imgSize, std::ostream& pbrt)
    {
        // auto camPos = appNode_->GetCamera()->GetPosition();
        auto camPos = appNode_->GetCameraEnh().GetPosition();

        std::stringstream cam_pos_str, cam_fov_str, model_theta_str, model_phi_str, light_pos_str, light_strength_str, img_size_x_str, img_size_y_str;
        cam_pos_str << camPos.x << " " << camPos.y << " " << camPos.z;
        model_theta_str << mesh_->GetTheta() * 180.f / glm::pi<float>();
        model_phi_str << mesh_->GetPhi() * 180.f / glm::pi<float>();
        light_pos_str << GetLightPosition().x << " " << GetLightPosition().y << " " << GetLightPosition().z;
        auto lightStrength = GetLightColor() * GetLightMultiplicator();
        light_strength_str << lightStrength.x << " " << lightStrength.y << " " << lightStrength.z;
        img_size_x_str << imgSize.x;
        img_size_y_str << imgSize.y;
        cam_fov_str << 2.0f * glm::atan(1.0f / appNode_->GetCamera()->GetCentralPerspectiveMatrix()[1][1]) * 180.f / glm::pi<float>();


        auto sigmasp = GetAlpha() * GetSigmaT();
        auto sigmaa = GetSigmaT() * sigmasp;
        std::stringstream matte_Kd_str, subsurface_sigmaa_str, subsurface_sigmasp_str, subsurface_eta_str;
        matte_Kd_str << GetAlpha().r << " " << GetAlpha().g << " " << GetAlpha().b;
        subsurface_sigmaa_str << sigmaa.r << " " << sigmaa.g << " " << sigmaa.b;
        subsurface_sigmasp_str << sigmasp.r << " " << sigmasp.g << " " << sigmasp.b;
        subsurface_eta_str << GetEta();

        std::string matte_comment = "";
        std::string subsurface_comment = "";
        if (pcType_ == PCType::AO || pcType_ == PCType::MATTE) {
            matte_comment = "";
            subsurface_comment = "# ";
        }
        else if (pcType_ == PCType::SUBSURFACE) {
            matte_comment = "# ";
            subsurface_comment = "";
        }

        std::stringstream model_filename;
        model_filename << std::quoted(std::filesystem::path(mesh_->GetMeshFilename()).lexically_normal().string());

        std::string integrator_name = "path";
        if (pcType_ == PCType::AO) integrator_name = "ambientocclusion";

        std::ifstream pbrt_in(Resource::FindResourceLocation("basic.pbrt.in", appNode_->GetConfig()));

        std::string line;
        while (pbrt_in.good()) {
            std::getline(pbrt_in, line);

            line = std::regex_replace(line, std::regex(R"(\$\{INTEGRATOR_NAME\})"), integrator_name);
            line = std::regex_replace(line, std::regex(R"(\$\{PBRT_FN\})"), pbrtOutName);
            line = std::regex_replace(line, std::regex(R"(\$\{XRES\})"), img_size_x_str.str());
            line = std::regex_replace(line, std::regex(R"(\$\{YRES\})"), img_size_y_str.str());
            line = std::regex_replace(line, std::regex(R"(\$\{CAM_POS\})"), cam_pos_str.str());
            line = std::regex_replace(line, std::regex(R"(\$\{CAM_FOV\})"), cam_fov_str.str());
            line = std::regex_replace(line, std::regex(R"(\$\{MODEL_FN\})"), model_filename.str());
            line = std::regex_replace(line, std::regex(R"(\$\{LIGHT_POS\})"), light_pos_str.str());
            line = std::regex_replace(line, std::regex(R"(\$\{LIGHT_STRENGTH\})"), light_strength_str.str());
            line = std::regex_replace(line, std::regex(R"(\$\{MODEL_THETA\})"), model_theta_str.str());
            line = std::regex_replace(line, std::regex(R"(\$\{MODEL_PHI\})"), model_phi_str.str());
            line = std::regex_replace(line, std::regex(R"(\$\{MATTE_COMMENT\})"), matte_comment);
            line = std::regex_replace(line, std::regex(R"(\$\{MATTE_KD\})"), matte_Kd_str.str());
            line = std::regex_replace(line, std::regex(R"(\$\{SUBSURFACE_COMMENT\})"), subsurface_comment);
            line = std::regex_replace(line, std::regex(R"(\$\{SUBSURFACE_SIGMAA\})"), subsurface_sigmaa_str.str());
            line = std::regex_replace(line, std::regex(R"(\$\{SUBSURFACE_SIGMAPS\})"), subsurface_sigmasp_str.str());
            line = std::regex_replace(line, std::regex(R"(\$\{SUBSURFACE_ETA\})"), subsurface_eta_str.str());
            pbrt << line << std::endl;
        }
    }

    void BaseRenderer::ExportScreenPointCloud(const FrameBuffer& deferredExportFBO, const std::string& namePrefix,
        std::ostream& info, std::ostream& screenPoints, std::ostream& meshPoints)
    {
        GetApp()->ClearExportScreenPointCloud(deferredExportFBO);

        deferredExportFBO.DrawToFBO(GetApp()->GetDeferredDrawIndices(), [this]() {
            // if (pointCloud_ && pointCloud_->HasDirectLight()) mesh_->DrawMeshDeferredAndExport(false);
            // else mesh_->DrawMeshDeferredAndExport(true);
            mesh_->DrawMeshDeferredAndExport(true);
        });

        ExportScreenPointCloudScreen(deferredExportFBO, namePrefix, screenPoints);

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

        if (*mesh_ && pointCloud_) {
            ImGui::Spacing();

            if (ImGui::RadioButton("Export Mesh", !exportPointCloud)) exportPointCloud = false;
            if (ImGui::RadioButton("Export Pointcloud", exportPointCloud)) exportPointCloud = true;
        }

        if (*mesh_ && ImGui::Button("Export Screen")) {
            std::string namePrefix;
            if (pointCloud_) namePrefix = pointCloud_->GetPointCloudName();
            else namePrefix = mesh_->GetMeshName();
            std::ofstream infoOut{ namePrefix + "_info.txt" }, screenPoints{ namePrefix + "_screen_export.txt" }, meshPoints{ namePrefix + "_mesh_export.txt" };
            std::ofstream pbrtOut{ namePrefix + ".pbrt" };
        
            ExportScreenPointCloud(appNode_->GetDeferredExportFBO(), namePrefix, infoOut, screenPoints, meshPoints);
            ExportPBRT(namePrefix, glm::uvec2(appNode_->GetDeferredExportFBO().GetWidth(), appNode_->GetDeferredExportFBO().GetHeight()), pbrtOut);
        }
        // TODO add export version for images and pbrt... [9/28/2018 Sebastian Maisch]
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

    const glm::vec3& BaseRenderer::GetAlpha() const
    {
        return const_cast<const ApplicationNodeImplementation*>(appNode_)->GetAlpha();
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
