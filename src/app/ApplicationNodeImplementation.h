/**
 * @file   ApplicationNodeImplementation.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.30
 *
 * @brief  Declaration of the application node implementation common for master and slave nodes.
 */

#pragma once

#include "enh/ApplicationNodeBase.h"
#include "core/camera/ArcballCamera.h"

namespace viscom::enh {
    class DepthOfField;
    class FilmicTMOperator;
    class BloomEffect;
}

namespace viscom {

    enum class PCType {
        AO,
        MATTE,
        SUBSURFACE
    };

    struct PointCloudPointAO {
        glm::vec3 position_;
        glm::vec3 normal_;
        float ao_;
    };

    struct PointCloudPointMatte {

    };

    struct PointCloudPointSubsurface {

    };

    class MeshRenderable;

    class ApplicationNodeImplementation : public enh::ApplicationNodeBase
    {
    public:
        explicit ApplicationNodeImplementation(ApplicationNodeInternal* appNode);
        ApplicationNodeImplementation(const ApplicationNodeImplementation&) = delete;
        ApplicationNodeImplementation(ApplicationNodeImplementation&&) = delete;
        ApplicationNodeImplementation& operator=(const ApplicationNodeImplementation&) = delete;
        ApplicationNodeImplementation& operator=(ApplicationNodeImplementation&&) = delete;
        virtual ~ApplicationNodeImplementation() override;

        virtual void InitOpenGL() override;
        virtual void UpdateFrame(double currentTime, double elapsedTime) override;
        virtual void ClearBuffer(FrameBuffer& fbo) override;
        virtual void DrawFrame(FrameBuffer& fbo) override;
        virtual void CleanUp() override;

        virtual bool KeyboardCallback(int key, int scancode, int action, int mods) override;
        virtual bool MouseButtonCallback(int button, int action) override;
        virtual bool MousePosCallback(double x, double y) override;
        virtual bool MouseScrollCallback(double xoffset, double yoffset) override;


    protected:
        enh::DepthOfField* GetDOF() { return dof_.get(); }
        enh::FilmicTMOperator* GetToneMapping() { return tm_.get(); }
        enh::BloomEffect* GetBloom() { return bloom_.get(); }

    private:
        ArcballCamera camera_;
        glm::vec3 camPos_;
        glm::vec3 camRot_;

        std::vector<FrameBuffer> sceneFBOs_;
        std::unique_ptr<enh::DepthOfField> dof_;
        std::unique_ptr<enh::BloomEffect> bloom_;
        std::unique_ptr<enh::FilmicTMOperator> tm_;
    };
}
