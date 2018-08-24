/**
 * @file   MeshRenderer.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.24
 *
 * @brief  Declaration of the mesh renderer base class.
 */

#pragma once

#include "app/Renderer.h"

namespace pcViewer {

    class MeshRenderer : public BaseRenderer
    {
    public:
        MeshRenderer(PCType pcType, ApplicationNodeImplementation* appNode);

    protected:
        virtual void DrawPointCloudInternal(const FrameBuffer& fbo, const FrameBuffer& deferredFBO, bool batched) override;

    private:
    };

}
