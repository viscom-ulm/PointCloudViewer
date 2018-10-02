/**
 * @file   AOMeshRenderer.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.27
 *
 * @brief  Declaration of the mesh renderer for ambient occlusion.
 */

#pragma once

#include "app/meshRenderers/MeshRenderer.h"

namespace viscom {
    class GPUProgram;
}

namespace pcViewer {

    class AOMeshRenderer : public MeshRenderer
    {
    public:
        AOMeshRenderer(ApplicationNodeImplementation* appNode);

    protected:
        virtual void RenderGUIByType() override;
        // virtual void ExportScreenPointCloudScreen(const FrameBuffer& fbo, const std::string& namePrefix, std::ostream& screenPoints) const override;

    private:
    };
}
