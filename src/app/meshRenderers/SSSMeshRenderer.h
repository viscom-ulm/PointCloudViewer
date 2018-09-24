/**
 * @file   SSSMeshRenderer.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.29
 *
 * @brief  Declaration of the mesh renderer for subsurface scattering.
 */

#pragma once

#include "app/meshRenderers/MeshRenderer.h"

namespace viscom {
    class GPUProgram;
}

namespace pcViewer {

    class SSSMeshRenderer : public MeshRenderer
    {
    public:
        SSSMeshRenderer(ApplicationNodeImplementation* appNode);

    protected:
        virtual void RenderGUIByType() override;
        virtual void ExportScreenPointCloudScreen(const FrameBuffer& fbo, const std::string& namePrefix, std::ostream& screenPoints) const override;

    private:
    };
}
