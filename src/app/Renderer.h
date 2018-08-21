/**
 * @file   Renderer.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.17
 *
 * @brief  Declaration of the renderer base class.
 */

#pragma once

#include <vector>
#include <memory>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <ostream>
#include "core/gfx/Texture.h"
#include "enh/gfx/gl/OpenGLRAIIWrapper.h"
#include "app/gfx/mesh/MeshRenderable.h"
#include "app/Vertices.h"

namespace viscom {
    class FrameBuffer;
    class Mesh;
    class Texture;
    class CameraHelper;
    class ApplicationNodeImplementation;
    class FullscreenQuad;
    class FrameBuffer;
}

namespace viscom::enh {
    class MeshRenderable;
    class EnvironmentMapRenderer;
    class ArcballCameraEnhanced;
}

namespace pcViewer {

    enum class PCType {
        AO,
        MATTE,
        SUBSURFACE
    };

    struct PointCloudPointAO {
        glm::vec3 position_;
        glm::vec3 normal_;
        float ao_ = 0.0f;
    };

    static std::ostream& operator <<(std::ostream& out, const PointCloudPointAO& pcp) {
        return out << pcp.position_.x << ',' << pcp.position_.y << ',' << pcp.position_.z << ',' << pcp.normal_.x << ',' << pcp.normal_.y << ',' << pcp.normal_.z;
    }

    struct PointCloudPointMatte {
        glm::vec3 position_;
        glm::vec3 normal_;
        glm::vec3 albedo_;
        glm::vec3 directIllumination_;
        glm::vec3 globalIllumination_;
    };

    static std::ostream& operator <<(std::ostream& out, const PointCloudPointMatte& pcp) {
        return out << pcp.position_.x << ',' << pcp.position_.y << ',' << pcp.position_.z << ',' << pcp.normal_.x << ',' << pcp.normal_.y << ',' << pcp.normal_.z
            << ',' << pcp.albedo_.r << ',' << pcp.albedo_.g << ',' << pcp.albedo_.b << ',' << pcp.directIllumination_.r << ',' << pcp.directIllumination_.g << ',' << pcp.directIllumination_.b;
    }

    struct PointCloudPointSubsurface {
        glm::vec3 position_;
        glm::vec3 normal_;
        glm::vec3 albedo_;
        glm::vec3 sigma_tp_;
        float eta_ = 1.33f;
        glm::vec3 directIllumination_;
        glm::vec3 globalIllumination_;
    };

    static std::ostream& operator <<(std::ostream& out, const PointCloudPointSubsurface& pcp) {
        return out << pcp.position_.x << ',' << pcp.position_.y << ',' << pcp.position_.z << ',' << pcp.normal_.x << ',' << pcp.normal_.y << ',' << pcp.normal_.z
            << ',' << pcp.albedo_.r << ',' << pcp.albedo_.g << ',' << pcp.albedo_.b << ',' << pcp.sigma_tp_.r << ',' << pcp.sigma_tp_.g << ',' << pcp.sigma_tp_.b
            << ',' << pcp.eta_ << ',' << pcp.directIllumination_.r << ',' << pcp.directIllumination_.g << ',' << pcp.directIllumination_.b;
    }

    using namespace viscom;

    class BaseRenderer
    {
    public:
        BaseRenderer(PCType pcType, ApplicationNodeImplementation* appNode);

        void LoadPointCloud(const std::string& pointCloud);
        void DrawPointCloud(const FrameBuffer& fbo, const FrameBuffer& deferredFBO, bool batched);
        void ExportScreenPointCloud(std::ostream& screenPoints, std::ostream& meshPoints);

        void RenderGUI();

        PCType GetPointCloudType() const { return pcType_; }

        bool HasMesh() const { return static_cast<bool>(mesh_); }
        void SetMesh(std::shared_ptr<Mesh> mesh, float theta, float phi);
        void SetEnvironmentMap(std::shared_ptr<Texture> envMap);
        void SetRenderModel(bool rm) { renderModel_ = rm; }

        // bool IsRenderModel() const { return renderModel_; }

    protected:
        const enh::BufferRAII& GetVBO() const { return vboPointCloud_; }
        const enh::VertexArrayRAII& GetVAO() const { return vaoPointCloud_; }

        virtual void StorePointInPointCloud(const std::vector<std::string>& pointData) = 0;
        virtual void LoadPointCloudGPU() = 0;
        void PreparePointCloudVAOInternal();
        virtual void PreparePointCloudVAO() = 0;

        void DrawPointCloudPointsInternal(bool batched);
        virtual void DrawPointCloudPoints(const glm::mat4& MVP, const glm::vec3& camPos, bool batched) = 0;

        void DrawMeshPointCloudPoints(const FrameBuffer& fbo, const FrameBuffer& deferredFBO);
        void DrawMeshDeferred();
        void DrawPointCloudDistanceSumInternal(const FrameBuffer& deferredFBO);
        virtual void DrawPointCloudDistanceSum(const glm::mat4& MVP, const FrameBuffer& deferredFBO) = 0;
        void DrawPointCloudOnMesh(const FrameBuffer& deferredFBO);

        virtual void RenderGUIByType() = 0;

        virtual void ExportScreenPointCloudScreen(const FrameBuffer& fbo, std::ostream& screenPoints) const = 0;
        virtual void ExportScreenPointCloudMesh(std::ostream& meshPoints) const = 0;

        float GetBoundingSphereRadius() const;
        float GetDistancePower() const;
        ApplicationNodeImplementation* GetApp() { return appNode_; }

        const FrameBuffer& GetDeferredExportFBO() const { return *deferredExportFBO_.get(); }

        static enh::BufferRAII CopyTextureToPixelBuffer3(const FrameBuffer& deferredFBO, std::size_t tex);
        static enh::BufferRAII CopyTextureToPixelBuffer4(const FrameBuffer& deferredFBO, std::size_t tex);
        static void CopyPBOToVector3(gl::GLuint pbo, std::vector<glm::vec3>& content);
        static void CopyPBOToVector4(gl::GLuint pbo, std::vector<glm::vec4>& content);

        virtual void ClearPointCloud() = 0;

    private:
        PCType pcType_;
        /** Holds weather to render the model. */
        bool renderModel_ = false;

        /** Holds the mesh to render. */
        std::shared_ptr<Mesh> mesh_;
        /** Holds the mesh renderable. */
        std::unique_ptr<enh::MeshRenderable> meshRenderable_;
        /** The meshes model matrix. */
        glm::mat4 meshModel_;
        /** Holds the background environment map. */
        std::shared_ptr<Texture> envMap_;
        std::unique_ptr<enh::EnvironmentMapRenderer> envMapRenderer_;

        /** Holds the program for deferred mesh rendering. */
        std::shared_ptr<GPUProgram> deferredProgram_;
        /** Holds the uniform bindings for deferred mesh rendering. */
        std::vector<gl::GLint> deferredUniformLocations_;

        /** Holds the program for final rendering. */
        std::unique_ptr<FullscreenQuad> finalQuad_;
        /** Holds the uniform bindings for final rendering. */
        std::vector<gl::GLint> finalUniformLocations_;

        /** Holds the vertex buffer for the point could. */
        enh::BufferRAII vboPointCloud_;
        /** Holds the vertex array object for the point cloud. */
        enh::VertexArrayRAII vaoPointCloud_;

        /** Deferred FBO for export. */
        std::unique_ptr<FrameBuffer> deferredExportFBO_;

        ApplicationNodeImplementation* appNode_;
    };


    template<class PointCloudPoint> class TBaseRenderer : public BaseRenderer
    {
    public:
        TBaseRenderer(PCType pcType, ApplicationNodeImplementation* appNode) : BaseRenderer{ pcType, appNode } {}

        virtual void ExportScreenPointCloudMesh(std::ostream& meshPoints) const override;
        const std::vector<PointCloudPoint>& GetPointCloud() const { return pointCloud_; }

    protected:
        virtual void LoadPointCloudGPU() override;
        virtual void ClearPointCloud() override { pointCloud_.clear(); }
        PointCloudPoint& EditNextPoint() { return pointCloud_.emplace_back(); }

    private:
        std::vector<PointCloudPoint> pointCloud_;
    };

    template<class PointCloudPoint>
    void pcViewer::TBaseRenderer<PointCloudPoint>::ExportScreenPointCloudMesh(std::ostream& meshPoints) const
    {
        std::copy(pointCloud_.begin(), pointCloud_.end(), std::ostream_iterator<PointCloudPoint>(meshPoints, "\n"));
    }

    template<class PointCloudPoint>
    inline void TBaseRenderer<PointCloudPoint>::LoadPointCloudGPU()
    {
        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, GetVBO());
        gl::glBufferData(gl::GL_ARRAY_BUFFER, pointCloud_.size() * sizeof(PointCloudPoint), pointCloud_.data(), gl::GL_STATIC_DRAW);

        PreparePointCloudVAOInternal();
    }
}
