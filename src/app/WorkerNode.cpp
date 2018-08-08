/**
 * @file   WorkerNode.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.25
 *
 * @brief  Implementation of the worker application node.
 */

#include "WorkerNode.h"
#include <imgui.h>

namespace viscom {

    WorkerNode::WorkerNode(ApplicationNodeInternal* appNode) :
        ApplicationNodeImplementation{ appNode }
    {
    }

    WorkerNode::~WorkerNode() = default;

    void WorkerNode::Draw2D(FrameBuffer& fbo)
    {
        if constexpr (SHOW_CLIENT_GUI) ImGui::ShowTestWindow();
    }

}
