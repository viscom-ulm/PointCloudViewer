/**
 * @file   WorkerNode.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.25
 *
 * @brief  Declaration of the ApplicationNodeImplementation for the worker node.
 */

#pragma once

#include "app/ApplicationNodeImplementation.h"

namespace viscom {

    class WorkerNode final : public ApplicationNodeImplementation
    {
    public:
        explicit WorkerNode(ApplicationNodeInternal* appNode);
        virtual ~WorkerNode() override;

        void Draw2D(FrameBuffer& fbo) override;

    };
}
