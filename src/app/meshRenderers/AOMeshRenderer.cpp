/**
 * @file   AOMeshRenderer.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.27
 *
 * @brief  Definition of the mesh renderer for ambient occlusion.
 */

#include "AOMeshRenderer.h"
#include <glm/gtc/type_ptr.hpp>
#include "app/ApplicationNodeImplementation.h"
#include "enh/gfx/env/EnvironmentMapRenderer.h"
#include "app/PointCloudContainer.h"
#include "app/MeshContainer.h"

namespace pcViewer {

    AOMeshRenderer::AOMeshRenderer(ApplicationNodeImplementation* appNode) :
        MeshRenderer{ PCType::AO, appNode }
    {
    }

    void AOMeshRenderer::RenderGUIByType()
    {
    }

    void AOMeshRenderer::ExportScreenPointCloudScreen(const FrameBuffer& fbo, std::ostream& screenPoints) const
    {
        std::vector<glm::vec3> screenPositions, screenNormals;
        screenPositions.resize(static_cast<std::size_t>(fbo.GetWidth()) * fbo.GetHeight());
        screenNormals.resize(static_cast<std::size_t>(fbo.GetWidth()) * fbo.GetHeight());

        auto pboPos = CopyTextureToPixelBuffer3(fbo, 0);
        auto pboNormal = CopyTextureToPixelBuffer3(fbo, 1);

        gl::glMemoryBarrier(gl::GL_ALL_BARRIER_BITS);

        CopyPBOToVector3(pboPos, screenPositions);
        CopyPBOToVector3(pboNormal, screenNormals);

        for (std::size_t i = 0; i < screenPositions.size(); ++i) {
            screenPoints << screenPositions[i].x << ',' << screenPositions[i].y << ',' << screenPositions[i].z << ','
                << screenNormals[i].x << ',' << screenNormals[i].y << ',' << screenNormals[i].z << '\n';
        }
    }

    void AOMeshRenderer::ExportScreenPointCloudMesh(std::ostream& meshPoints) const
    {
        using PointCloudPoint = PointCloudPointAO;
        const auto& positions = GetMesh()->GetPositions();
        const auto& normals = GetMesh()->GetNormals();

        std::vector<PointCloudPoint> pointCloud(positions.size());

        for (std::size_t i = 0; i < pointCloud.size(); ++i) {
            pointCloud[i].position_ = positions[i];
            pointCloud[i].normal_ = normals[i];
        }

        std::copy(pointCloud.begin(), pointCloud.end(), std::ostream_iterator<PointCloudPoint>(meshPoints, "\n"));
    }

}
