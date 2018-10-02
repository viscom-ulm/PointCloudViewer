/**
 * @file   inproved_diffusion.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.04.10
 *
 * @brief  Helper functions for improved diffusion.
 */

#pragma once

namespace imprDiff {

    constexpr float FresnelMoment1(float eta)
    {
        float eta2 = eta * eta, eta3 = eta2 * eta, eta4 = eta3 * eta, eta5 = eta4 * eta;
        if (eta < 1)
            return 0.45966f - 1.73965f * eta + 3.37668f * eta2 - 3.904945f * eta3 + 2.49277f * eta4 - 0.68441f * eta5;
        else
            return -4.61686f + 11.1136f * eta - 10.4646f * eta2 + 5.11455f * eta3 - 1.27198f * eta4 + 0.12746f * eta5;
    }

    constexpr float FresnelMoment2(float eta)
    {
        float eta2 = eta * eta, eta3 = eta2 * eta, eta4 = eta3 * eta, eta5 = eta4 * eta;
        if (eta < 1) {
            return 0.27614f - 0.87350f * eta + 1.12077f * eta2 - 0.65095f * eta3 + 0.07883f * eta4 + 0.04860f * eta5;
        }
        else {
            float r_eta = 1 / eta, r_eta2 = r_eta * r_eta, r_eta3 = r_eta2 * r_eta;
            return -547.033f + 45.3087f * r_eta3 - 218.725f * r_eta2 + 458.843f * r_eta + 404.557f * eta
                - 189.519f * eta2 + 54.9327f * eta3 - 9.00603f * eta4 + 0.63942f * eta5;
        }
    }

    constexpr float CalcA(float eta)
    {
        return (1.0f + 3.0f * FresnelMoment2(eta)) / (1.0f - 2.0f * FresnelMoment1(eta));
    }

    constexpr float CalcD(float mua, float mus)
    {
        auto musp = mus;
        auto mutp = musp + mua;
        return (2.0f * mua + musp) / (3.0f * mutp * mutp);
    }

    constexpr float CalcZb(float eta, float D)
    {
        return 2.0f * CalcA(eta) * D;
    }

    constexpr float CalcCPhi(float eta)
    {
        return 0.25f * (1.0f - 2.0f * FresnelMoment1(eta));
    }

    constexpr float CalcCE(float eta)
    {
        return 0.5f * (1.0f - 3.0f * FresnelMoment2(eta));
    }

}