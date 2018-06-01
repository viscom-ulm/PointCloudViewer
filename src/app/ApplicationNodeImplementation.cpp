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
#include "enh/gfx/env/EnvironmentMapRenderer.h"
#include "app/gfx/mesh/MeshRenderable.h"
#include "app/Vertices.h"

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
        aoBBRLoc_ = aoProgram_->getUniformLocation("bbRadius");

        matteProgram_ = GetGPUProgramManager().GetResource("matte", std::vector<std::string>{ "showMatte.vert", "showPCResult.frag" });
        matteMVPLoc_ = matteProgram_->getUniformLocation("MVP");
        matteRenderTypeLoc_ = matteProgram_->getUniformLocation("renderType");
        matteBBRLoc_ = matteProgram_->getUniformLocation("bbRadius");

        subsurfaceProgram_ = GetGPUProgramManager().GetResource("subsurface", std::vector<std::string>{ "showSubsurface.vert", "showPCResult.frag" });
        subsurfaceMVPLoc_ = subsurfaceProgram_->getUniformLocation("MVP");
        subsurfaceRenderTypeLoc_ = subsurfaceProgram_->getUniformLocation("renderType");
        subsurfaceBBRLoc_ = subsurfaceProgram_->getUniformLocation("bbRadius");


        deferredProgram_ = GetGPUProgramManager().GetResource("deferredMesh", std::vector<std::string>{ "deferredMesh.vert", "deferredMesh.frag" });
        deferredUniformLocations_ = deferredProgram_->GetUniformLocations({ "viewProjection" });

        distanceSumAOProgram_ = GetGPUProgramManager().GetResource("distanceSumAO", std::vector<std::string>{ "distanceSumAO.vert", "distanceSum.frag" });
        distanceSumAOUniformLocations_ = distanceSumAOProgram_->GetUniformLocations({ "viewProjection", "positionTexture", "normalTexture", "distancePower", "pointSize" });

        distanceSumMatteProgram_ = GetGPUProgramManager().GetResource("distanceSumMatte", std::vector<std::string>{ "distanceSumMatte.vert", "distanceSum.frag" });
        distanceSumMatteUniformLocations_ = distanceSumMatteProgram_->GetUniformLocations({ "viewProjection", "positionTexture", "normalTexture", "distancePower", "pointSize" });

        distanceSumSubsurfaceProgram_ = GetGPUProgramManager().GetResource("distanceSumSubsurface", std::vector<std::string>{ "distanceSumSubsurface.vert", "distanceSum.frag" });
        distanceSumSubsurfaceUniformLocations_ = distanceSumSubsurfaceProgram_->GetUniformLocations({ "viewProjection", "positionTexture", "normalTexture", "distancePower", "pointSize" });

        finalQuad_ = std::make_unique<FullscreenQuad>("finalComposite.frag", this);
        finalUniformLocations_ = finalQuad_->GetGPUProgram()->GetUniformLocations({ "positionTexture", "normalTexture", "materialColorTexture", "directIlluminationTexture", "globalIlluminationTexture" });

        gl::glGenBuffers(1, &vboPointCloud_);
        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, vboPointCloud_);
        gl::glGenVertexArrays(1, &vaoPointCloud_);
        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, 0);

        FrameBufferDescriptor deferredFBODesc{ {
                FrameBufferTextureDescriptor{ static_cast<GLenum>(gl::GL_RGBA32F) }, // position
                FrameBufferTextureDescriptor{ static_cast<GLenum>(gl::GL_RGBA32F) }, // normal
                FrameBufferTextureDescriptor{ static_cast<GLenum>(gl::GL_RGBA32F) }, // color
                FrameBufferTextureDescriptor{ static_cast<GLenum>(gl::GL_RGBA32F) }, // direct illumination
                FrameBufferTextureDescriptor{ static_cast<GLenum>(gl::GL_RGBA32F) }, // global illumination
                FrameBufferTextureDescriptor{ static_cast<GLenum>(gl::GL_DEPTH_COMPONENT32F) } }, {} };
        deferredFBOs_ = CreateOffscreenBuffers(deferredFBODesc);

        deferredDrawIndices_ = { 0, 1, 2 };
        distanceSumDrawIndices_ = { 3, 4 };

        envMapRenderer_ = std::make_unique<enh::EnvironmentMapRenderer>(this);
        // 
        // dof_ = std::make_unique<enh::DepthOfField>(this);
        // bloom_ = std::make_unique<enh::BloomEffect>(this);
        // tm_ = std::make_unique<enh::FilmicTMOperator>(this);
    }

    void ApplicationNodeImplementation::UpdateFrame(double currentTime, double elapsedTime)
    {
        camera_.UpdateCamera(elapsedTime, this);
    }

    void ApplicationNodeImplementation::ClearBuffer(FrameBuffer& fbo)
    {
        SelectOffscreenBuffer(deferredFBOs_)->DrawToFBO([]() {
             gl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
             gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);
        });

        fbo.DrawToFBO([]() {
            gl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);
        });
    }

    void ApplicationNodeImplementation::DrawFrame(FrameBuffer& fbo)
    {
        auto deferredFBO = SelectOffscreenBuffer(deferredFBOs_);
        DrawPointCloud(fbo, *deferredFBO, false);
        // sceneFBO->DrawToFBO([this]() {
        // });

        // dof_->ApplyEffect(*GetCamera(), sceneFBO->GetTextures()[0], sceneFBO->GetTextures()[2], sceneFBO, 1);
        // tm_->ApplyTonemapping(sceneFBO->GetTextures()[1], sceneFBO, 0);
        // bloom_->ApplyEffect(sceneFBO->GetTextures()[0], &fbo);

        // fbo.DrawToFBO([this]() {});
    }

    void ApplicationNodeImplementation::DrawPointCloud(const FrameBuffer& fbo, const FrameBuffer& deferredFBO, bool batched)
    {
        if (envMap_) {
            fbo.DrawToFBO([this]() {
                envMapRenderer_->Draw(*GetCamera(), envMap_->getTextureId());
            });
        }

        if (!mesh_) {
            fbo.DrawToFBO([this, batched]() {
                DrawPointCloudPoints(batched);
            });
        }
        else {
            deferredFBO.DrawToFBO(deferredDrawIndices_, [this]() {
                DrawMeshDeferred();
            });

            deferredFBO.DrawToFBO(distanceSumDrawIndices_, [this, &deferredFBO]() {
                DrawPointCloudDistanceSum(deferredFBO);
            });

            fbo.DrawToFBO([this, &deferredFBO]() {
                DrawPointCloudOnMesh(deferredFBO);
            });
        }
    }

    void ApplicationNodeImplementation::DrawPointCloudPoints(bool batched)
    {
        gl::glEnable(gl::GL_PROGRAM_POINT_SIZE);
        gl::glEnable(gl::GL_BLEND);
        gl::glBlendFunc(gl::GL_SRC_ALPHA, gl::GL_ONE_MINUS_SRC_ALPHA);

        gl::glBindVertexArray(vaoPointCloud_);
        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, vboPointCloud_);

        glm::mat4 modelMatrix(0.8f);
        auto MVP = GetCamera()->GetViewPerspectiveMatrix() * modelMatrix;
        if (pcType_ == PCType::AO) {
            gl::glUseProgram(aoProgram_->getProgramId());
            gl::glUniformMatrix4fv(aoMVPLoc_, 1, gl::GL_FALSE, glm::value_ptr(MVP));
            gl::glUniform1f(aoBBRLoc_, batched ? boundingSphereRadius_ / 2.f : boundingSphereRadius_);
            gl::glDrawArrays(gl::GL_POINTS, 0, static_cast<gl::GLsizei>(pcAO_.size()));
        }

        if (pcType_ == PCType::MATTE) {
            gl::glUseProgram(matteProgram_->getProgramId());
            gl::glUniformMatrix4fv(matteMVPLoc_, 1, gl::GL_FALSE, glm::value_ptr(MVP));
            gl::glUniform1i(matteRenderTypeLoc_, matteRenderType_);
            gl::glUniform1f(matteBBRLoc_, batched ? boundingSphereRadius_ / 2.f : boundingSphereRadius_);
            gl::glDrawArrays(gl::GL_POINTS, 0, static_cast<gl::GLsizei>(pcMatte_.size()));
        }

        if (pcType_ == PCType::SUBSURFACE) {
            gl::glUseProgram(subsurfaceProgram_->getProgramId());
            gl::glUniformMatrix4fv(subsurfaceMVPLoc_, 1, gl::GL_FALSE, glm::value_ptr(MVP));
            gl::glUniform1i(subsurfaceRenderTypeLoc_, subsurfaceRenderType_);
            gl::glUniform1f(subsurfaceBBRLoc_, batched ? boundingSphereRadius_ / 2.f : boundingSphereRadius_);
            gl::glDrawArrays(gl::GL_POINTS, 0, static_cast<gl::GLsizei>(pcSubsurface_.size()));
        }

        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, 0);
        gl::glBindVertexArray(0);
        gl::glUseProgram(0);


        gl::glDisable(gl::GL_BLEND);
        gl::glDisable(gl::GL_PROGRAM_POINT_SIZE);
    }

    void ApplicationNodeImplementation::DrawMeshDeferred()
    {
        auto VP = GetCamera()->GetViewPerspectiveMatrix();

        gl::glCullFace(gl::GL_FRONT);
        glm::mat4 modelMatrix(10.f);
        modelMatrix[3][3] = 1.f;
        gl::glUseProgram(deferredProgram_->getProgramId());
        gl::glUniformMatrix4fv(deferredUniformLocations_[0], 1, gl::GL_FALSE, glm::value_ptr(VP));
        meshRenderable_->Draw(meshModel_ * modelMatrix);

        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, 0);
        gl::glBindVertexArray(0);
        gl::glUseProgram(0);

        gl::glCullFace(gl::GL_BACK);
    }

    void ApplicationNodeImplementation::DrawPointCloudDistanceSum(const FrameBuffer& deferredFBO)
    {
        gl::glEnable(gl::GL_PROGRAM_POINT_SIZE);

        gl::glBindVertexArray(vaoPointCloud_);
        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, vboPointCloud_);

        gl::glDisable(gl::GL_DEPTH_TEST);
        gl::glDepthMask(gl::GL_FALSE);
        gl::glEnable(gl::GL_BLEND);
        gl::glBlendEquationSeparate(gl::GL_FUNC_ADD, gl::GL_FUNC_ADD);
        gl::glBlendFuncSeparate(gl::GL_ONE, gl::GL_ONE, gl::GL_ONE, gl::GL_ONE);

        auto MVP = GetCamera()->GetViewPerspectiveMatrix();
        if (pcType_ == PCType::AO) {
            gl::glUseProgram(distanceSumAOProgram_->getProgramId());
            gl::glUniformMatrix4fv(distanceSumAOUniformLocations_[0], 1, gl::GL_FALSE, glm::value_ptr(MVP));

            gl::glActiveTexture(gl::GL_TEXTURE2);
            gl::glBindTexture(gl::GL_TEXTURE_2D, deferredFBO.GetTextures()[0]);
            gl::glUniform1i(distanceSumAOUniformLocations_[1], 2);

            gl::glActiveTexture(gl::GL_TEXTURE3);
            gl::glBindTexture(gl::GL_TEXTURE_2D, deferredFBO.GetTextures()[1]);
            gl::glUniform1i(distanceSumAOUniformLocations_[2], 3);
            gl::glUniform1f(distanceSumAOUniformLocations_[3], 1.0f);
            gl::glDrawArrays(gl::GL_POINTS, 0, static_cast<gl::GLsizei>(pcAO_.size()));
        }

        if (pcType_ == PCType::MATTE) {
            gl::glUseProgram(distanceSumMatteProgram_->getProgramId());
            gl::glUniformMatrix4fv(distanceSumMatteUniformLocations_[0], 1, gl::GL_FALSE, glm::value_ptr(MVP));

            gl::glActiveTexture(gl::GL_TEXTURE2);
            gl::glBindTexture(gl::GL_TEXTURE_2D, deferredFBO.GetTextures()[0]);
            gl::glUniform1i(distanceSumMatteUniformLocations_[1], 2);

            gl::glActiveTexture(gl::GL_TEXTURE3);
            gl::glBindTexture(gl::GL_TEXTURE_2D, deferredFBO.GetTextures()[1]);
            gl::glUniform1i(distanceSumMatteUniformLocations_[2], 3);
            gl::glUniform1f(distanceSumMatteUniformLocations_[3], 1.0f);
            gl::glDrawArrays(gl::GL_POINTS, 0, static_cast<gl::GLsizei>(pcMatte_.size()));
        }

        if (pcType_ == PCType::SUBSURFACE) {
            gl::glUseProgram(distanceSumSubsurfaceProgram_->getProgramId());
            gl::glUniformMatrix4fv(distanceSumSubsurfaceUniformLocations_[0], 1, gl::GL_FALSE, glm::value_ptr(MVP));

            gl::glActiveTexture(gl::GL_TEXTURE2);
            gl::glBindTexture(gl::GL_TEXTURE_2D, deferredFBO.GetTextures()[0]);
            gl::glUniform1i(distanceSumSubsurfaceUniformLocations_[1], 2);

            gl::glActiveTexture(gl::GL_TEXTURE3);
            gl::glBindTexture(gl::GL_TEXTURE_2D, deferredFBO.GetTextures()[1]);
            gl::glUniform1i(distanceSumSubsurfaceUniformLocations_[2], 3);
            gl::glUniform1f(distanceSumSubsurfaceUniformLocations_[3], 1.0f);
            gl::glDrawArrays(gl::GL_POINTS, 0, static_cast<gl::GLsizei>(pcSubsurface_.size()));
        }

        gl::glDisable(gl::GL_BLEND);
        gl::glDepthMask(gl::GL_TRUE);
        gl::glEnable(gl::GL_DEPTH_TEST);

        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, 0);
        gl::glBindVertexArray(0);
        gl::glUseProgram(0);

        gl::glDisable(gl::GL_PROGRAM_POINT_SIZE);
    }

    void ApplicationNodeImplementation::DrawPointCloudOnMesh(const FrameBuffer& deferredFBO)
    {
        gl::glUseProgram(finalQuad_->GetGPUProgram()->getProgramId());

        for (int i = 0; i < 5; ++i) {
            gl::glActiveTexture(gl::GL_TEXTURE0 + i);
            gl::glBindTexture(gl::GL_TEXTURE_2D, deferredFBO.GetTextures()[i]);
            gl::glUniform1i(finalUniformLocations_[i], i);
        }

        // gl::glUniformMatrix4fv(deferredUniformLocations_[0], 1, gl::GL_FALSE, glm::value_ptr(VP));
        finalQuad_->Draw();

        gl::glUseProgram(0);
    }

    void ApplicationNodeImplementation::SetMesh(std::shared_ptr<Mesh> mesh, float theta, float phi)
    {
        mesh_ = std::move(mesh);
        if (mesh_) meshRenderable_ = enh::MeshRenderable::create<SimpleMeshVertex>(mesh_.get(), deferredProgram_.get());

        meshModel_ = glm::rotate(glm::mat4(1.0f), theta, glm::vec3(1.0f, 0.0f, 0.0f));
        meshModel_ = meshModel_ * glm::rotate(glm::mat4(1.0f), phi, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    void ApplicationNodeImplementation::CleanUp()
    {
        if (vaoPointCloud_ != 0) gl::glDeleteVertexArrays(1, &vaoPointCloud_);
        vaoPointCloud_ = 0;
        if (vboPointCloud_ != 0) gl::glDeleteBuffers(1, &vboPointCloud_);
        vboPointCloud_ = 0;

        meshRenderable_ = nullptr;

        // dof_ = nullptr;
        // tm_ = nullptr;
        // bloom_ = nullptr;
        // sceneFBOs_.clear();
        deferredFBOs_.clear();

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

        camera_.SetCameraPosition(boundingSphereRadius_ * GetCamera()->GetCentralPerspectiveMatrix()[1][1] * glm::normalize(camera_.GetPosition()));
    }

    void ApplicationNodeImplementation::LoadPointCloudGPUMatte(std::vector<PointCloudPointMatte>& pointCloud)
    {
        using PointCloudPoint = PointCloudPointMatte;

        pcAO_.clear();
        pcMatte_.swap(pointCloud);
        pcSubsurface_.clear();
        pcType_ = PCType::MATTE;

        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, vboPointCloud_);
        gl::glBufferData(gl::GL_ARRAY_BUFFER, pcMatte_.size() * sizeof(PointCloudPoint), pcMatte_.data(), gl::GL_STATIC_DRAW);

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

        camera_.SetCameraPosition(boundingSphereRadius_ * GetCamera()->GetCentralPerspectiveMatrix()[1][1] * glm::normalize(camera_.GetPosition()));
    }

    void ApplicationNodeImplementation::LoadPointCloudGPUSubsurface(std::vector<PointCloudPointSubsurface>& pointCloud)
    {
        using PointCloudPoint = PointCloudPointSubsurface;

        pcAO_.clear();
        pcMatte_.clear();
        pcSubsurface_.swap(pointCloud);
        pcType_ = PCType::SUBSURFACE;

        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, vboPointCloud_);
        gl::glBufferData(gl::GL_ARRAY_BUFFER, pcSubsurface_.size() * sizeof(PointCloudPoint), pcSubsurface_.data(), gl::GL_STATIC_DRAW);

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

        camera_.SetCameraPosition(boundingSphereRadius_ * GetCamera()->GetCentralPerspectiveMatrix()[1][1] * glm::normalize(camera_.GetPosition()));
    }

}
