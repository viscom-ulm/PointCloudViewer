/**
 * @file   ApplicationNodeImplementation.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.30
 *
 * @brief  Declaration of the application node implementation common for master and slave nodes.
 */

#pragma once

#include "enh/ApplicationNodeBase.h"
#include "core/camera/ArcballCamera.h"

namespace viscom::enh {
    class DepthOfField;
    class FilmicTMOperator;
    class BloomEffect;
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

    class MeshRenderable;

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


    protected:
        void LoadPointCloudGPUAO(std::vector<PointCloudPointAO>& pointCloud);
        void LoadPointCloudGPUMatte(std::vector<PointCloudPointMatte>& pointCloud);
        void LoadPointCloudGPUSubsurface(std::vector<PointCloudPointSubsurface>& pointCloud);

        void DrawPointCloud();

        PCType GetPointCloudType() const { return pcType_; }
        int GetMatteRenderType() const { return matteRenderType_; }
        void SetMatteRenderType(int rt) { matteRenderType_ = rt; }
        int GetSubsurfaceRenderType() const { return subsurfaceRenderType_; }
        void SetSubsurfaceRenderType(int rt) { subsurfaceRenderType_ = rt; }

        // enh::DepthOfField* GetDOF() { return dof_.get(); }
        // enh::FilmicTMOperator* GetToneMapping() { return tm_.get(); }
        // enh::BloomEffect* GetBloom() { return bloom_.get(); }

    private:
        ArcballCamera camera_;
        // glm::vec3 camPos_;
        // glm::vec3 camRot_;

        // std::vector<FrameBuffer> sceneFBOs_;
        // std::unique_ptr<enh::DepthOfField> dof_;
        // std::unique_ptr<enh::BloomEffect> bloom_;
        // std::unique_ptr<enh::FilmicTMOperator> tm_;

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
        GLint aoMVPLoc_ = -1;

        /** Holds the shader program for drawing matte results. */
        std::shared_ptr<GPUProgram> matteProgram_;
        /** Holds the location of the MVP matrix. */
        GLint matteMVPLoc_ = -1;
        /** Holds the location of render type. */
        GLint matteRenderTypeLoc_ = -1;

        /** Holds the shader program for drawing subsurface results. */
        std::shared_ptr<GPUProgram> subsurfaceProgram_;
        /** Holds the location of the MVP matrix. */
        GLint subsurfaceMVPLoc_ = -1;
        /** Holds the location of render type. */
        GLint subsurfaceRenderTypeLoc_ = -1;

        /** Holds the vertex buffer for the point could. */
        GLuint vboPointCloud_ = 0;
        /** Holds the vertex array object for the point cloud. */
        GLuint vaoPointCloud_ = 0;
    };
}
