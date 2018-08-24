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
    enum class RenderType : std::size_t;
    enum class PCType : std::size_t;
    class BaseRenderer;
    class BasePointCloudContainer;
    class MeshContainer;
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
        float GetPointSize() const { return pointSize_; }
        int GetMatteRenderType() const { return matteRenderType_; }
        int GetSubsurfaceRenderType() const { return subsurfaceRenderType_; }
        int GetCompositeType() const { return compositeType_; }
        void SetMatteRenderType(int rt) { matteRenderType_ = rt; }
        void SetSubsurfaceRenderType(int rt) { subsurfaceRenderType_ = rt; }
        void SetCompositeType(int ct) { compositeType_ = ct; }
        const std::vector<std::size_t>& GetDeferredDrawIndices() const { return deferredDrawIndices_; }
        const std::vector<std::size_t>& GetDistanceSumDrawIndices() const { return distanceSumDrawIndices_; }

        const FrameBufferDescriptor& GetDeferredFBODesc() const { return deferredFBODesc_; }

        enh::ArcballCameraEnhanced& GetCameraEnh() { return camera_; }

        void ClearExportScreenPointCloud(const FrameBuffer& deferredFBO);

        void ClearRadius() { boundingSphereRadius_ = 0.0f; }
        void AddToBoundingSphere(const glm::vec3& v) { boundingSphereRadius_ = glm::max(boundingSphereRadius_, glm::length(v)); }

    protected:
        // pcViewer::BaseRenderer* GetRenderer() const { return currentRenderer_; }

        float& GetDistancePower() { return distancePower_; }
        float& GetPointSize() { return pointSize_; }

        void SelectRenderers(pcViewer::PCType type);
        void RendererSelectionGUI();

        void RenderersLoadPointCloud(const std::string& pointCloudName, const std::string& pointCloud);
        void RenderersSetMesh(std::shared_ptr<Mesh> mesh, float theta, float phi);
        void RenderersSetEnvironmentMap(std::shared_ptr<Texture> envMap);
        void CurrentRendererDrawPointCloud(const FrameBuffer& fbo, const FrameBuffer& deferredFBO, bool batched) const;

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
        float pointSize_ = 1.0f;
        enh::ArcballCameraEnhanced camera_;

        pcViewer::RenderType baseRenderType_;

        std::unique_ptr<pcViewer::MeshContainer> mesh_;
        std::array<std::unique_ptr<pcViewer::BasePointCloudContainer>, 3> pointClouds_;
        pcViewer::BasePointCloudContainer* currentPointCloud_ = nullptr;
        std::array<std::unique_ptr<pcViewer::BaseRenderer>, 3>* currentRenderers_ = nullptr;
        std::array<std::array<std::unique_ptr<pcViewer::BaseRenderer>, 3>, 3> renderers_;
        // glm::vec3 camPos_;
        // glm::vec3 camRot_;

        // std::vector<FrameBuffer> sceneFBOs_;
        // std::unique_ptr<enh::DepthOfField> dof_;
        // std::unique_ptr<enh::BloomEffect> bloom_;
        // std::unique_ptr<enh::FilmicTMOperator> tm_;

        int matteRenderType_ = 0;
        int subsurfaceRenderType_ = 0;
        int compositeType_ = 0;

        FrameBufferDescriptor deferredFBODesc_;
        std::vector<FrameBuffer> deferredFBOs_;

        std::vector<std::size_t> deferredDrawIndices_;
        std::vector<std::size_t> distanceSumDrawIndices_;
        std::vector<std::size_t> deferredPositionIndices_;
        std::vector<std::size_t> deferredNonPosIndices_;
    };
}
