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

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace pcViewer {
    MeshContainer::MeshContainer(ApplicationNodeImplementation * appNode) :
        appNode_{ appNode }
    {
        deferredProgram_ = appNode->GetGPUProgramManager().GetResource("deferredMesh", std::vector<std::string>{ "deferredMesh.vert", "deferredMesh.frag" });
        deferredUniformLocations_ = deferredProgram_->GetUniformLocations({ "viewProjection" });
    }

    void MeshContainer::SetMesh(std::shared_ptr<Mesh> mesh, float theta, float phi)
    {
        mesh_ = std::move(mesh);
        if (mesh_) meshRenderable_ = enh::MeshRenderable::create<SimpleMeshVertex>(mesh_.get(), deferredProgram_.get());

        meshModel_ = glm::rotate(glm::mat4(1.0f), theta, glm::vec3(1.0f, 0.0f, 0.0f));
        meshModel_ = meshModel_ * glm::rotate(glm::mat4(1.0f), phi, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    void MeshContainer::DrawMeshDeferred() const
    {
        auto VP = appNode_->GetCamera()->GetViewPerspectiveMatrix();

        gl::glDisable(gl::GL_CULL_FACE);
        glm::mat4 modelMatrix(10.f);
        modelMatrix[3][3] = 1.f;
        gl::glUseProgram(deferredProgram_->getProgramId());
        gl::glUniformMatrix4fv(deferredUniformLocations_[0], 1, gl::GL_FALSE, glm::value_ptr(VP));
        gl::glUniform3fv(deferredProgram_->getUniformLocation("camPos"), 1, glm::value_ptr(appNode_->GetCameraEnh().GetPosition()));
        meshRenderable_->Draw(meshModel_ * modelMatrix);

        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, 0);
        gl::glBindVertexArray(0);
        gl::glUseProgram(0);

        gl::glEnable(gl::GL_CULL_FACE);
    }
}
