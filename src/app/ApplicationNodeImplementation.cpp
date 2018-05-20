/**
 * @file   ApplicationNodeImplementation.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.30
 *
 * @brief  Implementation of the application node class.
 */

#include "ApplicationNodeImplementation.h"

#include "core/glfw.h"
#include <glbinding/gl/gl.h>
#include <glbinding/Binding.h>
#include <imgui.h>
#include <iostream>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Vertices.h"
#include "core/imgui/imgui_impl_glfw_gl3.h"
#include "enh/gfx/postprocessing/DepthOfField.h"
#include "enh/gfx/postprocessing/BloomEffect.h"
#include "enh/gfx/postprocessing/FilmicTMOperator.h"
// #include "core/gfx/mesh/MeshRenderable.h"

namespace viscom {

    ApplicationNodeImplementation::ApplicationNodeImplementation(ApplicationNodeInternal* appNode) :
        ApplicationNodeBase{ appNode },
        camera_(glm::vec3(0.0f, 0.0f, 5.0f), *appNode->GetCamera())
    {
    }

    ApplicationNodeImplementation::~ApplicationNodeImplementation() = default;

    void ApplicationNodeImplementation::InitOpenGL()
    {
        enh::ApplicationNodeBase::InitOpenGL();

        aoProgram_ = GetGPUProgramManager().GetResource("ao", std::vector<std::string>{ "showAO.vert", "showPCResult.frag" });
        aoMVPLoc_ = aoProgram_->getUniformLocation("MVP");

        matteProgram_ = GetGPUProgramManager().GetResource("matte", std::vector<std::string>{ "showMatte.vert", "showPCResult.frag" });
        matteMVPLoc_ = matteProgram_->getUniformLocation("MVP");
        matteRenderTypeLoc_ = matteProgram_->getUniformLocation("renderType");

        subsurfaceProgram_ = GetGPUProgramManager().GetResource("subsurface", std::vector<std::string>{ "showSubsurface.vert", "showPCResult.frag" });
        subsurfaceMVPLoc_ = subsurfaceProgram_->getUniformLocation("MVP");
        subsurfaceRenderTypeLoc_ = subsurfaceProgram_->getUniformLocation("renderType");

        gl::glGenBuffers(1, &vboPointCloud_);
        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, vboPointCloud_);
        gl::glGenVertexArrays(1, &vaoPointCloud_);
        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, 0);

        // FrameBufferDescriptor sceneFBODesc{ {
        //         FrameBufferTextureDescriptor{ static_cast<GLenum>(gl::GL_RGBA32F) },
        //         FrameBufferTextureDescriptor{ static_cast<GLenum>(gl::GL_RGBA32F) },
        //         FrameBufferTextureDescriptor{ static_cast<GLenum>(gl::GL_DEPTH_COMPONENT) } }, {} };
        // sceneFBOs_ = CreateOffscreenBuffers(sceneFBODesc);
        // 
        // dof_ = std::make_unique<enh::DepthOfField>(this);
        // bloom_ = std::make_unique<enh::BloomEffect>(this);
        // tm_ = std::make_unique<enh::FilmicTMOperator>(this);
    }

    void ApplicationNodeImplementation::UpdateFrame(double currentTime, double elapsedTime)
    {
        camera_.UpdateCamera(elapsedTime, this);

        // GetCamera()->SetPosition(camPos_);
        // glm::quat pitchQuat = glm::angleAxis(camRot_.x, glm::vec3(1.0f, 0.0f, 0.0f));
        // glm::quat yawQuat = glm::angleAxis(camRot_.y, glm::vec3(0.0f, 1.0f, 0.0f));
        // glm::quat rollQuat = glm::angleAxis(camRot_.z, glm::vec3(0.0f, 0.0f, 1.0f));
        // GetCamera()->SetOrientation(yawQuat * pitchQuat * rollQuat);

        // triangleModelMatrix_ = glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f)), 0.1f * static_cast<float>(currentTime), glm::vec3(0.0f, 1.0f, 0.0f));
        // teapotModelMatrix_ = glm::scale(glm::rotate(glm::translate(glm::mat4(0.01f), glm::vec3(-3.0f, 0.0f, -5.0f)), static_cast<float>(currentTime), glm::vec3(0.0f, 1.0f, 0.0f)), glm::vec3(0.01f));
    }

    void ApplicationNodeImplementation::ClearBuffer(FrameBuffer& fbo)
    {
        // SelectOffscreenBuffer(sceneFBOs_)->DrawToFBO([]() {
        //     gl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        //     gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);
        // });

        fbo.DrawToFBO([]() {
            gl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);
        });
    }

    void ApplicationNodeImplementation::DrawFrame(FrameBuffer& fbo)
    {
        // auto sceneFBO = SelectOffscreenBuffer(sceneFBOs_);
        // sceneFBO->DrawToFBO([this]() {
        // });

        fbo.DrawToFBO([this]() {
            DrawPointCloud();
        });

        // dof_->ApplyEffect(*GetCamera(), sceneFBO->GetTextures()[0], sceneFBO->GetTextures()[2], sceneFBO, 1);
        // tm_->ApplyTonemapping(sceneFBO->GetTextures()[1], sceneFBO, 0);
        // bloom_->ApplyEffect(sceneFBO->GetTextures()[0], &fbo);

        // fbo.DrawToFBO([this]() {});
    }

    void ApplicationNodeImplementation::DrawPointCloud()
    {
        gl::glEnable(gl::GL_PROGRAM_POINT_SIZE);

        gl::glBindVertexArray(vaoPointCloud_);
        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, vboPointCloud_);

        auto MVP = GetCamera()->GetViewPerspectiveMatrix();
        if (pcType_ == PCType::AO) {
            gl::glUseProgram(aoProgram_->getProgramId());
            gl::glUniformMatrix4fv(aoMVPLoc_, 1, gl::GL_FALSE, glm::value_ptr(MVP));
            gl::glDrawArrays(gl::GL_POINTS, 0, static_cast<gl::GLsizei>(pcAO_.size()));
        }

        if (pcType_ == PCType::MATTE) {
            gl::glUseProgram(matteProgram_->getProgramId());
            gl::glUniformMatrix4fv(matteMVPLoc_, 1, gl::GL_FALSE, glm::value_ptr(MVP));
            gl::glUniform1i(matteRenderTypeLoc_, matteRenderType_);
            gl::glDrawArrays(gl::GL_POINTS, 0, static_cast<gl::GLsizei>(pcMatte_.size()));
        }

        if (pcType_ == PCType::SUBSURFACE) {
            gl::glUseProgram(subsurfaceProgram_->getProgramId());
            gl::glUniformMatrix4fv(subsurfaceMVPLoc_, 1, gl::GL_FALSE, glm::value_ptr(MVP));
            gl::glUniform1i(subsurfaceRenderTypeLoc_, subsurfaceRenderType_);
            gl::glDrawArrays(gl::GL_POINTS, 0, static_cast<gl::GLsizei>(pcSubsurface_.size()));
        }

        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, 0);
        gl::glBindVertexArray(0);
        gl::glUseProgram(0);

        gl::glDisable(gl::GL_PROGRAM_POINT_SIZE);
    }

    void ApplicationNodeImplementation::CleanUp()
    {
        if (vaoPointCloud_ != 0) gl::glDeleteVertexArrays(1, &vaoPointCloud_);
        vaoPointCloud_ = 0;
        if (vboPointCloud_ != 0) gl::glDeleteBuffers(1, &vboPointCloud_);
        vboPointCloud_ = 0;

        // dof_ = nullptr;
        // tm_ = nullptr;
        // bloom_ = nullptr;
        // sceneFBOs_.clear();

        ApplicationNodeBase::CleanUp();
    }

    bool ApplicationNodeImplementation::MouseButtonCallback(int button, int action)
    {
        camera_.HandleMouse(button, action, 0.0f, this);
        return true;
    }

    bool ApplicationNodeImplementation::MousePosCallback(double x, double y)
    {
        camera_.HandleMouse(-1, 0, 0.0f, this);
        return true;
    }

    bool ApplicationNodeImplementation::MouseScrollCallback(double xoffset, double yoffset)
    {
        camera_.HandleMouse(-1, 0, static_cast<float>(yoffset), this);
        return true;
    }

    void ApplicationNodeImplementation::LoadPointCloudGPUAO(std::vector<PointCloudPointAO>& pointCloud)
    {
        using PointCloudPoint = PointCloudPointAO;

        pcAO_.swap(pointCloud);
        pcMatte_.clear();
        pcSubsurface_.clear();
        pcType_ = PCType::AO;

        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, vboPointCloud_);
        gl::glBufferData(gl::GL_ARRAY_BUFFER, pcAO_.size() * sizeof(PointCloudPoint), pcAO_.data(), gl::GL_STATIC_DRAW);

        gl::glBindVertexArray(vaoPointCloud_);
        gl::glEnableVertexAttribArray(0);
        gl::glVertexAttribPointer(0, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPoint), reinterpret_cast<GLvoid*>(offsetof(PointCloudPoint, position_)));
        gl::glEnableVertexAttribArray(1);
        gl::glVertexAttribPointer(1, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPoint), reinterpret_cast<GLvoid*>(offsetof(PointCloudPoint, normal_)));
        gl::glEnableVertexAttribArray(2);
        gl::glVertexAttribPointer(2, 1, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPoint), reinterpret_cast<GLvoid*>(offsetof(PointCloudPoint, ao_)));
        gl::glBindVertexArray(0);
        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, 0);
    }

    void ApplicationNodeImplementation::LoadPointCloudGPUMatte(std::vector<PointCloudPointMatte>& pointCloud)
    {
        using PointCloudPoint = PointCloudPointMatte;

        pcAO_.clear();
        pcMatte_.swap(pointCloud);
        pcSubsurface_.clear();
        pcType_ = PCType::MATTE;

        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, vboPointCloud_);
        gl::glBufferData(gl::GL_ARRAY_BUFFER, pcAO_.size() * sizeof(PointCloudPoint), pcAO_.data(), gl::GL_STATIC_DRAW);

        gl::glBindVertexArray(vaoPointCloud_);
        gl::glEnableVertexAttribArray(0);
        gl::glVertexAttribPointer(0, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPoint), reinterpret_cast<GLvoid*>(offsetof(PointCloudPoint, position_)));
        gl::glEnableVertexAttribArray(1);
        gl::glVertexAttribPointer(1, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPoint), reinterpret_cast<GLvoid*>(offsetof(PointCloudPoint, normal_)));
        gl::glEnableVertexAttribArray(2);
        gl::glVertexAttribPointer(2, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPoint), reinterpret_cast<GLvoid*>(offsetof(PointCloudPoint, albedo_)));
        gl::glEnableVertexAttribArray(3);
        gl::glVertexAttribPointer(3, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPoint), reinterpret_cast<GLvoid*>(offsetof(PointCloudPoint, directIllumination_)));
        gl::glEnableVertexAttribArray(4);
        gl::glVertexAttribPointer(4, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPoint), reinterpret_cast<GLvoid*>(offsetof(PointCloudPoint, globalIllumination_)));
        gl::glBindVertexArray(0);
        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, 0);
    }

    void ApplicationNodeImplementation::LoadPointCloudGPUSubsurface(std::vector<PointCloudPointSubsurface>& pointCloud)
    {
        using PointCloudPoint = PointCloudPointSubsurface;

        pcAO_.clear();
        pcMatte_.clear();
        pcSubsurface_.swap(pointCloud);
        pcType_ = PCType::SUBSURFACE;

        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, vboPointCloud_);
        gl::glBufferData(gl::GL_ARRAY_BUFFER, pcAO_.size() * sizeof(PointCloudPoint), pcAO_.data(), gl::GL_STATIC_DRAW);

        gl::glBindVertexArray(vaoPointCloud_);
        gl::glEnableVertexAttribArray(0);
        gl::glVertexAttribPointer(0, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPoint), reinterpret_cast<GLvoid*>(offsetof(PointCloudPoint, position_)));
        gl::glEnableVertexAttribArray(1);
        gl::glVertexAttribPointer(1, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPoint), reinterpret_cast<GLvoid*>(offsetof(PointCloudPoint, normal_)));
        gl::glEnableVertexAttribArray(2);
        gl::glVertexAttribPointer(2, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPoint), reinterpret_cast<GLvoid*>(offsetof(PointCloudPoint, albedo_)));
        gl::glEnableVertexAttribArray(3);
        gl::glVertexAttribPointer(3, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPoint), reinterpret_cast<GLvoid*>(offsetof(PointCloudPoint, sigma_tp_)));
        gl::glEnableVertexAttribArray(4);
        gl::glVertexAttribPointer(4, 1, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPoint), reinterpret_cast<GLvoid*>(offsetof(PointCloudPoint, eta_)));
        gl::glEnableVertexAttribArray(5);
        gl::glVertexAttribPointer(5, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPoint), reinterpret_cast<GLvoid*>(offsetof(PointCloudPoint, directIllumination_)));
        gl::glEnableVertexAttribArray(6);
        gl::glVertexAttribPointer(6, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPoint), reinterpret_cast<GLvoid*>(offsetof(PointCloudPoint, globalIllumination_)));
        gl::glBindVertexArray(0);
        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, 0);
    }

}
