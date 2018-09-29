/**
 * @file   SSSSSRenderer.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.23
 *
 * @brief  Declaration of the point cloud on mesh renderer base class.
 */

#pragma once

#include "app/Renderer.h"

namespace pcViewer {

    class SSSSSRenderer : public BaseRenderer
    {
    public:
        SSSSSRenderer(ApplicationNodeImplementation* appNode);

        virtual bool IsAvaialble() const override;

    protected:
        virtual void DrawPointCloudInternal(const FrameBuffer& fbo, const FrameBuffer& deferredFBO, bool batched) override;

        void DrawHBAO(const FrameBuffer& deferredFBO);

        virtual bool IsAmbientOcclustion() const { return false; }

        virtual void ExportScreenPointCloudMesh(std::ostream& meshPoints) const override;
        virtual void RenderGUIByType() override {};
        virtual void ExportScreenPointCloudScreen(const FrameBuffer& fbo, const std::string& namePrefix, std::ostream& screenPoints) const override;

    private:
        /** Holds the program for final rendering. */
        std::unique_ptr<FullscreenQuad> hbaoQuad_;
        /** Holds the uniform bindings for final rendering. */
        std::vector<gl::GLint> hbaoUniformLocations_;

        viscom::enh::TextureRAII noiseTexture_;
    };
}
