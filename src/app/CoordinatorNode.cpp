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
#include "app/PointCloudContainer.h"

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

        constexpr unsigned int headless_width = 512;
        constexpr unsigned int headless_height = 512;

        FrameBufferDescriptor headlessFBODesc{ {
                FrameBufferTextureDescriptor{ static_cast<GLenum>(gl::GL_RGBA8) },
                FrameBufferTextureDescriptor{ static_cast<GLenum>(gl::GL_DEPTH_COMPONENT32F) } },{} };

        headlessFBO_ = std::make_unique<FrameBuffer>(headless_width, headless_height, headlessFBODesc);
        deferredHeadlessFBO_ = std::make_unique<FrameBuffer>(headless_width, headless_height, GetDeferredFBODesc());
        deferredExportHeadlessFBO_ = std::make_unique<FrameBuffer>(headless_width, headless_height, GetDeferredFBODesc());
        // if (!singleFile_.empty()) {
        //     RenderFileHeadless(singleFile_, false);
        //     exit(0);
        // }
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
                ImGui::InputFloat("Point Size", &GetPointSize(), 0.1f);
                ImGui::ColorEdit3("SigmaT", reinterpret_cast<float*>(&GetSigmaT()));
                ImGui::InputFloat("Eta", &GetEta());
                ImGui::InputFloat3("Light Position", reinterpret_cast<float*>(&GetLightPosition()));
                ImGui::ColorEdit3("Light Color", reinterpret_cast<float*>(&GetLightColor()));
                ImGui::InputFloat("Light Multiplicator", &GetLightMultiplicator());
                ImGui::Spacing();
                RendererSelectionGUI();

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
                    auto fileExt = fs::path(selectedFilename).extension().string();
                    if (fileExt == ".obj" || fileExt == ".OBJ") {
                        if (ImGui::Button("Load Mesh AO")) LoadMesh(pcViewer::PCType::AO, selectedFilename);
                        ImGui::SameLine();
                        if (ImGui::Button("Load Mesh GI")) LoadMesh(pcViewer::PCType::MATTE, selectedFilename);
                        ImGui::SameLine();
                        if (ImGui::Button("Load Mesh SSS")) LoadMesh(pcViewer::PCType::SUBSURFACE, selectedFilename);
                    }
                    else {
                        inputFileSelected_ = ImGui::Button("Select File", ImVec2(50, 20));
                        ImGui::SameLine();
                        static bool loadModel;
                        ImGui::Checkbox("Load Model", &loadModel);
                        if (inputFileSelected_) {
                            try {
                                LoadTextFile(selectedFilename, loadModel);
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

                if (ImGui::Button("Select Folder (PC)", ImVec2(50, 20))) RenderFolderHeadless(inputDir_, pcViewer::RenderType::POINTCLOUD);
                ImGui::SameLine();
                if (ImGui::Button("Select Folder (PConMesh)", ImVec2(50, 20))) RenderFolderHeadless(inputDir_, pcViewer::RenderType::PC_ON_MESH);
                ImGui::SameLine();
                if (ImGui::Button("Select Folder (Export)", ImVec2(50, 20))) RenderFolderHeadless(inputDir_, pcViewer::RenderType::MESH); // dont kill me because of this *duckandcover* [9/5/2018 Sebastian Maisch]
                ImGui::SameLine();
                if (ImGui::Button("Select Folder (Screen)", ImVec2(50, 20))) RenderFolderHeadless(inputDir_, pcViewer::RenderType::SCREEN);

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

    void CoordinatorNode::RenderFolderHeadless(const std::string& folder, pcViewer::RenderType renderType)
    {
        auto folderContent = GetDirectoryContent(folder, true);

        for (const auto& file : folderContent) RenderFileHeadless(folder + "/" + file, renderType);

        inputBatchMode_ = false;
    }

    void CoordinatorNode::RenderFileHeadless(const std::string& pointCloud, pcViewer::RenderType renderType)
    {
        namespace fs = std::filesystem;
        auto splitFilename = utils::split(fs::path(pointCloud).stem().string(), '_');
        if (splitFilename[0] == "parameters") return;

        bool render = false, renderScreen = false;
        if (splitFilename[1] == "ao" || splitFilename[1] == "matte" || splitFilename[1] == "subsurface") render = true;
        if (splitFilename.back() == "export" && splitFilename[splitFilename.size() - 2] != "screenfinal") renderScreen = true;

        if (!render || !renderScreen) return;

        try {
            if (renderType == pcViewer::RenderType::POINTCLOUD && render) LoadPointCloud(pointCloud, splitFilename, false);
            else if (renderType == pcViewer::RenderType::PC_ON_MESH && render) LoadPointCloud(pointCloud, splitFilename, true);
            else if (renderType == pcViewer::RenderType::MESH && render) LoadPointCloud(pointCloud, splitFilename, true);
            else if (renderType == pcViewer::RenderType::SCREEN && renderScreen) LoadScreenTexture(pointCloud, splitFilename);
            else return;

            SetMatteRenderType(0);
            SetSubsurfaceRenderType(0);
            if (renderType != pcViewer::RenderType::MESH) SetBaseRenderType(renderType);
        }
        catch (std::invalid_argument e) {
            std::cout << e.what() << std::endl;
        }


        if (renderType == pcViewer::RenderType::MESH) {
            auto namePrefix = splitFilename[1] + '_' + splitFilename.back();
            std::ofstream infoOut{ namePrefix + "_info.txt" }, screenPoints{ namePrefix + "_screen_export.txt" }, meshPoints{ namePrefix + "_mesh_export.txt" };
            GetCurrentRenderer()->ExportScreenPointCloud(GetDeferredExportFBO(), infoOut, screenPoints, meshPoints);
        } else {
            headlessFBO_->DrawToFBO([this]() {
                gl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
                gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);
            });

            if (renderScreen) {
                SetScreenRenderingComposition(splitFilename[1] == "ao" ? 3 : 2);
                DrawLoadedScreen(*headlessFBO_);
            }
            else {

                deferredHeadlessFBO_->DrawToFBO([this]() {
                    gl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
                    gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);
                });

                CurrentRendererDrawPointCloud(*headlessFBO_, *deferredHeadlessFBO_, true);
            }

            auto out_filename = pointCloud.substr(0, pointCloud.size() - 3) + "png";
            enh::TextureDescriptor texDesc(4, gl::GL_RGBA8, gl::GL_RGBA, gl::GL_UNSIGNED_BYTE);
            enh::GLTexture::SaveTextureToFile(headlessFBO_->GetTextures()[0], texDesc, glm::uvec3(headlessFBO_->GetWidth(), headlessFBO_->GetHeight(), 1), out_filename);

            RenderersSetMesh("", nullptr, 0.0f, 0.0f);
            RenderersSetEnvironmentMap(nullptr);
        }
    }

    void CoordinatorNode::LoadTextFile(const std::string& filename, bool loadModel)
    {
        namespace fs = std::filesystem;
        auto splitFilename = utils::split(fs::path(filename).stem().string(), '_');
        if (splitFilename.back() == "export") {
            if (splitFilename[splitFilename.size() - 2] != "screenfinal") return;
            LoadScreenTexture(filename, splitFilename);
        }
        else LoadPointCloud(filename, splitFilename, loadModel);
    }

    void CoordinatorNode::LoadPointCloud(const std::string& pointCloud, const std::vector<std::string>& splitFilename, bool loadModel)
    {
        namespace fs = std::filesystem;
        if (splitFilename[0] == "parameters") return;
        if (splitFilename[1] == "ao") SelectRenderers(pcViewer::PCType::AO);
        else if (splitFilename[1] == "matte") SelectRenderers(pcViewer::PCType::MATTE);
        else if (splitFilename[1] == "subsurface") SelectRenderers(pcViewer::PCType::SUBSURFACE);
        else {
            LOG(WARNING) << "Wrong file format selected.";
            throw std::invalid_argument("Wrong file format selected.");
        }

        RenderersLoadPointCloud(splitFilename[1] + "_" + splitFilename[splitFilename.size() - 1], pointCloud);

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
                if (splitFilename[1] == "subsurface") {
                    glm::vec3 sigmat;
                    sigmat.r = static_cast<float>(std::atof(splitParameters[9].c_str()));
                    sigmat.g = static_cast<float>(std::atof(splitParameters[10].c_str()));
                    sigmat.b = static_cast<float>(std::atof(splitParameters[11].c_str()));
                    float eta;
                    eta = static_cast<float>(std::atof(splitParameters[12].c_str()));
                    GetSigmaT() = sigmat;
                    GetEta() = eta;
                }
                else {
                    GetSigmaT() = glm::vec3(1.0f);
                    GetEta() = 1.0f;
                }
                break;
            }

            auto envMapFilename = inputDir_ + "/" + splitFilename[2];
            for (std::size_t i = 3; i < splitFilename.size() - 3; ++i) envMapFilename += "_" + splitFilename[i];
            envMapFilename += ".hdr";
            std::string shapeNetCorePath = "/ShapeNetCore.v2/";
            while (!fs::exists(inputDir_ + shapeNetCorePath)) shapeNetCorePath = "/.." + shapeNetCorePath;
            auto meshFilename = inputDir_ + shapeNetCorePath + splitFilename[splitFilename.size() - 3] + "/" + splitFilename[splitFilename.size() - 2] + "/models/model_normalized.obj";
            RenderersSetEnvironmentMap(GetTextureManager().GetResource(envMapFilename));
            // SetMesh(GetMeshManager().GetResource(meshFilename, true), meshTheta, meshPhi);
            RenderersSetMesh(splitFilename[splitFilename.size() - 3] + "-" + splitFilename[splitFilename.size() - 2], GetMeshManager().GetResource(meshFilename), meshTheta, meshPhi);
            //SetMesh(GetMeshManager().GetResource("D:/Users/Sebastian Maisch/Documents/dev/deeplearning/ModelNet10/chair/train/chair_0009.off"), 0.0f, 0.0f);

        }
        else {
            RenderersSetMesh("", nullptr, 0.0f, 0.0f);
            RenderersSetEnvironmentMap(nullptr);
        }
        UpdateBaseRendererType();
    }

    void CoordinatorNode::LoadMesh(pcViewer::PCType type, const std::string& meshFilename)
    {
        inputFileSelected_ = true;
        SelectRenderers(type);
        namespace fs = std::filesystem;
        fs::path meshPath{ meshFilename };
        RenderersSetEnvironmentMap(nullptr);
        RenderersSetMesh(meshPath.filename().string(), GetMeshManager().GetResource(meshFilename), 0.f, 0.f);
        UpdateBaseRendererType();
    }

    void CoordinatorNode::LoadScreenTexture(const std::string& filename, const std::vector<std::string>& splitFilename)
    {
        if (splitFilename[0] == "ao") SelectRenderers(pcViewer::PCType::AO);
        else if (splitFilename[0] == "matte") SelectRenderers(pcViewer::PCType::MATTE);
        else if (splitFilename[0] == "subsurface") SelectRenderers(pcViewer::PCType::SUBSURFACE);

        glm::vec3 camPos, lightPos, lightColor;
        glm::quat camOrient;
        float lightMultiplicator = 1.0f;
        std::string envMapFile;
        std::size_t screenWidth = 0, screenHeight = 0;

        {
            std::string infoFile = splitFilename[0];
            for (std::size_t i = 1; i < splitFilename.size() - 2; ++i) infoFile += '_' + splitFilename[i];
            infoFile += "_info.txt";
            std::ifstream info_in{ infoFile };
            std::string line;

            std::getline(info_in, line);
            auto splitParameters = utils::split(line, ',');
            camPos.x = static_cast<float>(std::atof(splitParameters[0].c_str()));
            camPos.y = static_cast<float>(std::atof(splitParameters[1].c_str()));
            camPos.z = static_cast<float>(std::atof(splitParameters[2].c_str()));

            std::getline(info_in, line);
            splitParameters = utils::split(line, ',');
            camOrient.x = static_cast<float>(std::atof(splitParameters[0].c_str()));
            camOrient.y = static_cast<float>(std::atof(splitParameters[1].c_str()));
            camOrient.z = static_cast<float>(std::atof(splitParameters[2].c_str()));
            camOrient.w = static_cast<float>(std::atof(splitParameters[3].c_str()));

            std::getline(info_in, envMapFile);

            std::getline(info_in, line);
            splitParameters = utils::split(line, ',');
            lightPos.x = static_cast<float>(std::atof(splitParameters[0].c_str()));
            lightPos.y = static_cast<float>(std::atof(splitParameters[1].c_str()));
            lightPos.z = static_cast<float>(std::atof(splitParameters[2].c_str()));

            std::getline(info_in, line);
            splitParameters = utils::split(line, ',');
            lightColor.r = static_cast<float>(std::atof(splitParameters[0].c_str()));
            lightColor.g = static_cast<float>(std::atof(splitParameters[1].c_str()));
            lightColor.b = static_cast<float>(std::atof(splitParameters[2].c_str()));

            std::getline(info_in, line);
            lightMultiplicator = static_cast<float>(std::atof(line.c_str()));

            std::getline(info_in, line);
            screenWidth = static_cast<std::size_t>(std::atoi(splitParameters[0].c_str()));
            screenHeight = static_cast<std::size_t>(std::atoi(splitParameters[1].c_str()));
        }


        std::vector<glm::vec4> positionsCPU, normalsCPU, albedoCPU, directIlluminationCPU, textureCPU;
        positionsCPU.resize(screenWidth * screenHeight);
        normalsCPU.resize(screenWidth * screenHeight);
        albedoCPU.resize(screenWidth * screenHeight);
        textureCPU.resize(screenWidth * screenHeight);

        {
            std::ifstream screen_in{ filename };
            std::string line;

            std::size_t pixelIndex = 0;
            while (screen_in.good()) {
                std::getline(screen_in, line);
                auto splitParameters = utils::split(line, ',');
                positionsCPU[pixelIndex].x = static_cast<float>(std::atof(splitParameters[0].c_str()));
                positionsCPU[pixelIndex].y = static_cast<float>(std::atof(splitParameters[1].c_str()));
                positionsCPU[pixelIndex].z = static_cast<float>(std::atof(splitParameters[2].c_str()));
                positionsCPU[pixelIndex].w = 1.0f;

                normalsCPU[pixelIndex].x = static_cast<float>(std::atof(splitParameters[3].c_str()));
                normalsCPU[pixelIndex].y = static_cast<float>(std::atof(splitParameters[4].c_str()));
                normalsCPU[pixelIndex].z = static_cast<float>(std::atof(splitParameters[5].c_str()));
                normalsCPU[pixelIndex].w = 1.0f;

                albedoCPU[pixelIndex].x = static_cast<float>(std::atof(splitParameters[6].c_str()));
                albedoCPU[pixelIndex].y = static_cast<float>(std::atof(splitParameters[7].c_str()));
                albedoCPU[pixelIndex].z = static_cast<float>(std::atof(splitParameters[8].c_str()));
                albedoCPU[pixelIndex].w = 1.0f;

                directIlluminationCPU[pixelIndex].x = static_cast<float>(std::atof(splitParameters[9].c_str()));
                directIlluminationCPU[pixelIndex].y = static_cast<float>(std::atof(splitParameters[10].c_str()));
                directIlluminationCPU[pixelIndex].z = static_cast<float>(std::atof(splitParameters[11].c_str()));
                directIlluminationCPU[pixelIndex].w = 1.0f;

                textureCPU[pixelIndex].x = static_cast<float>(std::atof(splitParameters[12].c_str()));
                textureCPU[pixelIndex].y = static_cast<float>(std::atof(splitParameters[13].c_str()));
                textureCPU[pixelIndex].z = static_cast<float>(std::atof(splitParameters[14].c_str()));
                textureCPU[pixelIndex].w = 1.0f;

                pixelIndex += 1;
            }
        }

        enh::TexuturesRAII<5> screenTextures;
        gl::glBindTexture(gl::GL_TEXTURE_2D, screenTextures[0]);
        gl::glTexImage2D(gl::GL_TEXTURE_2D, 0, gl::GL_RGBA32F, static_cast<gl::GLsizei>(screenWidth), static_cast<gl::GLsizei>(screenHeight), 0, gl::GL_RGBA, gl::GL_FLOAT, positionsCPU.data());
        gl::glBindTexture(gl::GL_TEXTURE_2D, screenTextures[1]);
        gl::glTexImage2D(gl::GL_TEXTURE_2D, 0, gl::GL_RGBA32F, static_cast<gl::GLsizei>(screenWidth), static_cast<gl::GLsizei>(screenHeight), 0, gl::GL_RGBA, gl::GL_FLOAT, normalsCPU.data());
        gl::glBindTexture(gl::GL_TEXTURE_2D, screenTextures[2]);
        gl::glTexImage2D(gl::GL_TEXTURE_2D, 0, gl::GL_RGBA32F, static_cast<gl::GLsizei>(screenWidth), static_cast<gl::GLsizei>(screenHeight), 0, gl::GL_RGBA, gl::GL_FLOAT, albedoCPU.data());
        gl::glBindTexture(gl::GL_TEXTURE_2D, screenTextures[3]);
        gl::glTexImage2D(gl::GL_TEXTURE_2D, 0, gl::GL_RGBA32F, static_cast<gl::GLsizei>(screenWidth), static_cast<gl::GLsizei>(screenHeight), 0, gl::GL_RGBA, gl::GL_FLOAT, directIlluminationCPU.data());
        gl::glBindTexture(gl::GL_TEXTURE_2D, screenTextures[4]);
        gl::glTexImage2D(gl::GL_TEXTURE_2D, 0, gl::GL_RGBA32F, static_cast<gl::GLsizei>(screenWidth), static_cast<gl::GLsizei>(screenHeight), 0, gl::GL_RGBA, gl::GL_FLOAT, textureCPU.data());
        gl::glBindTexture(gl::GL_TEXTURE_2D, 0);

        StartRenderScreen(std::move(screenTextures));
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
