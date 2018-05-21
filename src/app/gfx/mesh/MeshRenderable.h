/**
 * @file   MeshRenderable.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.12.15
 *
 * @brief  Declaration of a mesh renderable class.
 */

#pragma once

#include "enh/gfx/gl/OpenGLRAIIWrapper.h"
#include "core/gfx/mesh/Mesh.h"
#include "core/gfx/GPUProgram.h"

namespace viscom::enh {

    /**
     *  This class renders a mesh with a specific shader. The shader is assumed to have fixed uniform names:
     *  modelMatrix: the model matrix.
     *  normalMatrix: the normal matrix.
     *  diffuseTexture: the diffuse texture.
     *  bumpTexture: a bump map.
     *  bumpMultiplier: the bump multiplier.
     *
     *  NOT ALL UNIFORM LOCATIONS NEED TO BE USED!
     *
     *  The attribute names are determined by the vertex structure.
     */
    class MeshRenderable
    {
    public:
        template<class VTX> static std::unique_ptr<MeshRenderable> create(const Mesh* renderMesh, GPUProgram* program);

        virtual ~MeshRenderable();
        MeshRenderable(const MeshRenderable&) = delete;
        MeshRenderable& operator=(const MeshRenderable&) = delete;
        MeshRenderable(MeshRenderable&&) noexcept;
        MeshRenderable& operator=(MeshRenderable&&) noexcept;

        void Draw(const glm::mat4& modelMatrix, bool overrideBump = false) const;

        template<class VTX> void NotifyRecompiledShader(const GPUProgram* program);

    protected:
        MeshRenderable(const Mesh* renderMesh, GLuint vBuffer, GPUProgram* program);

        void DrawNode(const glm::mat4& modelMatrix, const SceneMeshNode* node, bool overrideBump = false) const;

    private:
        /** Holds the mesh to render. */
        const Mesh* mesh_;
        /** Holds the vertex buffer. */
        BufferRAII vbo_;
        /** Holds the vertex array object. */
        VertexArrayRAII vao_;
        /** Holds the rendering GPU program for drawing. */
        GPUProgram* drawProgram_;
        /** Holds the standard uniform bindings. */
        std::vector<gl::GLint> uniformLocations_;

        void DrawSubMesh(const glm::mat4& modelMatrix, const SubMesh* subMesh, bool overrideBump = false) const;
    };

    template <class VTX>
    std::unique_ptr<MeshRenderable> MeshRenderable::create(const Mesh* renderMesh, GPUProgram* program)
    {
        std::unique_ptr<MeshRenderable> result{ new MeshRenderable(renderMesh, VTX::CreateVertexBuffer(renderMesh), program) };
        result->NotifyRecompiledShader<VTX>(program);
        return std::move(result);
    }

    template<class VTX> void MeshRenderable::NotifyRecompiledShader(const GPUProgram* program)
    {
        gl::glBindVertexArray(vao_);
        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, vbo_);
        gl::glBindBuffer(gl::GL_ELEMENT_ARRAY_BUFFER, mesh_->GetIndexBuffer());
        VTX::SetVertexAttributes(program);
        gl::glBindVertexArray(0);
        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, 0);
        gl::glBindBuffer(gl::GL_ELEMENT_ARRAY_BUFFER, 0);

        uniformLocations_ = program->GetUniformLocations({ "modelMatrix", "normalMatrix", "diffuseTexture", "bumpTexture", "bumpMultiplier" });
    }
}
