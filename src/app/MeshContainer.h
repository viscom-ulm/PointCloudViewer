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

namespace viscom {
    class FrameBuffer;
    class Mesh;
    class ApplicationNodeImplementation;
    class GPUProgram;
}

namespace viscom::enh {
    class MeshRenderable;
}

namespace pcViewer {

    using namespace viscom;

    class MeshContainer
    {
    public:
        MeshContainer(ApplicationNodeImplementation* appNode);
        ~MeshContainer() = default;

        void SetMesh(std::shared_ptr<Mesh> mesh, float theta, float phi);
        void DrawMeshDeferred() const;

    private:
        /** Holds the mesh to render. */
        std::shared_ptr<Mesh> mesh_;
        /** Holds the mesh renderable. */
        std::unique_ptr<enh::MeshRenderable> meshRenderable_;
        /** The meshes model matrix. */
        glm::mat4 meshModel_;

        /** Holds the program for deferred mesh rendering. */
        std::shared_ptr<GPUProgram> deferredProgram_;
        /** Holds the uniform bindings for deferred mesh rendering. */
        std::vector<gl::GLint> deferredUniformLocations_;

        ApplicationNodeImplementation* appNode_;

    };
}
