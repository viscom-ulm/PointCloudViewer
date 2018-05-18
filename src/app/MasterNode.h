/**
 * @file   MasterNode.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.25
 *
 * @brief  Declaration of the ApplicationNodeImplementation for the master node.
 */

#pragma once

#include "../app/ApplicationNodeImplementation.h"

// #include "core/open_gl.h"

namespace viscom {

    class MasterNode final : public ApplicationNodeImplementation
    {
    public:
        explicit MasterNode(ApplicationNodeInternal* appNode);
        virtual ~MasterNode() override;

        virtual void InitOpenGL() override;
        virtual void DrawFrame(FrameBuffer& fbo) override;
        virtual void Draw2D(FrameBuffer& fbo) override;
        virtual bool KeyboardCallback(int key, int scancode, int action, int mods) override;
        virtual void CleanUp() override;

        std::vector<std::string> GetDirectoryContent(const std::string& dir) const;

        static void SetSingleFile(const std::string& file) { singleFile_ = file; }
    private:
        void LoadPointCloud(const std::string& pointCloud);
        void LoadPointCloudAO(const std::string& pointCloud);
        void LoadPointCloudMatte(const std::string& pointCloud);
        void LoadPointCloudSubsurface(const std::string& pointCloud);

        /* Holds the input directory. */
        std::string inputDir_, selectedFilename_;
        /** Holds the selected file type. */
        PCType pcType_;
        /** The point clouds. */
        std::vector<PointCloudPointAO> pcAO_;
        std::vector<PointCloudPointMatte> pcMatte_;
        std::vector<PointCloudPointSubsurface> pcSubsurface_;

        /** Holds the shader program for drawing ambient occlusion. */
        std::shared_ptr<GPUProgram> aoProgram_;
        /** Holds the location of the MVP matrix. */
        GLint aoMVPLoc_ = -1;

        /** Holds the vertex buffer for the point could. */
        GLuint vboPointCloud_ = 0;
        /** Holds the vertex array object for the point cloud. */
        GLuint vaoPointCloud_ = 0;

        bool inputFileSelected_ = false;

        static std::string singleFile_;
    };
}
