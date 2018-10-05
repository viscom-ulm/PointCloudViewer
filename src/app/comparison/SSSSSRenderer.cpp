/**
 * @file   SSSSSRenderer.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.23
 *
 * @brief  Implementation of the base point cloud on mesh renderer class.
 */

#include "SSSSSRenderer.h"
#include <glm/gtc/type_ptr.hpp>
#include "core/CameraHelper.h"
#include "core/gfx/FrameBuffer.h"
#include "enh/gfx/env/EnvironmentMapRenderer.h"
#include "app/camera/ArcballCameraEnhanced.h"
#include "app/ApplicationNodeImplementation.h"
#include "app/PointCloudContainer.h"
#include "app/MeshContainer.h"
#include <imgui.h>
#include <fstream>

#include <glm/gtc/random.hpp>
#include <random>

#include "app/comparison/improved_diffusion.h"

namespace pcViewer {

    constexpr int BEAM_SAMPLES = 50;

    // BxDF Utility Functions
    float FrDielectric(float cosThetaI, float etaI, float etaT)
    {
        cosThetaI = std::min(static_cast<float>(-1.0), std::max(cosThetaI, static_cast<float>(1.0)));
        // Potentially swap indices of refraction
        bool entering = cosThetaI > 0.f;
        if (!entering) {
            std::swap(etaI, etaT);
            cosThetaI = std::abs(cosThetaI);
        }

        // Compute _cosThetaT_ using Snell's law
        float sinThetaI = std::sqrt(std::max((float)0, 1 - cosThetaI * cosThetaI));
        float sinThetaT = etaI / etaT * sinThetaI;

        // Handle total internal reflection
        if (sinThetaT >= 1) return 1;
        float cosThetaT = std::sqrt(std::max((float)0, 1 - sinThetaT * sinThetaT));
        float Rparl = ((etaT * cosThetaI) - (etaI * cosThetaT)) / ((etaT * cosThetaI) + (etaI * cosThetaT));
        float Rperp = ((etaI * cosThetaI) - (etaT * cosThetaT)) / ((etaI * cosThetaI) + (etaT * cosThetaT));
        return (Rparl * Rparl + Rperp * Rperp) / 2;
    }

    float SSSSSRenderer::ComputeSingleScatteringSample(float r, float ti, const DiffusionParams& p) const
    {
        float cosSign = -1.0;

        // Determine length $d$ of connecting segment and $\cos\theta_\roman{o}$
        float d = std::sqrt(r * r + ti * ti);
        float cosThetaO = ti / d;

        assert(cosThetaO > 0.0);

        // Ft(thetai) = Ft(0) = 1
        float phase = 0.5f * glm::one_over_two_pi<float>();;
        float Ft = (1 - FrDielectric(cosSign * cosThetaO, 1, eta_));

        // Add contribution of single scattering at depth $t$
        auto result = std::exp(-p.sigmat_ * d) / (d * d) * phase * Ft * std::abs(cosThetaO);

        assert(result >= 0);
        // multiply with Q.
        // sigma_t_ * std::exp(-sigma_t_ * ti) canceled out by PDF.
        return p.rho_ * result;
    }

    float SSSSSRenderer::ComputeSingleScattering(float r, const DiffusionParams& p) const
    {
        float result = 0;
        float tCrit = r * std::sqrt(eta_ * eta_ - 1);

        for (int i = 0; i < BEAM_SAMPLES; ++i) {
            float u = (static_cast<float>(i) + .5f) / static_cast<float>(BEAM_SAMPLES);
            float l = -std::log((1 - u) * std::exp(-p.sigmat_ * tCrit)) / p.sigmat_;

            //Real xi2 = sampleFactor * xi;
            //Real l = -std::log(1.0 - xi2) / sigma_t_;
            //if constexpr (SSDIR == SingleScatteringDirection::REFLECTANCE) l = tCrit + l;

            assert(l > 0);

            // Evaluate single scattering integrand and add to _Ess_

            result += ComputeSingleScatteringSample(r, l, p);
        }
        // return 2 * pi<Real>() * r * result / BEAM_SAMPLES;
        float factor = std::exp(-p.sigmat_ * tCrit);
        return (result * factor) / static_cast<float>(BEAM_SAMPLES);
    }

    float SSSSSRenderer::GetDipoleValue(float r, float l, const DiffusionParams& p) const
    {
        if (l < 0) return -1.0;

        // Evaluate dipole integrand $E_{\roman{d}}$ at $\depthreal$ and add to _Ed_
        float zri = l;
        float zvi = l + 2.0f * p.zb_;


        float dr = std::sqrt(r * r + zri * zri), dv = std::sqrt(r * r + zvi * zvi);

        float expTermR = std::exp(-p.sigma_tr_ * dr);
        float expTermV = std::exp(-p.sigma_tr_ * dv);

        float divTermR = expTermR / dr;
        float divTermV = expTermV / dv;

        // Compute dipole fluency rate $\dipole(r)$ using Equation (15.27)
        float Rphi = (1.0f / (4.0f * glm::pi<float>() * p.D_)) * (divTermR - divTermV);

        float kappa = static_cast<float>(1.0) - std::exp(-static_cast<float>(2.0) * p.sigmat_ * (dr + std::abs(zri)));

        // Compute dipole vector irradiance $-\N{}\cdot\dipoleE(r)$ using
        // Equation (15.27)
        float EDn1 = zri * (static_cast<float>(1.0) + p.sigma_tr_ * dr) * expTermR / (dr * dr * dr);
        float EDn2 = zvi * (static_cast<float>(1.0) + p.sigma_tr_ * dv) * expTermV / (dv * dv * dv);

        float EDn = (1.0f / (4.0f * glm::pi<float>())) * (EDn1 + EDn2);

        // Add contribution from dipole for depth $\depthreal$ to _Ed_
        float R = Rphi * p.cPhi_ + EDn * p.cE_;
        // first rho: see PBD Eq. (5) + (6)
        // second rho: from source term
        // remainder of source term canceled out by PDF.
        return kappa * p.rho_ * p.rho_ * R;
    }

    float SSSSSRenderer::PBD(float r, const DiffusionParams& p) const {
        // return glm::exp(-r);

        float result = 0;
        float tCrit = r * std::sqrt(eta_ * eta_ - 1);

        // this makes sure we never leave the slab.

        for (int i = 0; i < BEAM_SAMPLES; ++i) {
            float u = (static_cast<float>(i) + .5f) / static_cast<float>(BEAM_SAMPLES);
            float l = -1;

            l = -std::log(1.0f - u) / p.sigmat_;

            assert(l > 0);

            result += GetDipoleValue(r, l, p);
        }

        result /= static_cast<float>(BEAM_SAMPLES);

        result += ComputeSingleScattering(r, p);

        return result / (4.0f * glm::pi<float>() * imprDiff::CalcCPhi(1.0f / eta_));
    }

    SSSSSRenderer::SSSSSRenderer(ApplicationNodeImplementation* appNode) :
        BaseRenderer{ PCType::SUBSURFACE, RenderType::PC_ON_MESH, "SSSSS", appNode }
    {
        s5Quad_ = std::make_unique<FullscreenQuad>("comparison/sssss.frag", GetApp());
        s5UniformLocations_ = s5Quad_->GetGPUProgram()->GetUniformLocations({ "positionTexture", "normalTexture", "materialColorTexture", "sssIlluminationTexture", "kernelTexture", "camPos", "maxOffsetMm", "dir", "distanceToProjectionWindow", "eta" });

        RecalculateKernel(true);
    }

    bool SSSSSRenderer::IsAvaialble() const
    {
        return (GetMesh() != nullptr);
    }

    void SSSSSRenderer::DrawPointCloudInternal(const FrameBuffer& fbo, const FrameBuffer& deferredFBO, bool batched)
    {
        GetMesh()->DrawShadowMap();
        deferredFBO.DrawToFBO(GetApp()->GetDeferredDrawIndices(), [this]() {
            GetMesh()->DrawMeshDeferred(true);
        });

        fbo.DrawToFBO([this, &deferredFBO]() {
            DrawHBAO(deferredFBO);
        });
    }

    double SSSSSRenderer::DoPerformanceMeasureInternal(const FrameBuffer& fbo, const FrameBuffer& deferredFBO, bool batched)
    {
        GetMesh()->DrawShadowMap();
        deferredFBO.DrawToFBO(GetApp()->GetDeferredDrawIndices(), [this]() {
            GetMesh()->DrawMeshDeferred(true);
        });

        auto start = std::chrono::high_resolution_clock::now();
        for (std::size_t i = 0; i < 100; ++i) {
            fbo.DrawToFBO([this, &deferredFBO]() {
                DrawHBAO(deferredFBO);
            });
            gl::glFlush();
            gl::glFinish();
        }

        auto stop = std::chrono::high_resolution_clock::now();
        return static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count()) / 100.0;
    }

    void SSSSSRenderer::DrawHBAO(const FrameBuffer& deferredFBO)
    {
        RecalculateKernel(false);

        gl::glUseProgram(s5Quad_->GetGPUProgram()->getProgramId());

        float aspectRatio = static_cast<float>(deferredFBO.GetHeight()) / static_cast<float>(deferredFBO.GetWidth());
        auto projMat = GetApp()->GetCameraEnh().GetProjMatrixEnh(GetApp()->GetCamera()->GetCentralPerspectiveMatrix());
        float distanceToProjectionWindow = 0.5f * projMat[1][1];
        glm::vec2 dir{ 1.0f / aspectRatio, 1.0f };

        std::array<std::size_t, 4> textureIndices{ 0, 1, 2, 4 };
        for (int i = 0; i < 4; ++i) {
            gl::glActiveTexture(gl::GL_TEXTURE0 + i);
            gl::glBindTexture(gl::GL_TEXTURE_2D, deferredFBO.GetTextures()[textureIndices[i]]);
            gl::glUniform1i(s5UniformLocations_[i], i);
        }

        gl::glActiveTexture(gl::GL_TEXTURE0 + 5);
        gl::glBindTexture(gl::GL_TEXTURE_2D, kernelTexture_);
        gl::glUniform1i(s5UniformLocations_[4], 5);

        gl::glUniform3fv(s5UniformLocations_[5], 1, glm::value_ptr(GetApp()->GetCameraEnh().GetPosition()));
        gl::glUniform1f(s5UniformLocations_[6], maxOffsetMm_);
        gl::glUniform2fv(s5UniformLocations_[7], 1, glm::value_ptr(dir));
        gl::glUniform1f(s5UniformLocations_[8], distanceToProjectionWindow);
        gl::glUniform1f(s5UniformLocations_[9], eta_);

        // gl::glUniformMatrix4fv(s5UniformLocations_[6], 1, false, glm::value_ptr(GetApp()->GetCameraEnh().GetViewMatrix()));

        gl::glEnable(gl::GL_BLEND);
        gl::glBlendEquationSeparate(gl::GL_FUNC_ADD, gl::GL_FUNC_ADD);
        gl::glBlendFuncSeparate(gl::GL_SRC_ALPHA, gl::GL_ONE_MINUS_SRC_ALPHA, gl::GL_ONE, gl::GL_ONE);
        // gl::glUniformMatrix4fv(deferredUniformLocations_[0], 1, gl::GL_FALSE, glm::value_ptr(VP));
        s5Quad_->Draw();

        gl::glDisable(gl::GL_BLEND);
        gl::glUseProgram(0);

        // GetMesh()->DrawMeshDeferredAndExport(true);
    }

    void SSSSSRenderer::RenderGUIByType()
    {
        if (ImGui::Button("Export SSSSS PBRT")) {
            glm::uvec2 size{ GetApp()->GetDeferredExportFBO().GetWidth(), GetApp()->GetDeferredExportFBO().GetHeight() };

            auto outputName = GetMesh()->GetMeshName() + "_sss";
            std::ofstream pbrtOut{ outputName + ".pbrt" };
            std::ofstream pbrtOut_do{ outputName + "_direct_only.pbrt" };
            ExportPBRT(outputName + "_pbrt", size, pbrtOut, pbrtOut_do);

            GetApp()->SaveImageAllTechniques(outputName);
        }
    }


    void SSSSSRenderer::RecalculateKernel(bool force)
    {
        if (!GetMesh() || !(*GetMesh())) return;
        auto alpha = GetMesh()->GetMesh(0).info_.albedo_;
        auto sigmat = GetMesh()->GetMesh(0).info_.sigmat_;
        auto eta = GetMesh()->GetMesh(0).info_.eta_;
        if (!force && (alpha_ == alpha && sigmat_ == sigmat && eta_ == eta)) return;
        if (sigmat.r == 0.0f || sigmat.g == 0.0f || sigmat.b == 0.0f) return;
        if (eta == 0.0f) return;

        sigmat_ = sigmat;
        alpha_ = alpha;
        eta_ = eta;

        for (int i = 0; i < 3; ++i) {
            diffParams_[i].sigmat_ = sigmat_[i];
            diffParams_[i].rho_ = alpha_[i];

            diffParams_[i].sigmas_ = diffParams_[i].rho_ * diffParams_[i].sigmat_;
            diffParams_[i].sigmaa_ = diffParams_[i].sigmat_ - diffParams_[i].sigmas_;

            diffParams_[i].A_ = imprDiff::CalcA(eta_);
            diffParams_[i].D_ = imprDiff::CalcD(diffParams_[i].sigmaa_, diffParams_[i].sigmas_);
            diffParams_[i].sigma_tr_ = std::sqrt(diffParams_[i].sigmaa_ / diffParams_[i].D_);
            diffParams_[i].zb_ = imprDiff::CalcZb(eta_, diffParams_[i].D_);

            diffParams_[i].cPhi_ = imprDiff::CalcCPhi(eta_);
            diffParams_[i].cE_ = imprDiff::CalcCE(eta_);
        }

        gl::glBindTexture(gl::GL_TEXTURE_2D, kernelTexture_);
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_S, gl::GL_CLAMP_TO_EDGE);
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_T, gl::GL_CLAMP_TO_EDGE);
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MAG_FILTER, gl::GL_LINEAR);
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MIN_FILTER, gl::GL_LINEAR);

        const std::size_t kernelTexSize = 32;
        const std::size_t halfKTS = ((kernelTexSize & 1) ? (kernelTexSize + 1) : kernelTexSize) / 2;

        std::vector<glm::vec4> kernelData;
        auto kAt = [kernelTexSize, halfKTS](std::vector<glm::vec4>& kernel, std::size_t x, std::size_t y) -> glm::vec4& {
            return kernel[(halfKTS - 1) * kernelTexSize + (halfKTS - 1) + y * kernelTexSize + x];
        };

        float sigmatmax = sigmat_.z;
        std::size_t comp = 2;
        if (sigmat_.x > sigmat_.y && sigmat_.x > sigmat_.z) {
            sigmatmax = sigmat_.x;
            comp = 0;
        }
        else if (sigmat_.y > sigmat_.z) {
            sigmatmax = sigmat_.y;
            comp = 1;
        }

        float minProfile = 1.0f;
        maxOffsetMm_ = 1.0f / sigmatmax; // set to 1 mean free path to start with.
        do {
            maxOffsetMm_ *= 1.1f;
            minProfile = PBD(maxOffsetMm_, diffParams_[comp]);
        } while (minProfile > 0.005f);

        auto calcKernel = [this, halfKTS](float r) -> glm::vec4 {
            auto rmod = r * maxOffsetMm_ / static_cast<float>(halfKTS - 1);
            return glm::vec4(glm::vec3(
                PBD(rmod, diffParams_[0]),
                PBD(rmod, diffParams_[1]),
                PBD(rmod, diffParams_[2])), 1.0f);
        };

        kernelData.resize(kernelTexSize * kernelTexSize);
        glm::vec4 oval;
        for (int tX = 0; tX < halfKTS; ++tX) {
            for (int tY = 0; tY < halfKTS; ++tY) {
                // calculate r when kernelTexSize is odd:
                auto r = glm::vec2(tX, tY);
                // else shift by 0.5
                if (!(kernelTexSize & 1)) r += glm::vec2(0.5f);

                auto v = glm::vec4(0.0f);
                if (tX > tY) v = kAt(kernelData, tY, tX);
                else v = calcKernel(glm::length(r));

                if (tX == 0 && tY == 0) oval = v;

                kAt(kernelData, tX, tY) = v;
                kAt(kernelData, tX, -tY) = v;
                kAt(kernelData, -tX, tY) = v;
                kAt(kernelData, -tX, -tY) = v;
            }
        }

        gl::glTexImage2D(gl::GL_TEXTURE_2D, 0, gl::GL_RGBA32F, kernelTexSize, kernelTexSize, 0, gl::GL_RGBA, gl::GL_FLOAT, kernelData.data());
    }

}
