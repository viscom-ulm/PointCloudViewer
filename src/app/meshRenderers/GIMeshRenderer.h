/**
 * @file   GIMeshRenderer.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.29
 *
 * @brief  Declaration of the mesh renderer for ambient occlusion.
 */

#pragma once

#include "app/meshRenderers/MeshRenderer.h"

namespace viscom {
    class GPUProgram;
}

namespace pcViewer {

    class GIMeshRenderer : public MeshRenderer
    {
    public:
        GIMeshRenderer(ApplicationNodeImplementation* appNode);

    protected:
        virtual void RenderGUIByType() override;
        virtual void ExportScreenPointCloudScreen(const FrameBuffer& fbo, const std::string& namePrefix, std::ostream& screenPoints) const override;

    private:
        /** Holds the shader to calculate mesh. */
        std::shared_ptr<GPUProgram> meshExportShader_;
        /** Holds the uniform bindings for final rendering. */
        std::vector<gl::GLint> meshExportUniformLocations_;

    };
}
