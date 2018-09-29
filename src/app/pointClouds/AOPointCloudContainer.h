/**
 * @file   AOPointCloudContainer.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.23
 *
 * @brief  Declaration of the ambient occlusion point cloud.
 */

#pragma once

#include "app/PointCloudContainer.h"

namespace pcViewer {

    class AOPointCloudContainer : public TBasePointCloudContainer<PointCloudPointAO>
    {
    public:
        AOPointCloudContainer(ApplicationNodeImplementation* appNode) : TBasePointCloudContainer{ PCType::AO, appNode } {}

        virtual bool HasDirectLight() const override { return false; }

    protected:
        virtual void PreparePointCloudVAO() override;
        virtual void StorePointInPointCloud(const std::vector<std::string>& pointData) override;
    };
}
