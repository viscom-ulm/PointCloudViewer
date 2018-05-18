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

        FrameBufferDescriptor sceneFBODesc{ {
                FrameBufferTextureDescriptor{ static_cast<GLenum>(gl::GL_RGBA32F) },
                FrameBufferTextureDescriptor{ static_cast<GLenum>(gl::GL_RGBA32F) },
                FrameBufferTextureDescriptor{ static_cast<GLenum>(gl::GL_DEPTH_COMPONENT) } }, {} };
        sceneFBOs_ = CreateOffscreenBuffers(sceneFBODesc);

        dof_ = std::make_unique<enh::DepthOfField>(this);
        bloom_ = std::make_unique<enh::BloomEffect>(this);
        tm_ = std::make_unique<enh::FilmicTMOperator>(this);
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
        });

        dof_->ApplyEffect(*GetCamera(), sceneFBO->GetTextures()[0], sceneFBO->GetTextures()[2], sceneFBO, 1);
        tm_->ApplyTonemapping(sceneFBO->GetTextures()[1], sceneFBO, 0);
        bloom_->ApplyEffect(sceneFBO->GetTextures()[0], &fbo);

        // fbo.DrawToFBO([this]() {});
    }

    void ApplicationNodeImplementation::CleanUp()
    {
        dof_ = nullptr;
        tm_ = nullptr;
        bloom_ = nullptr;
        sceneFBOs_.clear();

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


}
