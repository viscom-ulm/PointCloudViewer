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
        virtual double DoPerformanceMeasureInternal(const FrameBuffer& fbo, const FrameBuffer& deferredFBO, bool batched) override;

        void DrawHBAO(const FrameBuffer& deferredFBO);

        virtual bool IsAmbientOcclustion() const { return false; }

        // virtual void ExportScreenPointCloudMesh(std::ostream& meshPoints) const override;
        virtual void RenderGUIByType() override;
        // virtual void ExportScreenPointCloudScreen(const FrameBuffer& fbo, const std::string& namePrefix, std::ostream& screenPoints) const override;

    private:
        struct DiffusionParams {
            float sigmat_ = 0.0f;
            float rho_ = 0.0f;
            float sigmaa_ = 0.0f;
            float sigmas_ = 0.0f;

            float D_ = 0.0f, A_ = 0.0f;
            float sigma_tr_ = 0.0f;
            float zb_ = 0.0f;
            float cPhi_ = 0.0f, cE_ = 0.0f;
        };

        void RecalculateKernel(bool force);

        float PBD(float r, const DiffusionParams& p) const;
        float GetDipoleValue(float r, float l, const DiffusionParams& p) const;
        float ComputeSingleScattering(float r, const DiffusionParams& p) const;
        float ComputeSingleScatteringSample(float r, float ti, const DiffusionParams& p) const;

        /** Holds the program for final rendering. */
        std::unique_ptr<FullscreenQuad> s5Quad_;
        /** Holds the uniform bindings for final rendering. */
        std::vector<gl::GLint> s5UniformLocations_;

        float maxOffsetMm_ = 1.0f;
        glm::vec3 sigmat_, alpha_;

        float eta_ = 0.0f;

        std::array<DiffusionParams, 3> diffParams_;

        viscom::enh::TextureRAII kernelTexture_;
    };
}
