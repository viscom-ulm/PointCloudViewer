/**
 * @file   MasterNode.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.25
 *
 * @brief  Implementation of the master application node.
 */

#include "MasterNode.h"
#include <imgui.h>
#include "enh/gfx/postprocessing/DepthOfField.h"
#include "enh/gfx/postprocessing/BloomEffect.h"
#include "enh/gfx/postprocessing/FilmicTMOperator.h"
#include "enh/gfx/gl/GLTexture.h"

#include "core/glfw.h"
#include <glbinding/gl/gl.h>
#include <filesystem>
#include <fstream>
#include <glm/gtc/type_ptr.hpp>

namespace viscom {

    std::string MasterNode::singleFile_ = "";

    MasterNode::MasterNode(ApplicationNodeInternal* appNode) :
        ApplicationNodeImplementation{ appNode }
    {
    }

    MasterNode::~MasterNode() = default;

    void MasterNode::InitOpenGL()
    {
        ApplicationNodeImplementation::InitOpenGL();

        if (!singleFile_.empty()) {
            RenderFileHeadless(singleFile_, false);
            exit(0);
        }
    }

    void MasterNode::Draw2D(FrameBuffer& fbo)
    {
        std::vector<std::string> supportedDriveLetters = { "A:/", "B:/", "C:/", "D:/", "E:/", "F:/", "G:/", "H:/" };
        namespace fs = std::filesystem;

        fbo.DrawToFBO([&]() {
            ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);
            ImGui::StyleColorsClassic();
            if (inputFileSelected_ && !inputBatchMode_ && ImGui::Begin("", nullptr)) {
                // GetDOF()->RenderParameterSliders();
                // GetToneMapping()->RenderParameterSliders();
                // GetBloom()->RenderParameterSliders();
                if (GetPointCloudType() == PCType::MATTE) {
                    if (ImGui::RadioButton("Global Illumination", GetMatteRenderType() == 0)) SetMatteRenderType(0);
                    if (ImGui::RadioButton("Matte Albedo", GetMatteRenderType() == 1)) SetMatteRenderType(1);
                    if (ImGui::RadioButton("Direct Illumination", GetMatteRenderType() == 2)) SetMatteRenderType(2);
                }
                else if (GetPointCloudType() == PCType::SUBSURFACE) {
                    if (ImGui::RadioButton("Global Illumination", GetMatteRenderType() == 0)) SetMatteRenderType(0);
                    if (ImGui::RadioButton("Subsurface Albedo", GetMatteRenderType() == 1)) SetMatteRenderType(1);
                    if (ImGui::RadioButton("Sigma_t'", GetMatteRenderType() == 2)) SetMatteRenderType(2);
                    if (ImGui::RadioButton("Eta", GetMatteRenderType() == 3)) SetMatteRenderType(3);
                    if (ImGui::RadioButton("Direct Illumination", GetMatteRenderType() == 4)) SetMatteRenderType(4);
                }
                
            }
            ImGui::End();

            std::string selectedFile;
            ImGui::SetNextWindowPos(ImVec2(60, 60), ImGuiSetCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);
            ImGui::StyleColorsClassic();
            if (!inputFileSelected_ && ImGui::Begin("Select point cloud file", nullptr, ImGuiWindowFlags_MenuBar)) {
                ImGui::Text(inputDir_.data());
                for (const auto& dl : supportedDriveLetters) {
                    if (fs::is_directory(dl)) {
                        bool selected = false;
                        ImGui::Selectable(dl.c_str(), &selected);
                        if (selected) inputDir_ = dl;
                    }
                }

                std::string selectedFilename = "";
                for (auto &path : GetDirectoryContent(inputDir_, false)) {
                    bool selected = false;
                    ImGui::Selectable(path.data(), &selected);
                    if (selected) {
                        auto selection = fs::canonical(fs::path(inputDir_) / path).string();
                        if (fs::is_regular_file(selection)) selectedFilename = selection;
                        else inputDir_ = selection;
                    }
                }

                if (!selectedFilename.empty()) inputFileSelected_ = ImGui::Button("Select File", ImVec2(50, 20));
                ImGui::SameLine();
                static bool loadModel;
                ImGui::Checkbox("Load Model", &loadModel);
                if (inputFileSelected_) LoadPointCloud(selectedFilename, loadModel);

                ImGui::End();
            }

            if (inputBatchMode_ && ImGui::Begin("Select point cloud file", nullptr, ImGuiWindowFlags_MenuBar)) {
                ImGui::Text(inputDir_.data());
                for (const auto& dl : supportedDriveLetters) {
                    if (fs::is_directory(dl)) {
                        bool selected = false;
                        ImGui::Selectable(dl.c_str(), &selected);
                        if (selected) inputDir_ = dl;
                    }
                }

                for (auto &path : GetDirectoryContent(inputDir_, false)) {
                    bool selected = false;
                    ImGui::Selectable(path.data(), &selected);
                    if (selected) {
                        auto selection = fs::canonical(fs::path(inputDir_) / path).string();
                        if (fs::is_directory(selection)) inputDir_ = selection;
                    }
                }

                bool batchFolderSelect = ImGui::Button("Select Batch Folder", ImVec2(50, 20));
                ImGui::SameLine();
                static bool loadModel;
                ImGui::Checkbox("Load Model", &loadModel);
                if (batchFolderSelect) RenderFolderHeadless(inputDir_, loadModel);

                ImGui::End();
            }
        });

        ApplicationNodeImplementation::Draw2D(fbo);
    }


    std::vector<std::string> MasterNode::GetDirectoryContent(const std::string& dir, bool pcFilesOnly) const
    {
        namespace fs = std::filesystem;

        std::vector<std::string> content;
        content.emplace_back("..");

        auto checkPCFile = [](const fs::path& p) { 
            if (!(p.extension().string() == ".txt" || p.extension().string() == ".TXT")) return false;
        };

        for (auto& p : fs::directory_iterator(dir)) {
            try {
                if (fs::is_regular_file(p) && checkPCFile(p)) content.push_back(p.path().filename().string());
                else if (!pcFilesOnly && fs::is_directory(p)) content.push_back(p.path().filename().string());
            } catch (fs::filesystem_error e) {}
        }
        return content;
    }

    void MasterNode::RenderFolderHeadless(const std::string& folder, bool loadModel)
    {
        auto folderContent = GetDirectoryContent(folder, true);

        for (const auto& file : folderContent) RenderFileHeadless(folder + "/" + file, loadModel);

        inputBatchMode_ = false;
    }

    void MasterNode::RenderFileHeadless(const std::string& pointCloud, bool loadModel)
    {
        FrameBufferDescriptor headlessFBODesc{ {
                FrameBufferTextureDescriptor{ static_cast<GLenum>(gl::GL_RGBA8) },
                FrameBufferTextureDescriptor{ static_cast<GLenum>(gl::GL_DEPTH_COMPONENT32F) } },{} };

        FrameBuffer fbo(800, 600, headlessFBODesc);

        LoadPointCloud(singleFile_, loadModel);

        fbo.DrawToFBO([this]() {
            gl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);

            DrawPointCloud();
        });

        auto out_filename = pointCloud.substr(0, pointCloud.size() - 3) + "png";
        enh::TextureDescriptor texDesc(4, gl::GL_RGBA8, gl::GL_RGBA, gl::GL_UNSIGNED_BYTE);
        enh::GLTexture::SaveTextureToFile(fbo.GetTextures()[0], texDesc, glm::uvec3(800, 600, 1), out_filename);
    }

    void MasterNode::LoadPointCloud(const std::string& pointCloud, bool loadModel)
    {
        namespace fs = std::filesystem;

        auto splitFilename = utils::split(fs::path(pointCloud).stem().string(), '_');
        if (splitFilename[1] == "ao") LoadPointCloudAO(pointCloud);
        else if (splitFilename[1] == "matte") LoadPointCloudMatte(pointCloud);
        else if (splitFilename[1] == "subsurface") LoadPointCloudSubsurface(pointCloud);
        else {
            LOG(WARNING) << "Wrong file format selected.";
            inputFileSelected_ = false;
        }

        // TODO: handle model loading. [5/20/2018 Sebastian Maisch]
    }

    void MasterNode::LoadPointCloudAO(const std::string& pointCloud)
    {
        std::ifstream pc_in(pointCloud);
        std::vector<PointCloudPointAO> pointCloudData;

        std::string line;
        while (pc_in.good()) {
            std::getline(pc_in, line);

            auto splitPointData = utils::split(line, ',');
            if (splitPointData.size() < 7) continue;

            pointCloudData.emplace_back();
            pointCloudData.back().position_.x = static_cast<float>(std::atof(splitPointData[0].c_str()));
            pointCloudData.back().position_.y = static_cast<float>(std::atof(splitPointData[1].c_str()));
            pointCloudData.back().position_.z = static_cast<float>(std::atof(splitPointData[2].c_str()));

            pointCloudData.back().normal_.x = static_cast<float>(std::atof(splitPointData[3].c_str()));
            pointCloudData.back().normal_.y = static_cast<float>(std::atof(splitPointData[4].c_str()));
            pointCloudData.back().normal_.z = static_cast<float>(std::atof(splitPointData[5].c_str()));

            pointCloudData.back().ao_ = static_cast<float>(std::atof(splitPointData[6].c_str()));
        }

        LoadPointCloudGPUAO(pointCloudData);
    }

    void MasterNode::LoadPointCloudMatte(const std::string& pointCloud)
    {
        std::ifstream pc_in(pointCloud);
        std::vector<PointCloudPointMatte> pointCloudData;

        std::string line;
        while (pc_in.good()) {
            std::getline(pc_in, line);

            auto splitPointData = utils::split(line, ',');
            if (splitPointData.size() < 7) continue;

            pointCloudData.emplace_back();
            pointCloudData.back().position_.x = static_cast<float>(std::atof(splitPointData[0].c_str()));
            pointCloudData.back().position_.y = static_cast<float>(std::atof(splitPointData[1].c_str()));
            pointCloudData.back().position_.z = static_cast<float>(std::atof(splitPointData[2].c_str()));

            pointCloudData.back().normal_.x = static_cast<float>(std::atof(splitPointData[3].c_str()));
            pointCloudData.back().normal_.y = static_cast<float>(std::atof(splitPointData[4].c_str()));
            pointCloudData.back().normal_.z = static_cast<float>(std::atof(splitPointData[5].c_str()));

            pointCloudData.back().directIllumination_.x = static_cast<float>(std::atof(splitPointData[6].c_str()));
            pointCloudData.back().directIllumination_.y = static_cast<float>(std::atof(splitPointData[7].c_str()));
            pointCloudData.back().directIllumination_.z = static_cast<float>(std::atof(splitPointData[8].c_str()));

            pointCloudData.back().albedo_.x = static_cast<float>(std::atof(splitPointData[9].c_str()));
            pointCloudData.back().albedo_.y = static_cast<float>(std::atof(splitPointData[10].c_str()));
            pointCloudData.back().albedo_.z = static_cast<float>(std::atof(splitPointData[11].c_str()));

            pointCloudData.back().globalIllumination_.x = static_cast<float>(std::atof(splitPointData[12].c_str()));
            pointCloudData.back().globalIllumination_.y = static_cast<float>(std::atof(splitPointData[13].c_str()));
            pointCloudData.back().globalIllumination_.z = static_cast<float>(std::atof(splitPointData[14].c_str()));
        }

        LoadPointCloudGPUMatte(pointCloudData);
    }

    void MasterNode::LoadPointCloudSubsurface(const std::string& pointCloud)
    {
        std::ifstream pc_in(pointCloud);
        std::vector<PointCloudPointSubsurface> pointCloudData;

        std::string line;
        while (pc_in.good()) {
            std::getline(pc_in, line);

            auto splitPointData = utils::split(line, ',');
            if (splitPointData.size() < 7) continue;

            pointCloudData.emplace_back();
            pointCloudData.back().position_.x = static_cast<float>(std::atof(splitPointData[0].c_str()));
            pointCloudData.back().position_.y = static_cast<float>(std::atof(splitPointData[1].c_str()));
            pointCloudData.back().position_.z = static_cast<float>(std::atof(splitPointData[2].c_str()));

            pointCloudData.back().normal_.x = static_cast<float>(std::atof(splitPointData[3].c_str()));
            pointCloudData.back().normal_.y = static_cast<float>(std::atof(splitPointData[4].c_str()));
            pointCloudData.back().normal_.z = static_cast<float>(std::atof(splitPointData[5].c_str()));

            pointCloudData.back().directIllumination_.x = static_cast<float>(std::atof(splitPointData[6].c_str()));
            pointCloudData.back().directIllumination_.y = static_cast<float>(std::atof(splitPointData[7].c_str()));
            pointCloudData.back().directIllumination_.z = static_cast<float>(std::atof(splitPointData[8].c_str()));

            pointCloudData.back().albedo_.x = static_cast<float>(std::atof(splitPointData[9].c_str()));
            pointCloudData.back().albedo_.y = static_cast<float>(std::atof(splitPointData[10].c_str()));
            pointCloudData.back().albedo_.z = static_cast<float>(std::atof(splitPointData[11].c_str()));

            pointCloudData.back().sigma_tp_.x = static_cast<float>(std::atof(splitPointData[12].c_str()));
            pointCloudData.back().sigma_tp_.y = static_cast<float>(std::atof(splitPointData[13].c_str()));
            pointCloudData.back().sigma_tp_.z = static_cast<float>(std::atof(splitPointData[14].c_str()));

            pointCloudData.back().eta_ = static_cast<float>(std::atof(splitPointData[15].c_str()));

            pointCloudData.back().globalIllumination_.x = static_cast<float>(std::atof(splitPointData[16].c_str()));
            pointCloudData.back().globalIllumination_.y = static_cast<float>(std::atof(splitPointData[17].c_str()));
            pointCloudData.back().globalIllumination_.z = static_cast<float>(std::atof(splitPointData[18].c_str()));
        }

        LoadPointCloudGPUSubsurface(pointCloudData);
    }

    bool MasterNode::KeyboardCallback(int key, int scancode, int action, int mods)
    {
        if (ApplicationNodeBase::KeyboardCallback(key, scancode, action, mods)) return true;

        switch (key)
        {
        case GLFW_KEY_L:
            if (action == GLFW_PRESS && mods == GLFW_MOD_CONTROL) {
                inputFileSelected_ = false;
                inputBatchMode_ = false;
                return true;
            }
            break;
        case GLFW_KEY_B:
            if (action == GLFW_PRESS && mods == GLFW_MOD_CONTROL) {
                inputBatchMode_ = true;
                inputFileSelected_ = true;
                return true;
            }
            break;
        default: return false;
        }

        return false;
    }

}
