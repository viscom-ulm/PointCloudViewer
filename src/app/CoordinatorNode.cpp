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
#include "app/MeshContainer.h"

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
        InitOpenGLInternal();
    }

    CoordinatorNode::~CoordinatorNode() = default;

    void CoordinatorNode::InitOpenGLInternal()
    {
        constexpr unsigned int headless_width = 1024;
        constexpr unsigned int headless_height = 1024;

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

        if (hideGUI_) return;

        fbo.DrawToFBO([&]() {
            ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);
            ImGui::StyleColorsClassic();
            if (inputFileSelected_ && !inputBatchMode_ && ImGui::Begin("", nullptr)) {
                ImGui::InputFloat("Distance Power", &GetDistancePower(), 0.1f);
                ImGui::InputFloat("Point Size", &GetPointSize(), 0.1f);
                if (mesh_ && *mesh_) {
                    ImGui::InputFloat3("Albedo", reinterpret_cast<float*>(&mesh_->GetMesh(0).info_.albedo_), "%.6f");
                    ImGui::InputFloat3("SigmaT", reinterpret_cast<float*>(&mesh_->GetMesh(0).info_.sigmat_), "%.6f");
                    ImGui::InputFloat("Eta", &mesh_->GetMesh(0).info_.eta_);
                }
                ImGui::InputFloat3("Light Position", reinterpret_cast<float*>(&GetLightPosition()));
                ImGui::ColorEdit3("Light Color", reinterpret_cast<float*>(&GetLightColor()));
                ImGui::InputFloat("Light Multiplicator", &GetLightMultiplicator());
                ImGui::Spacing();
                if (ImGui::Button("Release Camera")) GetCameraEnh().ReleaseView();
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
                    else if (fileExt == ".params") {
                        if (ImGui::Button("Import DNN Results")) LoadParamsFile(selectedFilename);
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
                ImGui::EndChild();

                if (ImGui::Button("Select (PC)", ImVec2(100, 20))) RenderFolderHeadless(inputDir_, pcViewer::RenderType::POINTCLOUD);
                ImGui::SameLine();
                if (ImGui::Button("Select (PConMesh)", ImVec2(100, 20))) RenderFolderHeadless(inputDir_, pcViewer::RenderType::PC_ON_MESH);
                ImGui::SameLine();
                if (ImGui::Button("Select (Export)", ImVec2(100, 20))) RenderFolderHeadless(inputDir_, pcViewer::RenderType::MESH); // dont kill me because of this *duckandcover* [9/5/2018 Sebastian Maisch]
                ImGui::SameLine();
                if (ImGui::Button("Select (Screen)", ImVec2(100, 20))) RenderFolderHeadless(inputDir_, pcViewer::RenderType::SCREEN);

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
            if (!(p.extension().string() == ".txt" || p.extension().string() == ".TXT" || p.extension().string() == ".obj" || p.extension().string() == ".OBJ" || p.extension().string() == ".params")) return false;
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

        namespace fs = std::filesystem;
        for (const auto& file : folderContent) {
            RenderFileHeadless(folder + "/" + file, renderType);
        }

        inputBatchMode_ = false;
    }

    void CoordinatorNode::RenderFileHeadless(const std::string& pointCloud, pcViewer::RenderType renderType)
    {
        namespace fs = std::filesystem;
        auto splitFilename = utils::split(fs::path(pointCloud).stem().string(), '_');
        if (splitFilename[0] == "parameters") return;
        if (splitFilename.size() < 2) return;

        bool render = false, renderScreen = false;
        if (splitFilename[1] == "ao" || splitFilename[1] == "matte" || splitFilename[1] == "subsurface") render = true;
        if (splitFilename.back() == "export" && splitFilename[splitFilename.size() - 2] != "screenfinal") renderScreen = true;

        if (!render && !renderScreen) return;

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
            std::ofstream pbrtOut{ namePrefix + ".pbrt" };
            std::ofstream pbrtOut_do{ namePrefix + "_direct_only.pbrt" };
            // GetCurrentRenderer()->ExportScreenPointCloud(*deferredExportHeadlessFBO_, namePrefix, infoOut, screenPoints, meshPoints);
            GetCurrentRenderer()->ExportPBRT(namePrefix, glm::uvec2(deferredExportHeadlessFBO_->GetWidth(), deferredExportHeadlessFBO_->GetHeight()), pbrtOut, pbrtOut_do);
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
            enh::TextureDescriptor texDesc(4, gl::GL_RGBA8, gl::GL_RGB, gl::GL_UNSIGNED_BYTE);
            enh::GLTexture::SaveTextureToFile(headlessFBO_->GetTextures()[0], texDesc, glm::uvec3(headlessFBO_->GetWidth(), headlessFBO_->GetHeight(), 1), out_filename);

            RenderersResetMeshes("");
            RenderersSetEnvironmentMap(nullptr);
            UnselectCurrentRenderer();
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

        RenderersLoadPointCloud(splitFilename[1] + "_" + splitFilename[splitFilename.size() - 1], pointCloud, glm::mat4());

        if (loadModel) {
            std::ifstream params_in(inputDir_ + "/parameters_" + splitFilename[1] + ".txt");
            std::string line;
            std::getline(params_in, line); // description line...

            std::size_t lineCounter = static_cast<std::size_t>(std::atoi(splitFilename[splitFilename.size() - 1].c_str())) + 2;
            float meshTheta = 0.0f, meshPhi = 0.0f;

            if (splitFilename[1] == "matte") lineCounter -= 20000;
            else if (splitFilename[1] == "subsurface") lineCounter -= 40000;

            glm::vec3 alpha{ 1.0f }, sigmat{ 1.0f };
            float eta = 1.0f;

            while (params_in.good()) {
                std::getline(params_in, line);
                auto splitParameters = utils::split(line, ',');
                if (splitParameters[0] != splitFilename[splitFilename.size() - 1]) continue;
                // if (--lineCounter != 0) continue;

                // assert(splitParameters[0] == splitFilename[splitFilename.size() - 1]);
                meshTheta = static_cast<float>(std::atof(splitParameters[4].c_str()));
                meshPhi = static_cast<float>(std::atof(splitParameters[5].c_str()));
                if (splitFilename[1] != "ao") {
                    alpha.r = static_cast<float>(std::atof(splitParameters[6].c_str()));
                    alpha.g = static_cast<float>(std::atof(splitParameters[7].c_str()));
                    alpha.b = static_cast<float>(std::atof(splitParameters[8].c_str()));
                }

                if (splitFilename[1] == "subsurface") {
                    sigmat.r = static_cast<float>(std::atof(splitParameters[9].c_str()));
                    sigmat.g = static_cast<float>(std::atof(splitParameters[10].c_str()));
                    sigmat.b = static_cast<float>(std::atof(splitParameters[11].c_str()));
                    eta = static_cast<float>(std::atof(splitParameters[12].c_str()));
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

            RenderersResetMeshes(splitFilename[splitFilename.size() - 3] + "-" + splitFilename[splitFilename.size() - 2]);
            pcViewer::MeshContainerEntryInfo meshInfo;
            meshInfo.mesh_ = GetMeshManager().GetResource(meshFilename);
            meshInfo.albedo_ = alpha;
            meshInfo.sigmat_ = sigmat;
            meshInfo.eta_ = eta;
            RenderersAddMesh(meshInfo, meshTheta, meshPhi, true);
            //SetMesh(GetMeshManager().GetResource("D:/Users/Sebastian Maisch/Documents/dev/deeplearning/ModelNet10/chair/train/chair_0009.off"), 0.0f, 0.0f);

        }
        else {
            RenderersResetMeshes("");
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

        std::string typeStr;
        if (type == pcViewer::PCType::AO) typeStr = "ao";
        else if (type == pcViewer::PCType::MATTE) typeStr = "matte";
        else if (type == pcViewer::PCType::SUBSURFACE) typeStr = "subsurface";

        auto meshPCPath = meshPath.parent_path() / (meshPath.stem().string() + "_" + typeStr + ".txt");
        if (fs::exists(meshPCPath)) {
            RenderersLoadPointCloud(meshPath.stem().string(), meshPCPath.string(), glm::mat4());
        }

        pcViewer::MeshContainerEntryInfo meshInfo;
        meshInfo.mesh_ = GetMeshManager().GetResource(meshFilename);
        meshInfo.albedo_ = glm::vec3(1.0f);
        meshInfo.sigmat_ = glm::vec3(1.0f);
        meshInfo.eta_ = 1.3f;
        RenderersSetEnvironmentMap(nullptr);
        RenderersResetMeshes(meshPath.stem().string());
        RenderersAddMesh(meshInfo, 0.f, 0.f, false);
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

    float parseFlt(const std::string& str) {
        return static_cast<float>(std::atof(str.c_str()));
    }

    glm::vec3 parseVec3(const std::string& str) {
        glm::vec3 res;
        auto splitStr = utils::split(str, ',');
        res.x = static_cast<float>(std::atof(splitStr[0].c_str()));
        res.y = static_cast<float>(std::atof(splitStr[1].c_str()));
        res.z = static_cast<float>(std::atof(splitStr[2].c_str()));
        return res;
    }

    glm::uvec2 parseUvec2(const std::string& str) {
        glm::uvec2 res;
        auto splitStr = utils::split(str, ',');
        res.x = static_cast<unsigned int>(std::atoi(splitStr[0].c_str()));
        res.y = static_cast<unsigned int>(std::atoi(splitStr[1].c_str()));
        return res;
    }

    glm::mat4 parseMat4(const std::string& str) {
        glm::mat4 res;
        auto splitStr = utils::split(str, ',');
        res[0][0] = static_cast<float>(std::atof(splitStr[0].c_str()));
        res[1][0] = static_cast<float>(std::atof(splitStr[1].c_str()));
        res[2][0] = static_cast<float>(std::atof(splitStr[2].c_str()));
        res[3][0] = static_cast<float>(std::atof(splitStr[3].c_str()));
        res[0][1] = static_cast<float>(std::atof(splitStr[4].c_str()));
        res[1][1] = static_cast<float>(std::atof(splitStr[5].c_str()));
        res[2][1] = static_cast<float>(std::atof(splitStr[6].c_str()));
        res[3][1] = static_cast<float>(std::atof(splitStr[7].c_str()));
        res[0][2] = static_cast<float>(std::atof(splitStr[8].c_str()));
        res[1][2] = static_cast<float>(std::atof(splitStr[9].c_str()));
        res[2][2] = static_cast<float>(std::atof(splitStr[10].c_str()));
        res[3][2] = static_cast<float>(std::atof(splitStr[11].c_str()));
        res[0][3] = static_cast<float>(std::atof(splitStr[12].c_str()));
        res[1][3] = static_cast<float>(std::atof(splitStr[13].c_str()));
        res[2][3] = static_cast<float>(std::atof(splitStr[14].c_str()));
        res[3][3] = static_cast<float>(std::atof(splitStr[15].c_str()));
        return res;
    }

    void CoordinatorNode::LoadParamsFile(const std::string& paramsname)
    {
        namespace fs = std::filesystem;
        fs::path paramsPath{ paramsname };

        inputFileSelected_ = true;
        std::ifstream paramsIn{ paramsname };
        std::string typeLine, pcFilename, pcModelMatrixStr, numMeshesStr;
        std::getline(paramsIn, typeLine); //
        std::getline(paramsIn, pcFilename); //
        std::getline(paramsIn, pcModelMatrixStr); //
        std::getline(paramsIn, numMeshesStr);

        if (typeLine == "ao") SelectRenderers(pcViewer::PCType::AO);
        else if (typeLine == "matte") SelectRenderers(pcViewer::PCType::MATTE);
        else if (typeLine == "subsurface") SelectRenderers(pcViewer::PCType::SUBSURFACE);


        auto modelMatrixPC = parseMat4(pcModelMatrixStr);
        auto meshPCPath = paramsPath.parent_path() / pcFilename;
        auto pcName = fs::path(pcFilename).filename().stem().string();

        if (fs::exists(meshPCPath)) {
            RenderersLoadPointCloud(pcName, meshPCPath.string(), modelMatrixPC);
        }

        auto numMeshes = static_cast<std::size_t>(std::atoi(numMeshesStr.c_str()));
        RenderersResetMeshes(pcName);
        for (std::size_t i = 0; i < numMeshes; ++i) {
            std::string meshFilename, albedoStr, sigmaTStr, etaStr, modelMatrixStr;
            std::getline(paramsIn, meshFilename); //
            std::getline(paramsIn, albedoStr); //
            std::getline(paramsIn, sigmaTStr); //
            std::getline(paramsIn, etaStr); //
            std::getline(paramsIn, modelMatrixStr);

            pcViewer::MeshContainerEntryInfo meshInfo;
            meshInfo.albedo_ = parseVec3(albedoStr);
            meshInfo.sigmat_ = parseVec3(sigmaTStr);
            meshInfo.eta_ = parseFlt(etaStr);
            auto modelMatrix = parseMat4(modelMatrixStr);
            

            auto meshPath = paramsPath.parent_path() / meshFilename;

            auto meshName = meshPath.filename().stem().string();

            if (fs::exists(meshPath)) {
                meshInfo.mesh_ = GetMeshManager().GetResource(meshPath.string());
                RenderersAddMesh(meshInfo, modelMatrix, false);
            }
        }

        std::string lightPosStr, lightColorStr, lightMulStr, resStr, viewMatrixStr, projMatrixStr;
        
        std::getline(paramsIn, lightPosStr); //
        std::getline(paramsIn, lightColorStr); //
        std::getline(paramsIn, lightMulStr); //
        std::getline(paramsIn, resStr); //
        std::getline(paramsIn, viewMatrixStr);
        std::getline(paramsIn, projMatrixStr);

        GetLightPosition() = parseVec3(lightPosStr);
        GetLightColor() = parseVec3(lightColorStr);
        GetLightMultiplicator() = parseFlt(lightMulStr);
        auto res = parseUvec2(resStr);
        assert(res.x == 1024 && res.y == 1024);

        viewportSize_ = res;

        auto viewMatrix = parseMat4(viewMatrixStr);
        auto projMatrix = parseMat4(projMatrixStr);

        GetCameraEnh().FixView(viewMatrix, projMatrix);

        outFolder_ = paramsPath.parent_path().string();

        RenderersSetEnvironmentMap(nullptr);
        UpdateBaseRendererType();
    }

    void CoordinatorNode::SaveImageAllTechniques(const std::string& name)
    {
        auto currentRenderType = baseRenderType_;

        std::ofstream timings_out{ name + "_timings.txt" };

        for (std::size_t i = 0; i < static_cast<std::size_t>(pcViewer::RenderType::SCREEN); ++i) {
            baseRenderType_ = static_cast<pcViewer::RenderType>(i);

            if (!GetCurrentRenderer()->IsAvaialble()) continue;

            headlessFBO_->DrawToFBO([this]() {
                gl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
                gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);
            });
            deferredHeadlessFBO_->DrawToFBO([this]() {
                gl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
                gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);
            });

            timings_out << GetCurrentRenderer()->GetRendererName() << ": ";
            timings_out << CurrentRendererDoPerformanceMeasure(*headlessFBO_, *deferredHeadlessFBO_, true);
            timings_out << std::endl;

            CurrentRendererDrawPointCloud(*headlessFBO_, *deferredHeadlessFBO_, true);

            auto out_filename = name + "_" + GetCurrentRenderer()->GetRendererName() +".png";
            enh::TextureDescriptor texDesc(4, gl::GL_RGBA8, gl::GL_RGB, gl::GL_UNSIGNED_BYTE);
            enh::GLTexture::SaveTextureToFile(headlessFBO_->GetTextures()[0], texDesc, glm::uvec3(headlessFBO_->GetWidth(), headlessFBO_->GetHeight(), 1), out_filename);
        }

        baseRenderType_ = currentRenderType;
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
        case GLFW_KEY_H:
            if (action == GLFW_PRESS && mods == GLFW_MOD_CONTROL) {
                hideGUI_ = !hideGUI_;
                return true;
            }
            break;
        default: return false;
        }

        return false;
    }

}
