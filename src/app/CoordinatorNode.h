/**
 * @file   CoordinatorNode.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.25
 *
 * @brief  Declaration of the ApplicationNodeImplementation for the coordinator node.
 */

#pragma once

#include "app/ApplicationNodeImplementation.h"

namespace viscom {

    class CoordinatorNode final : public ApplicationNodeImplementation
    {
    public:
        explicit CoordinatorNode(ApplicationNodeInternal* appNode);
        virtual ~CoordinatorNode() override;

        virtual void InitOpenGL() override;
        virtual void Draw2D(FrameBuffer& fbo) override;
        virtual bool KeyboardCallback(int key, int scancode, int action, int mods) override;

        std::vector<std::string> GetDirectoryContent(const std::string& dir, bool pcFilesOnly) const;

        static void SetSingleFile(const std::string& file) { singleFile_ = file; }

    private:
        void RenderFolderHeadless(const std::string& folder, pcViewer::RenderType renderType);
        void RenderFileHeadless(const std::string& pointCloud, pcViewer::RenderType renderType);
        // void RenderFileHeadlessToFBO(const std::string& pointCloud, bool loadModel);
        void LoadTextFile(const std::string& filename, bool loadModel);
        void LoadPointCloud(const std::string& pointCloud, const std::vector<std::string>& splitFilename, bool loadModel);
        void LoadMesh(pcViewer::PCType type, const std::string& meshname);
        void LoadScreenTexture(const std::string& filename, const std::vector<std::string>& splitFilename);

        /* Holds the input directory. */
        std::string inputDir_;
        std::vector<std::string> inputDirEntries_;
        /** Holds the selected file type. */
        // PCType pcType_;
        // /** The point clouds. */
        // std::vector<PointCloudPointAO> pcAO_;
        // std::vector<PointCloudPointMatte> pcMatte_;
        // std::vector<PointCloudPointSubsurface> pcSubsurface_;
        // 
        // /** Holds the shader program for drawing ambient occlusion. */
        // std::shared_ptr<GPUProgram> aoProgram_;
        // /** Holds the location of the MVP matrix. */
        // GLint aoMVPLoc_ = -1;
        // 
        // /** Holds the vertex buffer for the point could. */
        // GLuint vboPointCloud_ = 0;
        // /** Holds the vertex array object for the point cloud. */
        // GLuint vaoPointCloud_ = 0;

        bool inputFileSelected_ = false;
        bool inputBatchMode_ = false;

        static std::string singleFile_;

        std::unique_ptr<FrameBuffer> headlessFBO_;
        std::unique_ptr<FrameBuffer> deferredHeadlessFBO_;
        std::unique_ptr<FrameBuffer> deferredExportHeadlessFBO_;
    };
}
