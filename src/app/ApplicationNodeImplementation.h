/**
 * @file   ApplicationNodeImplementation.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.30
 *
 * @brief  Declaration of the application node implementation common for coordinator and worker nodes.
 */

#pragma once

#include "enh/ApplicationNodeBase.h"
#include "camera/ArcballCameraEnhanced.h"

namespace viscom::enh {
    class DepthOfField;
    class FilmicTMOperator;
    class BloomEffect;
    class MeshRenderable;
    class EnvironmentMapRenderer;
}

namespace viscom {

    enum class PCType {
        AO,
        MATTE,
        SUBSURFACE
    };

    struct PointCloudPointAO {
        glm::vec3 position_;
        glm::vec3 normal_;
        float ao_;
    };

    struct PointCloudPointMatte {
        glm::vec3 position_;
        glm::vec3 normal_;
        glm::vec3 albedo_;
        glm::vec3 directIllumination_;
        glm::vec3 globalIllumination_;
    };

    struct PointCloudPointSubsurface {
        glm::vec3 position_;
        glm::vec3 normal_;
        glm::vec3 albedo_;
        glm::vec3 sigma_tp_;
        float eta_;
        glm::vec3 directIllumination_;
        glm::vec3 globalIllumination_;
    };

    class ApplicationNodeImplementation : public enh::ApplicationNodeBase
    {
    public:
        explicit ApplicationNodeImplementation(ApplicationNodeInternal* appNode);
        ApplicationNodeImplementation(const ApplicationNodeImplementation&) = delete;
        ApplicationNodeImplementation(ApplicationNodeImplementation&&) = delete;
        ApplicationNodeImplementation& operator=(const ApplicationNodeImplementation&) = delete;
        ApplicationNodeImplementation& operator=(ApplicationNodeImplementation&&) = delete;
        virtual ~ApplicationNodeImplementation() override;

        virtual void InitOpenGL() override;
        virtual void UpdateFrame(double currentTime, double elapsedTime) override;
        virtual void ClearBuffer(FrameBuffer& fbo) override;
        virtual void DrawFrame(FrameBuffer& fbo) override;
        virtual void CleanUp() override;

        virtual bool MouseButtonCallback(int button, int action) override;
        virtual bool MousePosCallback(double x, double y) override;
        virtual bool MouseScrollCallback(double xoffset, double yoffset) override;
        virtual bool KeyboardCallback(int key, int scancode, int action, int mods) override;

    protected:
        void LoadPointCloudGPUAO(std::vector<PointCloudPointAO>& pointCloud);
        void LoadPointCloudGPUMatte(std::vector<PointCloudPointMatte>& pointCloud);
        void LoadPointCloudGPUSubsurface(std::vector<PointCloudPointSubsurface>& pointCloud);

        void DrawPointCloud(const FrameBuffer& fbo, const FrameBuffer& deferredFBO, bool batched);

        PCType GetPointCloudType() const { return pcType_; }
        int GetMatteRenderType() const { return matteRenderType_; }
        void SetMatteRenderType(int rt) { matteRenderType_ = rt; }
        int GetSubsurfaceRenderType() const { return subsurfaceRenderType_; }
        void SetSubsurfaceRenderType(int rt) { subsurfaceRenderType_ = rt; }

        void SetMesh(std::shared_ptr<Mesh> mesh, float theta, float phi);
        void SetEnvironmentMap(std::shared_ptr<Texture> envMap);

        void ClearRadius() { boundingSphereRadius_ = 0.0f; }
        void AddToBoundingSphere(const glm::vec3& v) { boundingSphereRadius_ = glm::max(boundingSphereRadius_, glm::length(v)); }
        float& GetDistancePower() { return distancePower_; }

        // enh::DepthOfField* GetDOF() { return dof_.get(); }
        // enh::FilmicTMOperator* GetToneMapping() { return tm_.get(); }
        // enh::BloomEffect* GetBloom() { return bloom_.get(); }

        bool HasMesh() const { return static_cast<bool>(mesh_); }
        bool GetRenderModel() const { return renderModel_; }
        void SetRenderModel(bool rm) { renderModel_ = rm; }

    private:
        void DrawPointCloudPoints(bool batched);
        void DrawMeshDeferred();
        void DrawPointCloudDistanceSum(const FrameBuffer& deferredFBO);
        void DrawPointCloudOnMesh(const FrameBuffer& deferredFBO);

        float boundingSphereRadius_ = 0.0f;
        float distancePower_ = 1.0f;
        enh::ArcballCameraEnhanced camera_;
        // glm::vec3 camPos_;
        // glm::vec3 camRot_;

        // std::vector<FrameBuffer> sceneFBOs_;
        // std::unique_ptr<enh::DepthOfField> dof_;
        // std::unique_ptr<enh::BloomEffect> bloom_;
        // std::unique_ptr<enh::FilmicTMOperator> tm_;

        /** Holds wether to render the model. */
        bool renderModel_;

        /** Holds the selected file type. */
        PCType pcType_;
        /** The point clouds. */
        std::vector<PointCloudPointAO> pcAO_;
        std::vector<PointCloudPointMatte> pcMatte_;
        std::vector<PointCloudPointSubsurface> pcSubsurface_;
        int matteRenderType_ = 0;
        int subsurfaceRenderType_ = 0;

        /** Holds the shader program for drawing ambient occlusion. */
        std::shared_ptr<GPUProgram> aoProgram_;
        /** Holds the location of the MVP matrix. */
        gl::GLint aoMVPLoc_ = -1;
        gl::GLint aoBBRLoc_ = -1;

        /** Holds the shader program for drawing matte results. */
        std::shared_ptr<GPUProgram> matteProgram_;
        /** Holds the location of the MVP matrix. */
        gl::GLint matteMVPLoc_ = -1;
        /** Holds the location of render type. */
        gl::GLint matteRenderTypeLoc_ = -1;
        gl::GLint matteBBRLoc_ = -1;

        /** Holds the shader program for drawing subsurface results. */
        std::shared_ptr<GPUProgram> subsurfaceProgram_;
        /** Holds the location of the MVP matrix. */
        gl::GLint subsurfaceMVPLoc_ = -1;
        /** Holds the location of render type. */
        gl::GLint subsurfaceRenderTypeLoc_ = -1;
        gl::GLint subsurfaceBBRLoc_ = -1;

        /** Holds the vertex buffer for the point could. */
        gl::GLuint vboPointCloud_ = 0;
        /** Holds the vertex array object for the point cloud. */
        gl::GLuint vaoPointCloud_ = 0;


        std::vector<FrameBuffer> deferredFBOs_;

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

        /** Holds the program for summation of distances. */
        std::shared_ptr<GPUProgram> distanceSumAOProgram_;
        /** Holds the uniform bindings for summation of distances. */
        std::vector<gl::GLint> distanceSumAOUniformLocations_;

        /** Holds the program for summation of distances. */
        std::shared_ptr<GPUProgram> distanceSumMatteProgram_;
        /** Holds the uniform bindings for summation of distances. */
        std::vector<gl::GLint> distanceSumMatteUniformLocations_;

        /** Holds the program for summation of distances. */
        std::shared_ptr<GPUProgram> distanceSumSubsurfaceProgram_;
        /** Holds the uniform bindings for summation of distances. */
        std::vector<gl::GLint> distanceSumSubsurfaceUniformLocations_;

        /** Holds the program for final rendering. */
        std::unique_ptr<FullscreenQuad> finalQuad_;
        /** Holds the uniform bindings for final rendering. */
        std::vector<gl::GLint> finalUniformLocations_;

        std::vector<std::size_t> deferredDrawIndices_;
        std::vector<std::size_t> distanceSumDrawIndices_;
    };
}
