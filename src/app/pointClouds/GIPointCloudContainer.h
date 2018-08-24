/**
 * @file   GIPointCloudContainer.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.23
 *
 * @brief  Declaration of the global illumination point cloud.
 */

#pragma once

#include "app/PointCloudContainer.h"

namespace pcViewer {

    class GIPointCloudContainer : public TBasePointCloudContainer<PointCloudPointMatte>
    {
    public:
        GIPointCloudContainer(ApplicationNodeImplementation* appNode) : TBasePointCloudContainer{ PCType::MATTE, appNode } {}

    protected:
        virtual void PreparePointCloudVAO() override;
        virtual void StorePointInPointCloud(const std::vector<std::string>& pointData) override;
    };
}
