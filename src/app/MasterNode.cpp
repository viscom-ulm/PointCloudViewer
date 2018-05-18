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

        aoProgram_ = GetGPUProgramManager().GetResource("ao", std::vector<std::string>{ "showAO.vert", "showAO.frag" });
        aoMVPLoc_ = aoProgram_->getUniformLocation("MVP");

        gl::glGenBuffers(1, &vboPointCloud_);
        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, vboPointCloud_);
        gl::glGenVertexArrays(1, &vaoPointCloud_);
        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, 0);

        if (!singleFile_.empty()) {
            LoadPointCloud(singleFile_);
            // TODO: render and save image.
            exit(0);
        }
    }

    void MasterNode::DrawFrame(FrameBuffer& fbo)
    {
        ApplicationNodeImplementation::DrawFrame(fbo);

        fbo.DrawToFBO([this]() {
            gl::glEnable(gl::GL_PROGRAM_POINT_SIZE);

            gl::glBindVertexArray(vaoPointCloud_);
            gl::glBindBuffer(gl::GL_ARRAY_BUFFER, vboPointCloud_);

            auto MVP = GetCamera()->GetViewPerspectiveMatrix();
            if (pcType_ == PCType::AO) {
                gl::glUseProgram(aoProgram_->getProgramId());
                gl::glUniformMatrix4fv(aoMVPLoc_, 1, gl::GL_FALSE, glm::value_ptr(MVP));
                gl::glDrawArrays(gl::GL_POINTS, 0, static_cast<gl::GLsizei>(pcAO_.size()));
            }

            gl::glBindBuffer(gl::GL_ARRAY_BUFFER, 0);
            gl::glBindVertexArray(0);
            gl::glUseProgram(0);

            gl::glDisable(gl::GL_PROGRAM_POINT_SIZE);
        });
    }

    void MasterNode::Draw2D(FrameBuffer& fbo)
    {
        std::vector<std::string> supportedDriveLetters = { "A:/", "B:/", "C:/", "D:/", "E:/", "F:/", "G:/", "H:/" };
        namespace fs = std::filesystem;

        fbo.DrawToFBO([&]() {
            ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);
            ImGui::StyleColorsClassic();
            if (ImGui::Begin("", nullptr)) {
                GetDOF()->RenderParameterSliders();
                GetToneMapping()->RenderParameterSliders();
                GetBloom()->RenderParameterSliders();
            }
            ImGui::End();

            std::string selectedFile;
            ImGui::SetNextWindowPos(ImVec2(60, 60), ImGuiSetCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);
            ImGui::StyleColorsClassic();
            if (!inputFileSelected_ && ImGui::Begin("Select point cloud file", nullptr, ImGuiWindowFlags_MenuBar))
            {
                ImGui::Text(inputDir_.data());
                for (const auto& dl : supportedDriveLetters) {
                    if (fs::is_directory(dl)) {
                        bool selected = false;
                        ImGui::Selectable(dl.c_str(), &selected);
                        if (selected) inputDir_ = dl;
                    }
                }
                for (auto &path : GetDirectoryContent(inputDir_))
                {
                    bool selected = false;
                    ImGui::Selectable(path.data(), &selected);
                    if (selected) {
                        auto selection = fs::canonical(fs::path(inputDir_) / path).string();
                        if (fs::is_regular_file(selection)) selectedFilename_ = selection;
                        else inputDir_ = selection;
                    }
                }

                if (!selectedFilename_.empty()) inputFileSelected_ = ImGui::Button("Select File", ImVec2(50, 20));
                if (inputFileSelected_) LoadPointCloud(selectedFilename_);

                ImGui::End();
            }
        });

        ApplicationNodeImplementation::Draw2D(fbo);
    }


    std::vector<std::string> MasterNode::GetDirectoryContent(const std::string& dir) const
    {
        namespace fs = std::filesystem;

        std::vector<std::string> content;
        content.emplace_back("..");

        auto checkPCFile = [](const fs::path& p) { return (p.extension().string() == ".txt" || p.extension().string() == ".TXT"); };

        for (auto& p : fs::directory_iterator(dir)) {
            try {
                if (fs::is_regular_file(p) && checkPCFile(p)) content.push_back(p.path().filename().string());
                else if (fs::is_directory(p)) content.push_back(p.path().filename().string());
            } catch (fs::filesystem_error e) {}
        }
        return content;
    }

    void MasterNode::LoadPointCloud(const std::string& pointCloud)
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

        selectedFilename_ = "";
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

        pcAO_.swap(pointCloudData);
        pcMatte_.clear();
        pcSubsurface_.clear();
        pcType_ = PCType::AO;


        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, vboPointCloud_);
        gl::glBufferData(gl::GL_ARRAY_BUFFER, pcAO_.size() * sizeof(PointCloudPointAO), pcAO_.data(), gl::GL_STATIC_DRAW);

        gl::glBindVertexArray(vaoPointCloud_);
        gl::glEnableVertexAttribArray(0);
        gl::glVertexAttribPointer(0, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPointAO), reinterpret_cast<GLvoid*>(offsetof(PointCloudPointAO, position_)));
        gl::glEnableVertexAttribArray(1);
        gl::glVertexAttribPointer(1, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPointAO), reinterpret_cast<GLvoid*>(offsetof(PointCloudPointAO, normal_)));
        gl::glEnableVertexAttribArray(2);
        gl::glVertexAttribPointer(2, 1, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPointAO), reinterpret_cast<GLvoid*>(offsetof(PointCloudPointAO, ao_)));
        gl::glBindVertexArray(0);
        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, 0);
    }

    void MasterNode::LoadPointCloudMatte(const std::string& pointCloud)
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

        pcAO_.swap(pointCloudData);
        pcMatte_.clear();
        pcSubsurface_.clear();
        pcType_ = PCType::AO;


        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, vboPointCloud_);
        gl::glBufferData(gl::GL_ARRAY_BUFFER, pcAO_.size() * sizeof(PointCloudPointAO), pcAO_.data(), gl::GL_STATIC_DRAW);

        gl::glBindVertexArray(vaoPointCloud_);
        gl::glEnableVertexAttribArray(0);
        gl::glVertexAttribPointer(0, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPointAO), reinterpret_cast<GLvoid*>(offsetof(PointCloudPointAO, position_)));
        gl::glEnableVertexAttribArray(1);
        gl::glVertexAttribPointer(1, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPointAO), reinterpret_cast<GLvoid*>(offsetof(PointCloudPointAO, normal_)));
        gl::glEnableVertexAttribArray(2);
        gl::glVertexAttribPointer(2, 1, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPointAO), reinterpret_cast<GLvoid*>(offsetof(PointCloudPointAO, ao_)));
        gl::glBindVertexArray(0);
        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, 0);
    }

    void MasterNode::LoadPointCloudSubsurface(const std::string& pointCloud)
    {
    }

    bool MasterNode::KeyboardCallback(int key, int scancode, int action, int mods)
    {
        if (ApplicationNodeBase::KeyboardCallback(key, scancode, action, mods)) return true;

        switch (key)
        {
        case GLFW_KEY_L:
            if (action == GLFW_PRESS && mods == GLFW_MOD_CONTROL) {
                inputFileSelected_ = false;
                return true;
            }
            break;
        default: return false;
        }

        return false;
    }

    void MasterNode::CleanUp()
    {
        if (vaoPointCloud_ != 0) gl::glDeleteVertexArrays(1, &vaoPointCloud_);
        vaoPointCloud_ = 0;
        if (vboPointCloud_ != 0) gl::glDeleteBuffers(1, &vboPointCloud_);
        vboPointCloud_ = 0;

        ApplicationNodeImplementation::CleanUp();
    }

}
