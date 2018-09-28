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
#include "core/gfx/mesh/SceneMeshNode.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace pcViewer {
    MeshContainer::MeshContainer(ApplicationNodeImplementation * appNode) :
        appNode_{ appNode }
    {
        deferredProgram_ = appNode->GetGPUProgramManager().GetResource("deferredMesh", std::vector<std::string>{ "deferredMesh.vert", "deferredMesh.frag" });
        deferredUniformLocations_ = deferredProgram_->GetUniformLocations({ "viewProjection", "camPos", "sigmat", "eta", "lightPos", "lightColor", "lightMultiplicator", "outputDirectLight", "doExport", "albedo", "depthBiasVP", "shadowMap" });

        pointCloudOutputSSBO_ = std::make_unique<enh::ShaderBufferObject>("pointCloudOutput", appNode_->GetSSBOBindingPoints(), true);
        appNode_->GetSSBOBindingPoints()->BindBufferBlock(deferredProgram_->getProgramId(), "pointCloudOutput");
        pointCloudOutputSSBO_->BindBuffer();

        FrameBufferDescriptor smDesc{ {FrameBufferTextureDescriptor{ static_cast<GLenum>(gl::GL_DEPTH_COMPONENT32F) }}, {} };
        shadowMap_ = std::make_unique<FrameBuffer>(1024, 1024, smDesc);
        gl::glBindTexture(gl::GL_TEXTURE_2D, shadowMap_->GetTextures()[0]);
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MAG_FILTER, gl::GL_LINEAR);
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MIN_FILTER, gl::GL_LINEAR);
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_S, gl::GL_CLAMP_TO_EDGE);
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_T, gl::GL_CLAMP_TO_EDGE);
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_COMPARE_MODE, gl::GL_COMPARE_REF_TO_TEXTURE);

        shadowMapProgram_ = appNode->GetGPUProgramManager().GetResource("shadowMap", std::vector<std::string>{ "comparison/shadowMap.vert", "comparison/shadowMap.frag" });;
        shadowMapUniformLocations_ = shadowMapProgram_->GetUniformLocations({ "viewProjection" });
    }

    MeshContainer::~MeshContainer() = default;

    void MeshContainer::SetMesh(const std::string& meshName, std::shared_ptr<Mesh> mesh, float theta, float phi, bool doRescale)
    {
        theta_ = theta;
        phi_ = phi;
        auto mm = glm::rotate(glm::mat4(1.0f), theta, glm::vec3(1.0f, 0.0f, 0.0f));
        mm = mm * glm::rotate(glm::mat4(1.0f), phi, glm::vec3(0.0f, 1.0f, 0.0f));

        SetMesh(meshName, mesh, mm, doRescale);
    }
    
    void MeshContainer::SetMesh(const std::string& meshName, std::shared_ptr<Mesh> mesh, const glm::mat4& meshModel, bool doRescale)
    {
        meshName_ = meshName;
        mesh_ = std::move(mesh);
        meshModel_ = meshModel;
        if (doRescale) {
            glm::mat4 modelMatrix(10.0f);
            meshModel_ = meshModel_ * modelMatrix;
        }

        if (!mesh_) {
            meshRenderable_ = nullptr;
            transformedPointCloud_.clear();
            return;
        }

        meshRenderable_ = enh::MeshRenderable::create<SimpleMeshVertex>(mesh_.get(), deferredProgram_.get());
        meshRenderableSM_ = enh::MeshRenderable::create<SimpleMeshVertex>(mesh_.get(), shadowMapProgram_.get());

        transformedPointCloud_.resize(mesh_->GetVertices().size());
        pointCloudOutputSSBO_->GetBuffer()->InitializeData(transformedPointCloud_);
    }

    void MeshContainer::DrawMeshDeferred(bool doDirectLighting) const
    {
        DrawMeshDeferred(doDirectLighting, false);
    }

    void MeshContainer::DrawShadowMap()
    {
        auto meshBB = mesh_->GetRootNode()->GetBoundingBox();
        meshBB.Transform(meshModel_);
        glm::vec3 center = 0.5f * (meshBB.GetMin() + meshBB.GetMax());
        glm::vec3 lightPos = const_cast<const ApplicationNodeImplementation*>(appNode_)->GetLightPosition();
        float radius = 0.5f * glm::length(meshBB.GetMin() - meshBB.GetMax());
        float distance = glm::length(center - lightPos);

        float fovHalf = glm::asin(radius / glm::length(center - lightPos));

        auto lightView = glm::lookAt(lightPos, center, glm::vec3{ 0.0f, 1.0f, 0.0f });
        auto lightProj = glm::perspectiveFov(2.0f * fovHalf, 1024.0f, 1024.0f, distance - radius, distance + radius);
        lightViewProj_ = lightProj * lightView;

        gl::glDisable(gl::GL_CULL_FACE);
        shadowMap_->DrawToFBO([this] {
            gl::glClear(gl::GL_DEPTH_BUFFER_BIT);
            gl::glUseProgram(shadowMapProgram_->getProgramId());
            gl::glUniformMatrix4fv(shadowMapUniformLocations_[0], 1, gl::GL_FALSE, glm::value_ptr(lightViewProj_));
            meshRenderableSM_->Draw(meshModel_);
        });
        gl::glDisable(gl::GL_POLYGON_OFFSET_FILL);

        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, 0);
        gl::glBindVertexArray(0);
        gl::glUseProgram(0);
        gl::glEnable(gl::GL_CULL_FACE);
    }

    const std::string& MeshContainer::GetMeshFilename() const
    {
        return mesh_->GetId();
    }

    void MeshContainer::DrawMeshDeferred(bool doDirectLighting, bool doExport) const
    {
        glm::mat4 biasMatrix(
            0.5, 0.0, 0.0, 0.0,
            0.0, 0.5, 0.0, 0.0,
            0.0, 0.0, 0.5, 0.0,
            0.5, 0.5, 0.5, 1.0
        );
        glm::mat4 lightBiasVP = biasMatrix * lightViewProj_;

        auto VP = doExport ? appNode_->GetCameraEnh().GetViewPerspectiveExport() : appNode_->GetCameraEnh().GetViewProjMatrixEnh();

        auto VPE = appNode_->GetCameraEnh().GetViewPerspectiveExport();
        auto VPNE = appNode_->GetCameraEnh().GetViewProjMatrixEnh();

        // VP = appNode_->GetCamera()->GetViewPerspectiveMatrix();

        gl::glDisable(gl::GL_CULL_FACE);
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
        gl::glUniform3fv(deferredUniformLocations_[9], 1, glm::value_ptr(const_cast<const ApplicationNodeImplementation*>(appNode_)->GetAlpha()));
        gl::glUniformMatrix4fv(deferredUniformLocations_[10], 1, gl::GL_FALSE, glm::value_ptr(lightBiasVP));

        gl::glActiveTexture(gl::GL_TEXTURE2);
        gl::glBindTexture(gl::GL_TEXTURE_2D, shadowMap_->GetTextures()[0]);
        gl::glUniform1i(deferredUniformLocations_[11], 2);
        meshRenderable_->Draw(meshModel_);

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
