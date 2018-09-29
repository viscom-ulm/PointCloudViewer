/**
 * @file   SSSPointCloudContainer.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.23
 *
 * @brief  Implementation of the subsurface scattering point cloud.
 */

#define GLM_SWIZZLE

#include "SSSPointCloudContainer.h"
#include "app/ApplicationNodeImplementation.h"


namespace pcViewer {

    void SSSPointCloudContainer::PreparePointCloudVAO()
    {
        using PointCloudPoint = PointCloudPointSubsurface;

        gl::glEnableVertexAttribArray(0);
        gl::glVertexAttribPointer(0, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPoint), reinterpret_cast<GLvoid*>(offsetof(PointCloudPoint, position_)));
        gl::glEnableVertexAttribArray(1);
        gl::glVertexAttribPointer(1, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPoint), reinterpret_cast<GLvoid*>(offsetof(PointCloudPoint, normal_)));
        gl::glEnableVertexAttribArray(2);
        gl::glVertexAttribPointer(2, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPoint), reinterpret_cast<GLvoid*>(offsetof(PointCloudPoint, albedo_)));
        gl::glEnableVertexAttribArray(3);
        gl::glVertexAttribPointer(3, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPoint), reinterpret_cast<GLvoid*>(offsetof(PointCloudPoint, sigma_tp_)));
        gl::glEnableVertexAttribArray(4);
        gl::glVertexAttribPointer(4, 1, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPoint), reinterpret_cast<GLvoid*>(offsetof(PointCloudPoint, eta_)));
        gl::glEnableVertexAttribArray(5);
        gl::glVertexAttribPointer(5, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPoint), reinterpret_cast<GLvoid*>(offsetof(PointCloudPoint, directIllumination_)));
        gl::glEnableVertexAttribArray(6);
        gl::glVertexAttribPointer(6, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(PointCloudPoint), reinterpret_cast<GLvoid*>(offsetof(PointCloudPoint, globalIllumination_)));
    }

    void SSSPointCloudContainer::StorePointInPointCloud(const std::vector<std::string>& pointData)
    {
        if (pointData.size() < 19) return;

        auto& point = EditNextPoint();
        point.position_.x = static_cast<float>(std::atof(pointData[0].c_str()));
        point.position_.y = static_cast<float>(std::atof(pointData[1].c_str()));
        point.position_.z = static_cast<float>(std::atof(pointData[2].c_str()));
        point.position_.w = 1.f;
        GetApp()->AddToBoundingSphere(point.position_.xyz);

        point.normal_.x = static_cast<float>(std::atof(pointData[3].c_str()));
        point.normal_.y = static_cast<float>(std::atof(pointData[4].c_str()));
        point.normal_.z = static_cast<float>(std::atof(pointData[5].c_str()));

        point.directIllumination_.x = static_cast<float>(std::atof(pointData[6].c_str()));
        point.directIllumination_.y = static_cast<float>(std::atof(pointData[7].c_str()));
        point.directIllumination_.z = static_cast<float>(std::atof(pointData[8].c_str()));

        point.albedo_.x = static_cast<float>(std::atof(pointData[9].c_str()));
        point.albedo_.y = static_cast<float>(std::atof(pointData[10].c_str()));
        point.albedo_.z = static_cast<float>(std::atof(pointData[11].c_str()));

        point.sigma_tp_.x = static_cast<float>(std::atof(pointData[12].c_str()));
        point.sigma_tp_.y = static_cast<float>(std::atof(pointData[13].c_str()));
        point.sigma_tp_.z = static_cast<float>(std::atof(pointData[14].c_str()));

        point.eta_ = static_cast<float>(std::atof(pointData[15].c_str()));

        point.globalIllumination_.x = static_cast<float>(std::atof(pointData[16].c_str()));
        point.globalIllumination_.y = static_cast<float>(std::atof(pointData[17].c_str()));
        point.globalIllumination_.z = static_cast<float>(std::atof(pointData[18].c_str()));
    }

}
