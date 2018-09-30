/**
 * @file   ArcballCameraEnhanced.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2017.06.07
 *
 * @brief  Declaration of an arcball camera.
 */

#pragma once

#include "core/CameraHelper.h"
#include "core/camera/Arcball.h"
#include "core/camera/CameraBase.h"
#include <glm/gtc/quaternion.hpp>

namespace viscom::enh {

    /**
    * Represents a camera rotating around the origin.
    */
    class ArcballCameraEnhanced final : public viscom::CameraBase
    {
    public:
        ArcballCameraEnhanced(const glm::vec3& camPos, viscom::CameraHelper& cameraHelper) noexcept;
        virtual ~ArcballCameraEnhanced() override;

        virtual bool HandleMouse(int button, int action, float mouseWheelDelta, const viscom::ApplicationNodeBase* sender) override;
        virtual void UpdateCamera(double elapsedTime, const viscom::ApplicationNodeBase* sender) override;

        virtual glm::vec3 GetPosition() const noexcept override;

        virtual void SetCameraPosition(const glm::vec3& position) override;

        void FixView(const glm::mat4& view, const glm::mat4& proj);
        void ReleaseView() { isFixed_ = false; }
        glm::mat4 GetViewPerspectiveExport() const;
        glm::mat4 GetViewMatrix() const;
        glm::mat4 GetViewProjMatrixEnh() const;
        glm::mat4 GetProjMatrixEnh(const glm::mat4& nonFixed) const;
        bool IsFixed() const { return isFixed_; }

    private:
        /** Holds the current camera position. */
        glm::vec3 baseCamPos_;
        /** Holds the mouse wheel delta. */
        float mouseWheelDelta_;
        /** Holds the arc-ball used for camera rotation. */
        Arcball camArcball_;

        bool isFixed_ = false;
        glm::mat4 fixedView_, fixedProj_;
        glm::vec3 fixedCamPos_;
    };
}
