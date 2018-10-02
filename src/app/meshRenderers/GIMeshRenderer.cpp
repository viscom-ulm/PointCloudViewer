/**
 * @file   GIMeshRenderer.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2018.08.29
 *
 * @brief  Definition of the mesh renderer for global illumination.
 */

#include "GIMeshRenderer.h"
#include <glm/gtc/type_ptr.hpp>
#include "app/ApplicationNodeImplementation.h"
#include "enh/gfx/env/EnvironmentMapRenderer.h"
#include "app/PointCloudContainer.h"
#include "app/MeshContainer.h"
#include <imgui.h>

namespace pcViewer {

    GIMeshRenderer::GIMeshRenderer(ApplicationNodeImplementation* appNode) :
        MeshRenderer{ PCType::MATTE, appNode }
    {
        // meshExportShader_ = GetApp()->GetGPUProgramManager().GetResource("meshExportGI", std::vector<std::string>{ "meshExportGI.comp" });
        // meshExportUniformLocations_ = meshExportShader_->GetUniformLocations({ "" });

    }

    void GIMeshRenderer::RenderGUIByType()
    {
        if (ImGui::RadioButton("Global Illumination", GetApp()->GetMatteRenderType() == 0)) GetApp()->SetMatteRenderType(0);
        if (ImGui::RadioButton("Matte Albedo", GetApp()->GetMatteRenderType() == 1)) GetApp()->SetMatteRenderType(1);
        if (ImGui::RadioButton("Direct Illumination", GetApp()->GetMatteRenderType() == 2)) GetApp()->SetMatteRenderType(2);
    }

    // void GIMeshRenderer::ExportScreenPointCloudScreen(const FrameBuffer& fbo, const std::string& namePrefix, std::ostream& screenPoints) const
    // {
    //     std::vector<glm::vec3> screenPositions, screenNormals, screenAlbedo, screenDirectIllumination;
    //     screenPositions.resize(static_cast<std::size_t>(fbo.GetWidth()) * fbo.GetHeight());
    //     screenNormals.resize(static_cast<std::size_t>(fbo.GetWidth()) * fbo.GetHeight());
    //     screenAlbedo.resize(static_cast<std::size_t>(fbo.GetWidth()) * fbo.GetHeight());
    //     screenDirectIllumination.resize(static_cast<std::size_t>(fbo.GetWidth()) * fbo.GetHeight());
    // 
    //     auto pboPos = CopyTextureToPixelBuffer3(fbo, 0);
    //     auto pboNormal = CopyTextureToPixelBuffer3(fbo, 1);
    //     auto pboAlbedo = CopyTextureToPixelBuffer3(fbo, 2);
    //     auto pboDirectIllumination = CopyTextureToPixelBuffer3(fbo, 4);
    // 
    //     gl::glMemoryBarrier(gl::GL_ALL_BARRIER_BITS);
    // 
    //     CopyPBOToVector3(pboPos, screenPositions);
    //     CopyPBOToVector3(pboNormal, screenNormals);
    //     CopyPBOToVector3(pboAlbedo, screenAlbedo);
    //     CopyPBOToVector3(pboDirectIllumination, screenDirectIllumination);
    // 
    //     for (std::size_t i = 0; i < screenPositions.size(); ++i) {
    //         screenPoints << screenPositions[i].x << ',' << screenPositions[i].y << ',' << screenPositions[i].z << ','
    //             << screenNormals[i].x << ',' << screenNormals[i].y << ',' << screenNormals[i].z << ','
    //             << screenAlbedo[i].r << ',' << screenAlbedo[i].g << ',' << screenAlbedo[i].b << ','
    //             << screenDirectIllumination[i].r << ',' << screenDirectIllumination[i].g << ',' << screenDirectIllumination[i].b << '\n';
    //     }
    // }

}
