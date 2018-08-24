/**
 * @file   PointCloudContainer.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.23
 *
 * @brief  Implementation of a container class for point clouds.
 */

#include "PointCloudContainer.h"
#include "app/ApplicationNodeImplementation.h"
#include <fstream>

namespace pcViewer {

    BasePointCloudContainer::BasePointCloudContainer(PCType pcType, ApplicationNodeImplementation* appNode) :
        pcType_{ pcType }, appNode_{ appNode }
    {
    }

    void BasePointCloudContainer::LoadPointCloud(const std::string& pointCloudName, const std::string& pointCloud)
    {
        pointCloudName_ = pointCloudName;
        std::ifstream pc_in(pointCloud);
        ClearPointCloud();
        GetApp()->ClearRadius();

        std::string line;
        while (pc_in.good()) {
            std::getline(pc_in, line);

            auto splitPointData = utils::split(line, ',');

            StorePointInPointCloud(splitPointData);
        }

        LoadPointCloudGPU();
    }

    void BasePointCloudContainer::PreparePointCloudVAOInternal()
    {
        gl::glBindVertexArray(vaoPointCloud_);
        PreparePointCloudVAO();
        gl::glBindVertexArray(0);
        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, 0);

        appNode_->GetCameraEnh().SetCameraPosition(appNode_->GetBoundingSphereRadius() * appNode_->GetCamera()->GetCentralPerspectiveMatrix()[1][1] * glm::normalize(appNode_->GetCameraEnh().GetPosition()));
    }
}
