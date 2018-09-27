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
#include "enh/gfx/gl/OpenGLRAIIWrapper.h"

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

        virtual void UpdateFrame(double currentTime, double elapsedTime) override;
        virtual void ClearBuffer(FrameBuffer& fbo) override;
        virtual void DrawFrame(FrameBuffer& fbo) override;

        virtual bool MouseButtonCallback(int button, int action) override;
        virtual bool MousePosCallback(double x, double y) override;
        virtual bool MouseScrollCallback(double xoffset, double yoffset) override;
        virtual bool KeyboardCallback(int key, int scancode, int action, int mods) override;

        float GetBoundingSphereRadius() const { return boundingSphereRadius_; }
        float GetDistancePower() const { return distancePower_; }
        float GetPointSize() const { return pointSize_; }
        const glm::vec3& GetLightPosition() const { return lightPos_; }
        const glm::vec3& GetLightColor() const { return lightColor_; }
        float GetLightMultiplicator() const { return lightMultiplicator_; }
        const glm::vec3& GetSigmaT() const { return sigmaT_; }
        const glm::vec3& GetAlpha() const { return alpha_; }
        float GetEta() const { return eta_; }
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
        const FrameBuffer& GetDeferredExportFBO() const { return *deferredExportFBO_; }
        const FWConfiguration* GetConfig() { return &ApplicationNodeBase::GetConfig(); }

    protected:
        void InitOpenGLInternal();
        void CleanUpInternal();
        // pcViewer::BaseRenderer* GetRenderer() const { return currentRenderer_; }

        float& GetDistancePower() { return distancePower_; }
        float& GetPointSize() { return pointSize_; }
        glm::vec3& GetLightPosition() { return lightPos_; }
        glm::vec3& GetLightColor() { return lightColor_; }
        float& GetLightMultiplicator() { return lightMultiplicator_; }
        glm::vec3& GetAlpha() { return alpha_; }
        glm::vec3& GetSigmaT() { return sigmaT_; }
        float& GetEta() { return eta_; }

        void SelectRenderers(pcViewer::PCType type);
        void UnselectCurrentRenderer() { currentRenderers_ = nullptr; };
        void RendererSelectionGUI();
        void SetBaseRenderType(pcViewer::RenderType type);
        void SetScreenRenderingComposition(int comp) { screenRenderingComposition_ = comp; }

        void RenderersLoadPointCloud(const std::string& pointCloudName, const std::string& pointCloud, const glm::mat4& modelMatrix);
        void RenderersSetMesh(const std::string& meshName, std::shared_ptr<Mesh> mesh, float theta, float phi, bool doRescale);
        void RenderersSetMesh(const std::string& meshName, std::shared_ptr<Mesh> mesh, const glm::mat4& modelMatrix, bool doRescale);
        void RenderersSetEnvironmentMap(std::shared_ptr<Texture> envMap);
        void CurrentRendererDrawPointCloud(const FrameBuffer& fbo, const FrameBuffer& deferredFBO, bool batched) const;
        void DrawLoadedScreen(const FrameBuffer& fbo) const;
        void UpdateBaseRendererType();
        void StartRenderScreen(enh::TexuturesRAII<5> textures);
        pcViewer::BaseRenderer* GetCurrentRenderer() const { return (*currentRenderers_)[static_cast<std::size_t>(baseRenderType_)].get(); }

        // enh::DepthOfField* GetDOF() { return dof_.get(); }
        // enh::FilmicTMOperator* GetToneMapping() { return tm_.get(); }
        // enh::BloomEffect* GetBloom() { return bloom_.get(); }

        // bool HasMesh() const { return static_cast<bool>(mesh_); }
        // bool IsRenderModel() const { return renderModel_; }
        // bool IsModelDirect() const { return modelDirect_; }
        // void SetRenderModel(bool rm) { renderModel_ = rm; }

        glm::uvec2 viewportSize_;

    private:
        float boundingSphereRadius_ = 0.0f;
        float distancePower_ = 2.0f;
        float pointSize_ = 1.0f;
        glm::vec3 alpha_ = glm::vec3{ 1.0f}, sigmaT_ = glm::vec3{ 1.0f };
        float eta_ = 1.0f;
        glm::vec3 lightPos_ = glm::vec3{ 10.0f, 0.0f, 0.0f };
        glm::vec3 lightColor_ = glm::vec3{ 1.0f };
        float lightMultiplicator_ = 1.0f;
        pcViewer::PCType currentPointCloudType_;
        enh::ArcballCameraEnhanced camera_;
        int screenRenderingComposition_;

        std::unique_ptr<enh::TexuturesRAII<5>> screenTexturesPtr_;

        pcViewer::RenderType baseRenderType_;

        std::unique_ptr<pcViewer::MeshContainer> mesh_;
        std::array<std::unique_ptr<pcViewer::BasePointCloudContainer>, 3> pointClouds_;
        pcViewer::BasePointCloudContainer* currentPointCloud_ = nullptr;
        std::array<std::unique_ptr<pcViewer::BaseRenderer>, 4>* currentRenderers_ = nullptr;
        std::array<std::array<std::unique_ptr<pcViewer::BaseRenderer>, 4>, 3> renderers_;
        // glm::vec3 camPos_;
        // glm::vec3 camRot_;

        std::unique_ptr<FullscreenQuad> screenRenderQuad_;
        std::vector<gl::GLint> screenRenderUniformLocations_;
        // std::vector<FrameBuffer> sceneFBOs_;
        // std::unique_ptr<enh::DepthOfField> dof_;
        // std::unique_ptr<enh::BloomEffect> bloom_;
        // std::unique_ptr<enh::FilmicTMOperator> tm_;

        int matteRenderType_ = 0;
        int subsurfaceRenderType_ = 0;
        int compositeType_ = 0;

        FrameBufferDescriptor deferredFBODesc_;
        std::vector<FrameBuffer> deferredFBOs_;
        std::unique_ptr<FrameBuffer> deferredExportFBO_;

        std::vector<std::size_t> deferredDrawIndices_;
        std::vector<std::size_t> distanceSumDrawIndices_;
        std::vector<std::size_t> deferredPositionIndices_;
        std::vector<std::size_t> deferredNonPosIndices_;
    };
}
