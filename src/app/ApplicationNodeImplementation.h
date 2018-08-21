/**
 * @file   ApplicationNodeImplementation.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.30
 *
 * @brief  Declaration of the application node implementation common for coordinator and worker nodes.
 */

#pragma once

#include "enh/ApplicationNodeBase.h"
#include "camera/ArcballCameraEnhanced.h"

namespace viscom::enh {
    class DepthOfField;
    class FilmicTMOperator;
    class BloomEffect;
    class MeshRenderable;
}

namespace pcViewer {
    enum class PCType;
    class BaseRenderer;
}

namespace viscom {

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

        virtual bool MouseButtonCallback(int button, int action) override;
        virtual bool MousePosCallback(double x, double y) override;
        virtual bool MouseScrollCallback(double xoffset, double yoffset) override;
        virtual bool KeyboardCallback(int key, int scancode, int action, int mods) override;

        float GetBoundingSphereRadius() const { return boundingSphereRadius_; }
        float GetDistancePower() const { return distancePower_; }
        int GetMatteRenderType() const { return matteRenderType_; }
        int GetSubsurfaceRenderType() const { return subsurfaceRenderType_; }
        void SetMatteRenderType(int rt) { matteRenderType_ = rt; }
        void SetSubsurfaceRenderType(int rt) { subsurfaceRenderType_ = rt; }
        const std::vector<std::size_t>& GetDeferredDrawIndices() const { return deferredDrawIndices_; }
        const std::vector<std::size_t>& GetDistanceSumDrawIndices() const { return distanceSumDrawIndices_; }

        const FrameBufferDescriptor& GetDeferredFBODesc() const { return deferredFBODesc_; }

        enh::ArcballCameraEnhanced& GetCameraEnh() { return camera_; }

        void ClearExportScreenPointCloud(const FrameBuffer& deferredFBO);

        void ClearRadius() { boundingSphereRadius_ = 0.0f; }
        void AddToBoundingSphere(const glm::vec3& v) { boundingSphereRadius_ = glm::max(boundingSphereRadius_, glm::length(v)); }

    protected:
        pcViewer::BaseRenderer* GetRenderer() const { return currentRenderer_; }

        float& GetDistancePower() { return distancePower_; }

        void SelectPointCloudRenderer(pcViewer::PCType type) { currentRenderer_ = pointRenderers_[static_cast<std::size_t>(type)].get(); }

        // enh::DepthOfField* GetDOF() { return dof_.get(); }
        // enh::FilmicTMOperator* GetToneMapping() { return tm_.get(); }
        // enh::BloomEffect* GetBloom() { return bloom_.get(); }

        // bool HasMesh() const { return static_cast<bool>(mesh_); }
        // bool IsRenderModel() const { return renderModel_; }
        // bool IsModelDirect() const { return modelDirect_; }
        // void SetRenderModel(bool rm) { renderModel_ = rm; }

    private:
        float boundingSphereRadius_ = 0.0f;
        float distancePower_ = 2.0f;
        enh::ArcballCameraEnhanced camera_;

        pcViewer::BaseRenderer* currentRenderer_ = nullptr;
        std::array<std::unique_ptr<pcViewer::BaseRenderer>, 3> pointRenderers_;
        std::array<std::unique_ptr<pcViewer::BaseRenderer>, 3> meshRenderers_;
        // glm::vec3 camPos_;
        // glm::vec3 camRot_;

        // std::vector<FrameBuffer> sceneFBOs_;
        // std::unique_ptr<enh::DepthOfField> dof_;
        // std::unique_ptr<enh::BloomEffect> bloom_;
        // std::unique_ptr<enh::FilmicTMOperator> tm_;

        int matteRenderType_ = 0;
        int subsurfaceRenderType_ = 0;

        FrameBufferDescriptor deferredFBODesc_;
        std::vector<FrameBuffer> deferredFBOs_;

        std::vector<std::size_t> deferredDrawIndices_;
        std::vector<std::size_t> distanceSumDrawIndices_;
        std::vector<std::size_t> deferredPositionIndices_;
        std::vector<std::size_t> deferredNonPosIndices_;
    };
}
