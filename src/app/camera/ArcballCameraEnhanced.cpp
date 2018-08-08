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
        radius = glm::clamp(radius, 0.01, 20.0);
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

    glm::vec3 ArcballCameraEnhanced::GetPosition() const noexcept
    {
        return CameraBase::GetPosition() + GetUserPosition();
    }

}