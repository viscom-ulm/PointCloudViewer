/**
 * @file   ArcballCameraEnhanced.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2017.06.07
 *
 * @brief  Implementation of an arcball camera.
 */

#include "ArcballCameraEnhanced.h"

#define GLM_SWIZZLE
#include "core/glfw.h"
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>

namespace viscom::enh {

    /**
     *  Constructor.
     *  @param theCamPos the cameras initial position.
     *  @param cameraHelper the camera helper class.
     */
    ArcballCameraEnhanced::ArcballCameraEnhanced(const glm::vec3& theCamPos, viscom::CameraHelper& cameraHelper) noexcept :
        CameraBase(theCamPos, cameraHelper),
        baseCamPos_{ glm::normalize(theCamPos) },
        mouseWheelDelta_{ 0.0f },
        camArcball_{ GLFW_MOUSE_BUTTON_1 }
    {
    }

    ArcballCameraEnhanced::~ArcballCameraEnhanced() = default;

    /**
     *  Updates the camera parameters using the internal arc-ball.
     */
    void ArcballCameraEnhanced::UpdateCamera(double elapsedTime, const ApplicationNodeBase*)
    {
        const double mouseWheelSpeed = 8.0;

        auto radius = glm::length(glm::dvec3(GetPosition()));
        radius -= static_cast<double>(mouseWheelDelta_) * mouseWheelSpeed * elapsedTime;
        radius = glm::clamp(radius, 0.01, 200.0);
        mouseWheelDelta_ = 0.0f;

        auto camOrient = glm::inverse(glm::dquat(GetOrientation()));
        glm::dquat camOrientStep = camArcball_.GetWorldRotation(elapsedTime, camOrient);
        camOrient = camOrientStep * camOrient;
        auto matOrient{ glm::mat3_cast(camOrient) };
        auto camPos = radius * glm::normalize(matOrient * baseCamPos_);

        SetCameraPosition(camPos);
        SetCameraOrientation(glm::inverse(camOrient));
    }

    /**
     *  Handles the mouse events for the camera.
     *  @param button the mouse button the event belongs to.
     *  @param action the mouse buttons action.
     *  @param sender the application to supply normalized screen coordinates.
     */
    bool ArcballCameraEnhanced::HandleMouse(int button, int action, float mouseWheelDelta, const ApplicationNodeBase* sender)
    {
        bool handled = camArcball_.HandleMouse(button, action, sender);

        if (mouseWheelDelta != 0) { //-V550
            mouseWheelDelta_ = mouseWheelDelta;
            handled = true;
        }

        return handled;
    }

    void ArcballCameraEnhanced::SetCameraPosition(const glm::vec3 & position)
    {
        CameraBase::SetCameraPosition(position - GetUserPosition());
    }

    void ArcballCameraEnhanced::FixView(const glm::mat4& view, const glm::mat4& proj)
    {
        isFixed_ = true;
        fixedView_ = view;
        fixedProj_ = proj;
        fixedCamPos_ = glm::vec3(glm::inverse(view) * glm::vec4(glm::vec3(0.0f), 1.0f));
    }

    glm::mat4 ArcballCameraEnhanced::GetViewPerspectiveExport() const
    {
        auto projection = glm::perspective(2.0f * glm::atan(1.0f / cameraHelper_.GetCentralPerspectiveMatrix()[1][1]), 1.0f, cameraHelper_.GetNearPlane(), cameraHelper_.GetFarPlane());
        auto userView = glm::lookAt(cameraHelper_.GetUserPosition(), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));


        //4. transform user back to original position
        glm::mat4 result = glm::translate(glm::mat4(1.0f), cameraHelper_.GetUserPosition());
        //3. apply view rotation
        result *= glm::mat4_cast(cameraHelper_.GetOrientation());
        //2. apply navigation translation
        result *= glm::translate(glm::mat4(1.0f), -cameraHelper_.GetPosition());
        //1. transform user to coordinate system origin
        result *= glm::translate(glm::mat4(1.0f), -cameraHelper_.GetUserPosition());

        return projection * userView * result;
    }

    glm::mat4 ArcballCameraEnhanced::GetViewMatrix() const
    {
        auto userView = glm::lookAt(cameraHelper_.GetUserPosition(), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));


        //4. transform user back to original position
        glm::mat4 result = glm::translate(glm::mat4(1.0f), cameraHelper_.GetUserPosition());
        //3. apply view rotation
        result *= glm::mat4_cast(cameraHelper_.GetOrientation());
        //2. apply navigation translation
        result *= glm::translate(glm::mat4(1.0f), -cameraHelper_.GetPosition());
        //1. transform user to coordinate system origin
        result *= glm::translate(glm::mat4(1.0f), -cameraHelper_.GetUserPosition());

        return userView * result;
    }

    glm::mat4 ArcballCameraEnhanced::GetViewProjMatrixEnh() const
    {
        if (isFixed_) return fixedProj_ * fixedView_;
        return GetViewProjMatrix();
    }

    glm::vec3 ArcballCameraEnhanced::GetPosition() const noexcept
    {
        if (isFixed_) return fixedCamPos_;
        return CameraBase::GetPosition() + GetUserPosition();
    }

}
