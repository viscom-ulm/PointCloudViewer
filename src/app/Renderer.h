/**
 * @file   Renderer.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.17
 *
 * @brief  Declaration of the renderer base class.
 */

#pragma once

#include "enh/gfx/gl/OpenGLRAIIWrapper.h"

#include <vector>
#include <memory>
#include "core/gfx/Texture.h"

namespace viscom {
    class FrameBuffer;
    class Texture;
    class CameraHelper;
    class ApplicationNodeImplementation;
    class FullscreenQuad;
}

namespace viscom::enh {
    class EnvironmentMapRenderer;
    class ArcballCameraEnhanced;
}

namespace pcViewer {

    enum class PCType : std::size_t;
    class BasePointCloudContainer;
    class MeshContainer;

    enum class RenderType : std::size_t {
        POINTCLOUD = 0,
        PC_ON_MESH = 1,
        MESH = 2,
        SCREEN = 3
    };

    using namespace viscom;

    class BaseRenderer
    {
    public:
        BaseRenderer(PCType pcType, RenderType renderType, const std::string& rendererName, ApplicationNodeImplementation* appNode);
        virtual ~BaseRenderer() = default;

        void SetPointCloud(BasePointCloudContainer* pointCloud);
        void SetMesh(MeshContainer* mesh) { mesh_ = mesh; }
        void DrawPointCloud(const FrameBuffer& fbo, const FrameBuffer& deferredFBO, bool batched);
        void ExportPBRT(const std::string& pbrtOutName, const glm::uvec2& imgSize, std::ostream& pbrt);
        void ExportScreenPointCloud(const FrameBuffer& deferredExportFBO, const std::string& namePrefix, std::ostream& info,
            std::ostream& screenPoints, std::ostream& meshPoints);

        void RenderGUI();

        // PCType GetPointCloudType() const { return pcType_; }

        void SetEnvironmentMap(std::shared_ptr<Texture> envMap);
        // void SetRenderModel(bool rm) { renderModel_ = rm; }

        virtual bool IsAvaialble() const = 0;
        const std::string& GetRendererName() const { return rendererName_; }
        RenderType GetRendererType() const { return rendererType_; }
        PCType GetPCType() const { return pcType_; }

    protected:
        virtual void DrawPointCloudInternal(const FrameBuffer& fbo, const FrameBuffer& deferredFBO, bool batched) = 0;

        virtual void RenderGUIByType() = 0;

        virtual void ExportScreenPointCloudScreen(const FrameBuffer& fbo, const std::string& namePrefix, std::ostream& screenPoints) const = 0;
        virtual void ExportScreenPointCloudMesh(std::ostream& meshPoints) const = 0;

        float GetBoundingSphereRadius() const;
        float GetDistancePower() const;
        float GetPointSize() const;
        const glm::vec3& GetLightPosition() const;
        const glm::vec3& GetLightColor() const;
        float GetLightMultiplicator() const;
        const glm::vec3& GetAlpha() const;
        const glm::vec3& GetSigmaT() const;
        float GetEta() const;
        ApplicationNodeImplementation* GetApp() { return appNode_; }

        // const FrameBuffer& GetDeferredExportFBO() const { return *deferredExportFBO_; }

        static enh::BufferRAII CopyTextureToPixelBuffer3(const FrameBuffer& deferredFBO, std::size_t tex);
        static enh::BufferRAII CopyTextureToPixelBuffer4(const FrameBuffer& deferredFBO, std::size_t tex);
        static void CopyPBOToVector3(gl::GLuint pbo, std::vector<glm::vec3>& content);
        static void CopyPBOToVector4(gl::GLuint pbo, std::vector<glm::vec4>& content);

        MeshContainer* GetMesh() const { return mesh_; }
        const BasePointCloudContainer* GetPointCloud() const { return pointCloud_; }
        std::size_t GetPointCloudSize() const;
        bool DoExportPointCloud() const { return exportPointCloud; }

    private:
        PCType pcType_;
        RenderType rendererType_;
        std::string rendererName_;
        /** Holds the point cloud container. */
        BasePointCloudContainer* pointCloud_ = nullptr;
        /** Holds the mesh container. */
        MeshContainer* mesh_ = nullptr;

        bool exportPointCloud = false;

        /** Holds the background environment map. */
        std::shared_ptr<Texture> envMap_;
        std::unique_ptr<enh::EnvironmentMapRenderer> envMapRenderer_;

        ApplicationNodeImplementation* appNode_;
    };
}
