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

    class MeshContainer
    {
    public:
        MeshContainer(ApplicationNodeImplementation* appNode);
        ~MeshContainer();

        void SetMesh(const std::string& meshName, std::shared_ptr<Mesh> mesh, float theta, float phi);
        void DrawMeshDeferred(bool doDirectLighting) const;
        void DrawMeshDeferredAndExport(bool doDirectLighting);
        const std::vector<glm::vec3>& GetPositions() const;
        const std::vector<glm::vec3>& GetNormals() const;

        const std::vector<PointCloudPointSubsurface>& GetExportedPointCloud() const { return transformedPointCloud_; }
        const std::string& GetMeshName() const { return meshName_; }

    private:
        void DrawMeshDeferred(bool doDirectLighting, bool doExport) const;

        std::string meshName_;
        /** Holds the mesh to render. */
        std::shared_ptr<Mesh> mesh_;
        /** Holds the mesh renderable. */
        std::unique_ptr<enh::MeshRenderable> meshRenderable_;
        /** The meshes model matrix. */
        glm::mat4 meshModel_;

        std::vector<PointCloudPointSubsurface> transformedPointCloud_;

        /** Holds the program for deferred mesh rendering. */
        std::shared_ptr<GPUProgram> deferredProgram_;
        /** Holds the uniform bindings for deferred mesh rendering. */
        std::vector<gl::GLint> deferredUniformLocations_;

        std::unique_ptr<enh::ShaderBufferObject> pointCloudOutputSSBO_;

        ApplicationNodeImplementation* appNode_;

    };
}
