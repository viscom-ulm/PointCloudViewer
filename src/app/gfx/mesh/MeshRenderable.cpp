/**
 * @file   MeshRenderable.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.12.15
 *
 * @brief  Implementation of a mesh renderable class.
 */


#include "core/gfx/mesh/SceneMeshNode.h"
#include "core/gfx/mesh/SubMesh.h"
#include "core/gfx/Material.h"
#include "core/gfx/Texture.h"
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "MeshRenderable.h"

namespace viscom::enh {

    /**
     * Constructor.
     * @param renderMesh the Mesh to use for rendering.
     * @param program the program used for rendering.
     */
    MeshRenderable::MeshRenderable(const Mesh* renderMesh, GLuint vBuffer, GPUProgram* program) :
        mesh_(renderMesh),
        vbo_(vBuffer),
        drawProgram_(program)
    {
    }

    /**
     * Destructor.
     */
    MeshRenderable::~MeshRenderable() = default;

    /**
     * Move constructor.
     * @param orig the original object
     */
    MeshRenderable::MeshRenderable(MeshRenderable&& orig) noexcept :
        mesh_(orig.mesh_),
        vbo_(std::move(orig.vbo_)),
        vao_(std::move(orig.vao_)),
        drawProgram_(orig.drawProgram_),
        uniformLocations_(std::move(orig.uniformLocations_))
    {
        orig.mesh_ = nullptr;
        orig.drawProgram_ = nullptr;
    }

    /**
     * Move assignment operator.
     * @param orig the original object
     */
    MeshRenderable& MeshRenderable::operator=(MeshRenderable&& orig) noexcept
    {
        if (this != &orig) {
            this->~MeshRenderable();
            mesh_ = orig.mesh_;
            vbo_ = std::move(orig.vbo_);
            vao_ = std::move(orig.vao_);
            drawProgram_ = orig.drawProgram_;
            uniformLocations_ = std::move(orig.uniformLocations_);
            orig.mesh_ = nullptr;
            orig.drawProgram_ = nullptr;
        }
        return *this;
    }

    void MeshRenderable::Draw(const glm::mat4& modelMatrix, bool overrideBump) const
    {
        gl::glUseProgram(drawProgram_->getProgramId());
        gl::glBindVertexArray(vao_);
        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, vbo_);
        DrawNode(modelMatrix, mesh_->GetRootNode(), overrideBump);
        gl::glBindVertexArray(0);
        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, 0);
    }

    void MeshRenderable::DrawNode(const glm::mat4& modelMatrix, const SceneMeshNode* node, bool overrideBump) const
    {
        auto localMatrix = node->GetLocalTransform() * modelMatrix;
        for (std::size_t i = 0; i < node->GetNumberOfSubMeshes(); ++i) {
            const auto* submesh = &mesh_->GetSubMeshes()[node->GetSubMeshID(i)];
            DrawSubMesh(localMatrix, submesh, overrideBump);
        }
        for (std::size_t i = 0; i < node->GetNumberOfNodes(); ++i) DrawNode(localMatrix, node->GetChild(i), overrideBump);
    }

    void MeshRenderable::DrawSubMesh(const glm::mat4& modelMatrix, const SubMesh* subMesh, bool overrideBump) const
    {
        gl::glUniformMatrix4fv(uniformLocations_[0], 1, gl::GL_FALSE, glm::value_ptr(modelMatrix));
        gl::glUniformMatrix3fv(uniformLocations_[1], 1, gl::GL_FALSE, glm::value_ptr(glm::inverseTranspose(glm::mat3(modelMatrix))));

        auto mat = mesh_->GetMaterial(subMesh->GetMaterialIndex());
        auto matTex = mesh_->GetMaterialTexture(subMesh->GetMaterialIndex());
        if (matTex->diffuseTex && uniformLocations_.size() > 2) {
            gl::glActiveTexture(gl::GL_TEXTURE0);
            gl::glBindTexture(gl::GL_TEXTURE_2D, matTex->diffuseTex->getTextureId());
            gl::glUniform1i(uniformLocations_[2], 0);
        }
        if (matTex->bumpTex && uniformLocations_.size() > 3) {
            gl::glActiveTexture(gl::GL_TEXTURE1);
            gl::glBindTexture(gl::GL_TEXTURE_2D, matTex->bumpTex->getTextureId());
            gl::glUniform1i(uniformLocations_[3], 1);
            if (!overrideBump) gl::glUniform1f(uniformLocations_[4], mat->bumpMultiplier);
        }

        gl::glDrawElements(gl::GL_TRIANGLES, subMesh->GetNumberOfIndices(), gl::GL_UNSIGNED_INT,
            (static_cast<char*> (nullptr)) + (static_cast<std::size_t>(subMesh->GetIndexOffset()) * sizeof(unsigned int)));
    }
}
