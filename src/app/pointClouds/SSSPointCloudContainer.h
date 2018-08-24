/**
 * @file   SSSPointCloudContainer.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.23
 *
 * @brief  Declaration of the subsurface scattering point cloud.
 */

#pragma once

#include "app/PointCloudContainer.h"

namespace pcViewer {

    class SSSPointCloudContainer : public TBasePointCloudContainer<PointCloudPointSubsurface>
    {
    public:
        SSSPointCloudContainer(ApplicationNodeImplementation* appNode) : TBasePointCloudContainer{ PCType::SUBSURFACE, appNode } {}

    protected:
        virtual void PreparePointCloudVAO() override;
        virtual void StorePointInPointCloud(const std::vector<std::string>& pointData) override;
    };
}
