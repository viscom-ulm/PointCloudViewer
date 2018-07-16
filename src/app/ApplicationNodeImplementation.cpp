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
#include "enh/gfx/postprocessing/DepthOfField.h"
#include "enh/gfx/postprocessing/BloomEffect.h"
#include "enh/gfx/postprocessing/FilmicTMOperator.h"
// #include "core/gfx/mesh/MeshRenderable.h"

namespace viscom {

    ApplicationNodeImplementation::ApplicationNodeImplementation(ApplicationNodeInternal* appNode) :
        ApplicationNodeBase{ appNode }
    {
    }

    ApplicationNodeImplementation::~ApplicationNodeImplementation() = default;

    void ApplicationNodeImplementation::InitOpenGL()
    {
        enh::ApplicationNodeBase::InitOpenGL();

        backgroundProgram_ = GetGPUProgramManager().GetResource("backgroundGrid", std::vector<std::string>{ "backgroundGrid.vert", "backgroundGrid.frag" });
        backgroundMVPLoc_ = backgroundProgram_->getUniformLocation("MVP");

        triangleProgram_ = GetGPUProgramManager().GetResource("foregroundTriangle", std::vector<std::string>{ "foregroundTriangle.vert", "foregroundTriangle.frag" });
        triangleMVPLoc_ = triangleProgram_->getUniformLocation("MVP");

        teapotProgram_ = GetGPUProgramManager().GetResource("foregroundMesh", std::vector<std::string>{ "foregroundMesh.vert", "foregroundMesh.frag" });
        teapotModelMLoc_ = teapotProgram_->getUniformLocation("modelMatrix");
        teapotNormalMLoc_ = teapotProgram_->getUniformLocation("normalMatrix");
        teapotVPLoc_ = teapotProgram_->getUniformLocation("viewProjectionMatrix");

        std::vector<GridVertex> gridVertices;

        auto delta = 0.125f;
        for (auto x = -1.0f; x < 1.0f; x += delta) {
            auto green = (x + 1.0f) / 2.0f;

            for (float y = -1.0; y < 1.0; y += delta) {
                auto red = (y + 1.0f) / 2.0f;

                auto dx = 0.004f;
                auto dy = 0.004f;

                gridVertices.emplace_back(glm::vec3(x + dx, y + dy, -1.0f), glm::vec4(red, green, 0.0f, 1.0f));//right top
                gridVertices.emplace_back(glm::vec3(x - dx + delta, y + dy, -1.0f), glm::vec4(red, green, 0.0f, 1.0f));//left top
                gridVertices.emplace_back(glm::vec3(x - dx + delta, y - dy + delta, -1.0f), glm::vec4(red, green, 0.0f, 1.0f));//left bottom

                gridVertices.emplace_back(glm::vec3(x - dx + delta, y - dy + delta, -1.0f), glm::vec4(red, green, 0.0f, 1.0f));//left bottom
                gridVertices.emplace_back(glm::vec3(x + dx, y - dy + delta, -1.0f), glm::vec4(red, green, 0.0f, 1.0f));//right bottom
                gridVertices.emplace_back(glm::vec3(x + dx, y + dy, -1.0f), glm::vec4(red, green, 0.0f, 1.0f));//right top
            }
        }

        numBackgroundVertices_ = static_cast<unsigned int>(gridVertices.size());

        gridVertices.emplace_back(glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
        gridVertices.emplace_back(glm::vec3(0.0f, 0.5f, 0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
        gridVertices.emplace_back(glm::vec3(0.5f, -0.5f, 0.0f), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));

        gl::glGenBuffers(1, &vboBackgroundGrid_);
        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, vboBackgroundGrid_);
        gl::glBufferData(gl::GL_ARRAY_BUFFER, gridVertices.size() * sizeof(GridVertex), gridVertices.data(), gl::GL_STATIC_DRAW);

        gl::glGenVertexArrays(1, &vaoBackgroundGrid_);
        gl::glBindVertexArray(vaoBackgroundGrid_);
        gl::glEnableVertexAttribArray(0);
        gl::glVertexAttribPointer(0, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(GridVertex), reinterpret_cast<GLvoid*>(offsetof(GridVertex, position_)));
        gl::glEnableVertexAttribArray(1);
        gl::glVertexAttribPointer(1, 4, gl::GL_FLOAT, gl::GL_FALSE, sizeof(GridVertex), reinterpret_cast<GLvoid*>(offsetof(GridVertex, color_))); //-V112
        gl::glBindVertexArray(0);

        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, 0);

        teapotMesh_ = GetMeshManager().GetResource("/models/teapot/teapot.obj");
        // teapotRenderable_ = MeshRenderable::create<SimpleMeshVertex>(teapotMesh_.get(), teapotProgram_.get());

        FrameBufferDescriptor sceneFBODesc{ {
                FrameBufferTextureDescriptor{ static_cast<GLenum>(gl::GL_RGBA32F) },
                FrameBufferTextureDescriptor{ static_cast<GLenum>(gl::GL_RGBA32F) },
                FrameBufferTextureDescriptor{ static_cast<GLenum>(gl::GL_DEPTH_COMPONENT) } }, {} };
        sceneFBOs_ = CreateOffscreenBuffers(sceneFBODesc);

        dof_ = std::make_unique<enh::DepthOfField>(this);
        bloom_ = std::make_unique<enh::BloomEffect>(this);
        tm_ = std::make_unique<enh::FilmicTMOperator>(this);
    }

    void ApplicationNodeImplementation::UpdateFrame(double currentTime, double)
    {
        GetCamera()->SetPosition(camPos_);
        glm::quat pitchQuat = glm::angleAxis(camRot_.x, glm::vec3(1.0f, 0.0f, 0.0f));
        glm::quat yawQuat = glm::angleAxis(camRot_.y, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::quat rollQuat = glm::angleAxis(camRot_.z, glm::vec3(0.0f, 0.0f, 1.0f));
        GetCamera()->SetOrientation(yawQuat * pitchQuat * rollQuat);

        triangleModelMatrix_ = glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f)), 0.1f * static_cast<float>(currentTime), glm::vec3(0.0f, 1.0f, 0.0f));
        teapotModelMatrix_ = glm::scale(glm::rotate(glm::translate(glm::mat4(0.01f), glm::vec3(-3.0f, 0.0f, -5.0f)), static_cast<float>(currentTime), glm::vec3(0.0f, 1.0f, 0.0f)), glm::vec3(0.01f));
    }

    void ApplicationNodeImplementation::ClearBuffer(FrameBuffer& fbo)
    {
        SelectOffscreenBuffer(sceneFBOs_)->DrawToFBO([]() {
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
        auto sceneFBO = SelectOffscreenBuffer(sceneFBOs_);
        sceneFBO->DrawToFBO([this]() {
            gl::glBindVertexArray(vaoBackgroundGrid_);
            gl::glBindBuffer(gl::GL_ARRAY_BUFFER, vboBackgroundGrid_);

            auto MVP = GetCamera()->GetViewPerspectiveMatrix();
            {
                gl::glUseProgram(backgroundProgram_->getProgramId());
                gl::glUniformMatrix4fv(backgroundMVPLoc_, 1, gl::GL_FALSE, glm::value_ptr(MVP));
                // gl::glDrawArrays(gl::GL_TRIANGLES, 0, numBackgroundVertices_);
            }

            for (std::size_t i = 0; i < 50; ++i) {
                gl::glDisable(gl::GL_CULL_FACE);
                auto triangleMVP = MVP * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0f * static_cast<float>(i - 8))) * triangleModelMatrix_;
                gl::glUseProgram(triangleProgram_->getProgramId());
                gl::glUniformMatrix4fv(triangleMVPLoc_, 1, gl::GL_FALSE, glm::value_ptr(triangleMVP));
                gl::glDrawArrays(gl::GL_TRIANGLES, numBackgroundVertices_, 3);
                gl::glEnable(gl::GL_CULL_FACE);
            }

            {
                gl::glUseProgram(teapotProgram_->getProgramId());
                auto normalMatrix = glm::inverseTranspose(glm::mat3(teapotModelMatrix_));
                gl::glUniformMatrix4fv(teapotModelMLoc_, 1, gl::GL_FALSE, glm::value_ptr(teapotModelMatrix_));
                gl::glUniformMatrix3fv(teapotNormalMLoc_, 1, gl::GL_FALSE, glm::value_ptr(normalMatrix));
                gl::glUniformMatrix4fv(teapotVPLoc_, 1, gl::GL_FALSE, glm::value_ptr(MVP));
                // teapotRenderable_->Draw(teapotModelMatrix_);
            }

            gl::glBindBuffer(gl::GL_ARRAY_BUFFER, 0);
            gl::glBindVertexArray(0);
            gl::glUseProgram(0);
        });

        dof_->ApplyEffect(*GetCamera(), sceneFBO->GetTextures()[0], sceneFBO->GetTextures()[2], sceneFBO, 1);
        tm_->ApplyTonemapping(sceneFBO->GetTextures()[1], sceneFBO, 0);
        bloom_->ApplyEffect(sceneFBO->GetTextures()[0], &fbo);

        // fbo.DrawToFBO([this]() {});
    }

    void ApplicationNodeImplementation::CleanUp()
    {
        if (vaoBackgroundGrid_ != 0) gl::glDeleteVertexArrays(1, &vaoBackgroundGrid_);
        vaoBackgroundGrid_ = 0;
        if (vboBackgroundGrid_ != 0) gl::glDeleteBuffers(1, &vboBackgroundGrid_);
        vboBackgroundGrid_ = 0;

        dof_ = nullptr;
        tm_ = nullptr;
        bloom_ = nullptr;
        sceneFBOs_.clear();

        teapotMesh_ = nullptr;
        teapotProgram_ = nullptr;
        triangleProgram_ = nullptr;
        backgroundProgram_ = nullptr;

        ApplicationNodeBase::CleanUp();
    }

    bool ApplicationNodeImplementation::KeyboardCallback(int key, int scancode, int action, int mods)
    {
        if (ApplicationNodeBase::KeyboardCallback(key, scancode, action, mods)) return true;

        switch (key)
        {
        case GLFW_KEY_W:
            if (action == GLFW_REPEAT || action == GLFW_PRESS) camPos_ += glm::vec3(0.0, 0.0, -0.001);
            return true;

        case GLFW_KEY_S:
            if (action == GLFW_REPEAT || action == GLFW_PRESS) camPos_ += glm::vec3(0.0, 0.0, 0.001);
            return true;

        case GLFW_KEY_A:
            if (action == GLFW_REPEAT || action == GLFW_PRESS) camPos_ += glm::vec3(-0.001, 0.0, 0.0);
            return true;

        case GLFW_KEY_D:
            if (action == GLFW_REPEAT || action == GLFW_PRESS) camPos_ += glm::vec3(0.001, 0.0, 0.0);
            return true;

        case GLFW_KEY_LEFT_CONTROL:
            if (action == GLFW_REPEAT || action == GLFW_PRESS) camPos_ += glm::vec3(0.0, -0.001, 0.0);
            return true;

        case GLFW_KEY_LEFT_SHIFT:
            if (action == GLFW_REPEAT || action == GLFW_PRESS) camPos_ += glm::vec3(0.0, 0.001, 0.0);
            return true;

        case GLFW_KEY_UP:
            if (action == GLFW_REPEAT || action == GLFW_PRESS) camRot_ += glm::vec3(-0.002, 0.0, 0.0);
            return true;

        case GLFW_KEY_DOWN:
            if (action == GLFW_REPEAT || action == GLFW_PRESS) camRot_ += glm::vec3(0.002, 0.0, 0.0);
            return true;

        case GLFW_KEY_LEFT:
            if (action == GLFW_REPEAT || action == GLFW_PRESS) camRot_ += glm::vec3(0.0, -0.002, 0.0);
            return true;

        case GLFW_KEY_RIGHT:
            if (action == GLFW_REPEAT || action == GLFW_PRESS) camRot_ += glm::vec3(0.0, 0.002, 0.0);
            return true;

        case GLFW_KEY_Q:
            if (action == GLFW_REPEAT || action == GLFW_PRESS) camRot_ += glm::vec3(0.0, 0.0, -0.002);
            return true;

        case GLFW_KEY_E:
            if (action == GLFW_REPEAT || action == GLFW_PRESS) camRot_ += glm::vec3(0.0, 0.0, 0.002);
            return true;
        }
        return false;
    }

    bool ApplicationNodeImplementation::MouseButtonCallback(int button, int action)
    {
        auto test = GetCamera()->GetPickPosition(glm::vec2(0.75f, 0.5f));
        return true;
    }

}
