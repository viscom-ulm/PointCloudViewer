/**
 * @file   CoordinatorNode.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.25
 *
 * @brief  Implementation of the coordinator application node.
 */

#include "CoordinatorNode.h"
#include <imgui.h>
#include "enh/gfx/postprocessing/DepthOfField.h"
#include "enh/gfx/postprocessing/BloomEffect.h"
#include "enh/gfx/postprocessing/FilmicTMOperator.h"
#include "enh/gfx/gl/GLTexture.h"
#include "app/Renderer.h"

#include "core/glfw.h"
#include <glbinding/gl/gl.h>
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <glm/gtc/type_ptr.hpp>

namespace viscom {

    std::string CoordinatorNode::singleFile_ = "";

    CoordinatorNode::CoordinatorNode(ApplicationNodeInternal* appNode) :
        ApplicationNodeImplementation{ appNode },
        inputDir_{ std::filesystem::canonical(std::filesystem::path(".")).string() }
    {
        inputDirEntries_ = GetDirectoryContent(inputDir_, false);
    }

    CoordinatorNode::~CoordinatorNode() = default;

    void CoordinatorNode::InitOpenGL()
    {
        ApplicationNodeImplementation::InitOpenGL();

        if (!singleFile_.empty()) {
            RenderFileHeadless(singleFile_, false);
            exit(0);
        }
    }

    void CoordinatorNode::Draw2D(FrameBuffer& fbo)
    {
        std::vector<std::string> supportedDriveLetters = { "A:/", "B:/", "C:/", "D:/", "E:/", "F:/", "G:/", "H:/" };
        namespace fs = std::filesystem;

        fbo.DrawToFBO([&]() {
            ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);
            ImGui::StyleColorsClassic();
            if (inputFileSelected_ && !inputBatchMode_ && ImGui::Begin("", nullptr)) {
                ImGui::InputFloat("Distance Power", &GetDistancePower(), 0.1f);
                GetRenderer()->RenderGUI();

                ImGui::End();
            }

            std::string selectedFile;
            ImGui::SetNextWindowPos(ImVec2(60, 60), ImGuiSetCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);
            ImGui::StyleColorsClassic();
            if (!inputFileSelected_ && ImGui::Begin("Select point cloud file", nullptr, ImGuiWindowFlags_MenuBar)) {
                ImGui::Text(inputDir_.data());
                ImGui::BeginChild("Scrolling", ImVec2(0.0f, -40.0f));
                bool skipFiles = false;
                for (const auto& dl : supportedDriveLetters) {
                    std::error_code ec;
                    bool isDir = fs::is_directory(dl, ec);
                    if (!ec && isDir) {
                        bool selected = false;
                        ImGui::Selectable(dl.c_str(), &selected);
                        if (selected) {
                            inputDir_ = dl;
                            inputDirEntries_ = GetDirectoryContent(inputDir_, false);
                            skipFiles = true;
                        }
                    }
                }

                static std::string selectedFilename = "";
                if (!skipFiles) for (const auto& path : inputDirEntries_) {
                    bool selected = false;
                    ImGui::Selectable(path.data(), &selected);
                    if (selected) {
                        auto selection = fs::canonical(fs::path(inputDir_) / path).string();
                        if (fs::is_regular_file(selection)) selectedFilename = selection;
                        else {
                            inputDir_ = selection;
                            inputDirEntries_ = GetDirectoryContent(inputDir_, false);
                            break;
                        }
                    }
                }
                ImGui::EndChild();

                if (!selectedFilename.empty()) {
                    inputFileSelected_ = ImGui::Button("Select File", ImVec2(50, 20));
                    auto fileExt = fs::path(selectedFilename).extension().string();
                    if (fileExt == ".obj" || fileExt == ".OBJ") {
                        // TODO: Load model. [8/17/2018 Sebastian Maisch]
                    }
                    else {
                        ImGui::SameLine();
                        static bool loadModel;
                        ImGui::Checkbox("Load Model", &loadModel);
                        if (inputFileSelected_) {
                            try {
                                LoadPointCloud(selectedFilename, loadModel);
                            }
                            catch (std::invalid_argument e) {
                                inputFileSelected_ = false;
                            }
                        }
                    }
                }

                ImGui::End();
            }

            ImGui::SetNextWindowPos(ImVec2(60, 60), ImGuiSetCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);
            ImGui::StyleColorsClassic();
            if (inputBatchMode_ && ImGui::Begin("Select batch folder", nullptr, ImGuiWindowFlags_MenuBar)) {
                ImGui::Text(inputDir_.data());
                bool skipFiles = false;
                for (const auto& dl : supportedDriveLetters) {
                    std::error_code ec;
                    bool isDir = fs::is_directory(dl, ec);
                    if (!ec && isDir) {
                        bool selected = false;
                        ImGui::Selectable(dl.c_str(), &selected);
                        if (selected) {
                            inputDir_ = dl;
                            inputDirEntries_ = GetDirectoryContent(inputDir_, false);
                            skipFiles = true;
                        }
                    }
                }

                if (!skipFiles) for (auto &path : inputDirEntries_) {
                    bool selected = false;
                    ImGui::Selectable(path.data(), &selected);
                    if (selected) {
                        auto selection = fs::canonical(fs::path(inputDir_) / path).string();
                        if (fs::is_directory(selection)) {
                            inputDir_ = selection;
                            inputDirEntries_ = GetDirectoryContent(inputDir_, false);
                            break;
                        }
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


    std::vector<std::string> CoordinatorNode::GetDirectoryContent(const std::string& dir, bool pcFilesOnly) const
    {
        namespace fs = std::filesystem;

        std::vector<std::string> content;
        if (!pcFilesOnly) content.emplace_back("..");

        auto checkPCFile = [](const fs::path& p) { 
            if (!(p.extension().string() == ".txt" || p.extension().string() == ".TXT" || p.extension().string() == ".obj" || p.extension().string() == ".OBJ")) return false;
            return true;
        };

        for (auto& p : fs::directory_iterator(dir)) {
            try {
                if (fs::is_regular_file(p) && checkPCFile(p)) content.push_back(p.path().filename().string());
                else if (!pcFilesOnly && fs::is_directory(p)) content.push_back(p.path().filename().string());
            } catch (fs::filesystem_error e) {}
        }
        return content;
    }

    void CoordinatorNode::RenderFolderHeadless(const std::string& folder, bool loadModel)
    {
        auto folderContent = GetDirectoryContent(folder, true);

        for (const auto& file : folderContent) RenderFileHeadless(folder + "/" + file, loadModel);

        inputBatchMode_ = false;
    }

    void CoordinatorNode::RenderFileHeadless(const std::string& pointCloud, bool loadModel)
    {
        FrameBufferDescriptor headlessFBODesc{ {
                FrameBufferTextureDescriptor{ static_cast<GLenum>(gl::GL_RGBA8) },
                FrameBufferTextureDescriptor{ static_cast<GLenum>(gl::GL_DEPTH_COMPONENT32F) } },{} };

        FrameBuffer fbo(512, 512, headlessFBODesc);

        FrameBufferDescriptor headlessDefferedFBODesc{ {
                FrameBufferTextureDescriptor{ static_cast<GLenum>(gl::GL_RGBA8) },
                FrameBufferTextureDescriptor{ static_cast<GLenum>(gl::GL_DEPTH_COMPONENT32F) } },{} };

        FrameBuffer deferredFBO(fbo.GetWidth(), fbo.GetHeight(), headlessDefferedFBODesc);

        try {
            LoadPointCloud(pointCloud, loadModel);
        }
        catch (std::invalid_argument e) {
            std::cout << e.what() << std::endl;
        }

        fbo.DrawToFBO([this]() {
            gl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);
        });

        deferredFBO.DrawToFBO([this]() {
            gl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);
        });

        GetRenderer()->DrawPointCloud(fbo, deferredFBO, true);

        auto out_filename = pointCloud.substr(0, pointCloud.size() - 3) + "png";
        enh::TextureDescriptor texDesc(4, gl::GL_RGBA8, gl::GL_RGBA, gl::GL_UNSIGNED_BYTE);
        enh::GLTexture::SaveTextureToFile(fbo.GetTextures()[0], texDesc, glm::uvec3(fbo.GetWidth(), fbo.GetHeight(), 1), out_filename);

        GetRenderer()->SetMesh(nullptr, 0.0f, 0.0f);
        GetRenderer()->SetEnvironmentMap(nullptr);
    }

    void CoordinatorNode::LoadPointCloud(const std::string& pointCloud, bool loadModel)
    {
        namespace fs = std::filesystem;

        auto splitFilename = utils::split(fs::path(pointCloud).stem().string(), '_');
        if (splitFilename[0] == "parameters") return;
        if (splitFilename[1] == "ao") SelectPointCloudRenderer(pcViewer::PCType::AO);
        else if (splitFilename[1] == "matte") SelectPointCloudRenderer(pcViewer::PCType::MATTE);
        else if (splitFilename[1] == "subsurface") SelectPointCloudRenderer(pcViewer::PCType::SUBSURFACE);
        else {
            LOG(WARNING) << "Wrong file format selected.";
            throw std::invalid_argument("Wrong file format selected.");
        }

        GetRenderer()->LoadPointCloud(pointCloud);

        if (loadModel) {
            std::ifstream params_in(inputDir_ + "/parameters_" + splitFilename[1] + ".txt");
            std::string line;
            std::getline(params_in, line); // description line...

            std::size_t lineCounter = static_cast<std::size_t>(std::atoi(splitFilename[splitFilename.size() - 1].c_str())) + 2;
            float meshTheta = 0.0f, meshPhi = 0.0f;

            if (splitFilename[1] == "matte") lineCounter -= 20000;
            else if (splitFilename[1] == "subsurface") lineCounter -= 40000;

            while (params_in.good()) {
                std::getline(params_in, line);
                auto splitParameters = utils::split(line, ',');
                if (splitParameters[0] != splitFilename[splitFilename.size() - 1]) continue;
                // if (--lineCounter != 0) continue;

                // assert(splitParameters[0] == splitFilename[splitFilename.size() - 1]);
                meshTheta = static_cast<float>(std::atof(splitParameters[4].c_str()));
                meshPhi = static_cast<float>(std::atof(splitParameters[5].c_str()));
                break;
            }

            auto envMapFilename = inputDir_ + "/" + splitFilename[2];
            for (std::size_t i = 3; i < splitFilename.size() - 3; ++i) envMapFilename += "_" + splitFilename[i];
            envMapFilename += ".hdr";
            auto meshFilename = inputDir_ + "/../../ShapeNetCore.v2/" + splitFilename[splitFilename.size() - 3] + "/" + splitFilename[splitFilename.size() - 2] + "/models/model_normalized.obj";
            GetRenderer()->SetEnvironmentMap(GetTextureManager().GetResource(envMapFilename));
            // SetMesh(GetMeshManager().GetResource(meshFilename, true), meshTheta, meshPhi);
            GetRenderer()->SetMesh(GetMeshManager().GetResource(meshFilename), meshTheta, meshPhi);
            //SetMesh(GetMeshManager().GetResource("D:/Users/Sebastian Maisch/Documents/dev/deeplearning/ModelNet10/chair/train/chair_0009.off"), 0.0f, 0.0f);

        }
        else {
            GetRenderer()->SetMesh(nullptr, 0.0f, 0.0f);
            GetRenderer()->SetEnvironmentMap(nullptr);
        }
    }

    bool CoordinatorNode::KeyboardCallback(int key, int scancode, int action, int mods)
    {
        if (ApplicationNodeImplementation::KeyboardCallback(key, scancode, action, mods)) return true;

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
