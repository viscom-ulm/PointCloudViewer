/**
 * @file   PointCloudRenderer.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.23
 *
 * @brief  Declaration of the point cloud renderer base class.
 */

#pragma once

#include "app/Renderer.h"

namespace pcViewer {

    class PointCloudRenderer : public BaseRenderer
    {
    public:
        PointCloudRenderer(PCType pcType, ApplicationNodeImplementation* appNode);

        virtual bool IsAvaialble() const override;

    protected:
        virtual void DrawPointCloudInternal(const FrameBuffer& fbo, const FrameBuffer& deferredFBO, bool batched) override;
        virtual void DrawPointCloudPoints(const glm::mat4& MVP, const glm::vec3& camPos, bool batched) = 0;
    };
}
