/**
 * @file   MeshContainer.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.24
 *
 * @brief  Implementation of the mesh container.
 */

#include "MeshContainer.h"
#include "app/ApplicationNodeImplementation.h"
#include "app/gfx/mesh/MeshRenderable.h"
#include "app/Vertices.h"
#include "enh/gfx/gl/ShaderBufferObject.h"
#include "enh/gfx/gl/GLBuffer.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace pcViewer {
    MeshContainer::MeshContainer(ApplicationNodeImplementation * appNode) :
        appNode_{ appNode }
    {
        deferredProgram_ = appNode->GetGPUProgramManager().GetResource("deferredMesh", std::vector<std::string>{ "deferredMesh.vert", "deferredMesh.frag" });
        deferredUniformLocations_ = deferredProgram_->GetUniformLocations({ "viewProjection", "camPos", "sigmat", "eta", "lightPos", "lightColor", "lightMultiplicator", "outputDirectLight", "doExport" });

        pointCloudOutputSSBO_ = std::make_unique<enh::ShaderBufferObject>("pointCloudOutput", appNode_->GetSSBOBindingPoints(), true);
        appNode_->GetSSBOBindingPoints()->BindBufferBlock(deferredProgram_->getProgramId(), "pointCloudOutput");
        pointCloudOutputSSBO_->BindBuffer();
    }

    MeshContainer::~MeshContainer() = default;

    void MeshContainer::SetMesh(const std::string& meshName, std::shared_ptr<Mesh> mesh, float theta, float phi)
    {
        meshName_ = meshName;
        mesh_ = std::move(mesh);
        if (mesh_) meshRenderable_ = enh::MeshRenderable::create<SimpleMeshVertex>(mesh_.get(), deferredProgram_.get());

        meshModel_ = glm::rotate(glm::mat4(1.0f), theta, glm::vec3(1.0f, 0.0f, 0.0f));
        meshModel_ = meshModel_ * glm::rotate(glm::mat4(1.0f), phi, glm::vec3(0.0f, 1.0f, 0.0f));

        transformedPointCloud_.resize(mesh_->GetVertices().size());
        pointCloudOutputSSBO_->GetBuffer()->InitializeData(transformedPointCloud_);
    }

    void MeshContainer::DrawMeshDeferred(bool doDirectLighting) const
    {
        DrawMeshDeferred(doDirectLighting, false);
    }

    void MeshContainer::DrawMeshDeferred(bool doDirectLighting, bool doExport) const
    {
        auto VP = appNode_->GetCamera()->GetViewPerspectiveMatrix();

        gl::glDisable(gl::GL_CULL_FACE);
        glm::mat4 modelMatrix(10.f);
        modelMatrix[3][3] = 1.f;
        gl::glUseProgram(deferredProgram_->getProgramId());
        gl::glUniformMatrix4fv(deferredUniformLocations_[0], 1, gl::GL_FALSE, glm::value_ptr(VP));
        gl::glUniform3fv(deferredUniformLocations_[1], 1, glm::value_ptr(appNode_->GetCameraEnh().GetPosition()));
        gl::glUniform3fv(deferredUniformLocations_[2], 1, glm::value_ptr(const_cast<const ApplicationNodeImplementation*>(appNode_)->GetSigmaT()));
        gl::glUniform1f(deferredUniformLocations_[3], const_cast<const ApplicationNodeImplementation*>(appNode_)->GetEta());
        gl::glUniform3fv(deferredUniformLocations_[4], 1, glm::value_ptr(const_cast<const ApplicationNodeImplementation*>(appNode_)->GetLightPosition()));
        gl::glUniform3fv(deferredUniformLocations_[5], 1, glm::value_ptr(const_cast<const ApplicationNodeImplementation*>(appNode_)->GetLightColor()));
        gl::glUniform1f(deferredUniformLocations_[6], const_cast<const ApplicationNodeImplementation*>(appNode_)->GetLightMultiplicator());
        gl::glUniform1i(deferredUniformLocations_[7], doDirectLighting ? 1 : 0);
        gl::glUniform1i(deferredUniformLocations_[8], doExport ? 1 : 0);
        meshRenderable_->Draw(meshModel_ * modelMatrix);

        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, 0);
        gl::glBindVertexArray(0);
        gl::glUseProgram(0);

        gl::glEnable(gl::GL_CULL_FACE);
    }

    void MeshContainer::DrawMeshDeferredAndExport(bool doDirectLighting)
    {
        DrawMeshDeferred(doDirectLighting, true);

        gl::glMemoryBarrier(gl::GL_ALL_BARRIER_BITS);
        pointCloudOutputSSBO_->GetBuffer()->DownloadData(transformedPointCloud_);
    }

    const std::vector<glm::vec3>& MeshContainer::GetPositions() const
    {
        return mesh_->GetVertices();
    }

    const std::vector<glm::vec3>& MeshContainer::GetNormals() const
    {
        return mesh_->GetNormals();
    }
}
