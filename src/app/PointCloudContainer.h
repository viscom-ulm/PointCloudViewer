/**
 * @file   PointCloudContainer.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.23
 *
 * @brief  Declaration of a container class for point clouds.
 */

#pragma once

#include "enh/gfx/gl/OpenGLRAIIWrapper.h"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <ostream>

namespace viscom {
    class ApplicationNodeImplementation;
}

namespace pcViewer {

    using namespace viscom;

    enum class PCType : std::size_t {
        AO = 0,
        MATTE = 1,
        SUBSURFACE = 2
    };

    struct PointCloudPointAO {
        glm::vec3 position_;
        glm::vec3 normal_;
        float ao_ = 0.0f;
    };

    static std::ostream& operator <<(std::ostream& out, const PointCloudPointAO& pcp) {
        return out << pcp.position_.x << ',' << pcp.position_.y << ',' << pcp.position_.z << ',' << pcp.normal_.x << ',' << pcp.normal_.y << ',' << pcp.normal_.z;
    }

    struct PointCloudPointMatte {
        glm::vec4 position_;
        glm::vec4 normal_;
        glm::vec4 albedo_;
        glm::vec4 directIllumination_;
        glm::vec4 globalIllumination_;
    };

    static std::ostream& operator <<(std::ostream& out, const PointCloudPointMatte& pcp) {
        return out << pcp.position_.x << ',' << pcp.position_.y << ',' << pcp.position_.z << ',' << pcp.normal_.x << ',' << pcp.normal_.y << ',' << pcp.normal_.z
            << ',' << pcp.albedo_.r << ',' << pcp.albedo_.g << ',' << pcp.albedo_.b << ',' << pcp.directIllumination_.r << ',' << pcp.directIllumination_.g << ',' << pcp.directIllumination_.b;
    }

    struct PointCloudPointSubsurface {
        glm::vec4 position_;
        glm::vec4 normal_;
        glm::vec4 albedo_;
        glm::vec3 sigma_tp_;
        float eta_ = 1.33f;
        glm::vec4 directIllumination_;
        glm::vec4 globalIllumination_;
    };

    static std::ostream& operator <<(std::ostream& out, const PointCloudPointSubsurface& pcp) {
        if (pcp.position_.w == 0.0) return out;
        return out << pcp.position_.x << ',' << pcp.position_.y << ',' << pcp.position_.z << ',' << pcp.normal_.x << ',' << pcp.normal_.y << ',' << pcp.normal_.z
            << ',' << pcp.albedo_.r << ',' << pcp.albedo_.g << ',' << pcp.albedo_.b << ',' << pcp.sigma_tp_.r << ',' << pcp.sigma_tp_.g << ',' << pcp.sigma_tp_.b
            << ',' << pcp.eta_ << ',' << pcp.directIllumination_.r << ',' << pcp.directIllumination_.g << ',' << pcp.directIllumination_.b;
    }

    class BasePointCloudContainer
    {
    public:
        BasePointCloudContainer(PCType pcType, ApplicationNodeImplementation* appNode);
        virtual ~BasePointCloudContainer() = default;

        void LoadPointCloud(const std::string& pointCloudName, const std::string& pointCloud);
        virtual void ExportScreenPointCloudMesh(std::ostream& meshPoints) const = 0;

        const std::string& GetPointCloudName() const { return pointCloudName_; }
        const enh::BufferRAII& GetVBO() const { return vboPointCloud_; }
        const enh::VertexArrayRAII& GetVAO() const { return vaoPointCloud_; }

        virtual bool HasDirectLight() const = 0;
        virtual std::size_t GetPointCloudSize() const = 0;
        PCType GetPointCloudType() const { return pcType_; }

    protected:
        virtual void LoadPointCloudGPU() = 0;
        virtual void ClearPointCloud() = 0;
        void PreparePointCloudVAOInternal();
        virtual void PreparePointCloudVAO() = 0;
        virtual void StorePointInPointCloud(const std::vector<std::string>& pointData) = 0;

        ApplicationNodeImplementation* GetApp() { return appNode_; }

    private:
        PCType pcType_;
        /** Holds the point clouds name. */
        std::string pointCloudName_;
        /** Holds the vertex buffer for the point could. */
        enh::BufferRAII vboPointCloud_;
        /** Holds the vertex array object for the point cloud. */
        enh::VertexArrayRAII vaoPointCloud_;

        ApplicationNodeImplementation* appNode_;
    };

    template<class PointCloudPoint> class TBasePointCloudContainer : public BasePointCloudContainer
    {
    public:
        TBasePointCloudContainer(PCType pcType, ApplicationNodeImplementation* appNode) : BasePointCloudContainer{ pcType, appNode } {}

        virtual void ExportScreenPointCloudMesh(std::ostream& meshPoints) const override;
        const std::vector<PointCloudPoint>& GetPointCloud() const { return pointCloud_; }

        virtual std::size_t GetPointCloudSize() const override { return pointCloud_.size(); }

    protected:
        virtual void LoadPointCloudGPU() override;
        virtual void ClearPointCloud() override { pointCloud_.clear(); }
        PointCloudPoint& EditNextPoint() { return pointCloud_.emplace_back(); }

    private:
        std::vector<PointCloudPoint> pointCloud_;
    };

    template<class PointCloudPoint>
    void TBasePointCloudContainer<PointCloudPoint>::ExportScreenPointCloudMesh(std::ostream& meshPoints) const
    {
        std::copy(pointCloud_.begin(), pointCloud_.end(), std::ostream_iterator<PointCloudPoint>(meshPoints, "\n"));
    }

    template<class PointCloudPoint>
    inline void TBasePointCloudContainer<PointCloudPoint>::LoadPointCloudGPU()
    {
        gl::glBindBuffer(gl::GL_ARRAY_BUFFER, GetVBO());
        gl::glBufferData(gl::GL_ARRAY_BUFFER, pointCloud_.size() * sizeof(PointCloudPoint), pointCloud_.data(), gl::GL_STATIC_DRAW);

        PreparePointCloudVAOInternal();
    }
}