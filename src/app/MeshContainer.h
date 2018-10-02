/**
 * @file   MeshContainer.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.24
 *
 * @brief  Declaration of the mesh container.
 */

#pragma once

#include <memory>
#include <vector>
#include <glm/mat4x4.hpp>
#include "enh/gfx/gl/OpenGLRAIIWrapper.h"
#include "app/PointCloudContainer.h"

namespace viscom {
    class FrameBuffer;
    class Mesh;
    class ApplicationNodeImplementation;
    class GPUProgram;
}

namespace viscom::enh {
    class MeshRenderable;
    class ShaderBufferObject;
}

namespace pcViewer {

    using namespace viscom;

    struct MeshContainerEntryInfo {
        /** Holds the mesh to render. */
        std::shared_ptr<Mesh> mesh_;

        glm::vec3 albedo_, sigmat_;
        float eta_;
    };

    struct MeshContainerEntry {
        MeshContainerEntryInfo info_;
        /** The meshes model matrix. */
        glm::mat4 meshModel_;
        /** Holds the mesh renderable. */
        std::unique_ptr<enh::MeshRenderable> meshRenderable_;
        std::unique_ptr<enh::MeshRenderable> meshRenderableSM_;

        // float theta_ = 0.0f, phi_ = 0.0f, rescale_ = 1.0f;
    };

    class MeshContainer
    {
    public:
        MeshContainer(ApplicationNodeImplementation* appNode);
        ~MeshContainer();

        operator bool() const { return !meshEntries_.empty(); }
        void ResetContainer(const std::string& meshName);
        void AddMesh(const MeshContainerEntryInfo& meshInfo, float theta, float phi, bool doRescale);
        void AddMesh(const MeshContainerEntryInfo& meshInfo, const glm::mat4& meshModel, bool doRescale);
        void DrawMeshDeferred(bool doDirectLighting) const;
        void DrawShadowMap();
        // void DrawMeshDeferredAndExport(bool doDirectLighting);
        // const std::vector<glm::vec3>& GetPositions() const;
        // const std::vector<glm::vec3>& GetNormals() const;
        // float GetTheta() const { return theta_; }
        // float GetPhi() const { return phi_; }
        // float GetRescale() const { return rescale_; }
        // const glm::mat4& GetModelMatrix() const { return meshModel_; }

        // const std::vector<PointCloudPointSubsurface>& GetExportedPointCloud() const { return transformedPointCloud_; }
        std::size_t GetNumMeshes() const { return meshEntries_.size(); }
        const MeshContainerEntry& GetMesh(std::size_t i) const { return meshEntries_[i]; }
        MeshContainerEntry& GetMesh(std::size_t i) { return meshEntries_[i]; }
        const std::string& GetMeshName() const { return meshName_; }
        const std::string& GetMeshFilename(std::size_t i) const;

    private:

        void DrawMeshDeferred(bool doDirectLighting, bool doExport) const;

        std::string meshName_;

        std::vector<MeshContainerEntry> meshEntries_;

        // std::vector<PointCloudPointSubsurface> transformedPointCloud_;

        /** Holds the program for deferred mesh rendering. */
        std::shared_ptr<GPUProgram> deferredProgram_;
        /** Holds the uniform bindings for deferred mesh rendering. */
        std::vector<gl::GLint> deferredUniformLocations_;

        // std::unique_ptr<enh::ShaderBufferObject> pointCloudOutputSSBO_;

        std::unique_ptr<FrameBuffer> shadowMap_;
        std::shared_ptr<GPUProgram> shadowMapProgram_;
        std::vector<gl::GLint> shadowMapUniformLocations_;
        glm::mat4 lightViewProj_;

        ApplicationNodeImplementation* appNode_;

    };
}
