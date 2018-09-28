/**
 * @file   ApplicationNodeImplementation.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.30
 *
 * @brief  Implementation of the application node class.
 */

#include "ApplicationNodeImplementation.h"

#include "core/glfw.h"
#include <glbinding/gl/gl.h>
#include <glbinding/Binding.h>
#include <imgui.h>
#include <iostream>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>

#include "Vertices.h"
#include "enh/gfx/postprocessing/DepthOfField.h"
#include "enh/gfx/postprocessing/BloomEffect.h"
#include "enh/gfx/postprocessing/FilmicTMOperator.h"
#include "enh/gfx/env/EnvironmentMapRenderer.h"
#include "app/gfx/mesh/MeshRenderable.h"
#include "app/Vertices.h"
#include "app/Renderer.h"
#include "app/MeshContainer.h"
#include "app/pointClouds/AOPointCloudContainer.h"
#include "app/pointClouds/GIPointCloudContainer.h"
#include "app/pointClouds/SSSPointCloudContainer.h"
#include "app/pcRenderers/AOPointCloudRenderer.h"
#include "app/pcRenderers/GIPointCloudRenderer.h"
#include "app/pcRenderers/SSSPointCloudRenderer.h"
#include "app/pcOnMeshRenderers/AOPCOnMeshRenderer.h"
#include "app/pcOnMeshRenderers/GIPCOnMeshRenderer.h"
#include "app/pcOnMeshRenderers/SSSPCOnMeshRenderer.h"
#include "app/meshRenderers/AOMeshRenderer.h"
#include "app/meshRenderers/GIMeshRenderer.h"
#include "app/meshRenderers/SSSMeshRenderer.h"
#include "app/comparison/HBAORenderer.h"
#include "app/comparison/SSGIRenderer.h"
#include "app/comparison/SSSSSRenderer.h"

#include "python_fix.h"

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>

namespace viscom {

    ApplicationNodeImplementation::ApplicationNodeImplementation(ApplicationNodeInternal* appNode) :
        ApplicationNodeBase{ appNode },
        baseRenderType_{ pcViewer::RenderType::POINTCLOUD },
        currentPointCloudType_{ pcViewer::PCType::AO },
        screenRenderingComposition_{ 0 },
        camera_(glm::vec3(0.0f, 0.0f, 5.0f), *GetCamera()),
        deferredFBODesc_{ {
                FrameBufferTextureDescriptor{ static_cast<GLenum>(gl::GL_RGBA32F) }, // position
                FrameBufferTextureDescriptor{ static_cast<GLenum>(gl::GL_RGBA32F) }, // normal
                FrameBufferTextureDescriptor{ static_cast<GLenum>(gl::GL_RGBA32F) }, // color (albedo)
                FrameBufferTextureDescriptor{ static_cast<GLenum>(gl::GL_RGBA32F) }, // scattering parameters (sigma_t + eta)
                FrameBufferTextureDescriptor{ static_cast<GLenum>(gl::GL_RGBA32F) }, // direct illumination
                FrameBufferTextureDescriptor{ static_cast<GLenum>(gl::GL_RGBA32F) }, // global illumination
                FrameBufferTextureDescriptor{ static_cast<GLenum>(gl::GL_DEPTH_COMPONENT32F) } }, {} }
    {
        InitOpenGLInternal();
    }

    ApplicationNodeImplementation::~ApplicationNodeImplementation()
    {
        CleanUpInternal();
    }

    void ApplicationNodeImplementation::InitOpenGLInternal()
    {
        screenTexturesPtr_ = std::make_unique<enh::TexuturesRAII<5>>();

        screenRenderQuad_ = std::make_unique<FullscreenQuad>("renderScreen.frag", this);
        screenRenderUniformLocations_ = screenRenderQuad_->GetGPUProgram()->GetUniformLocations({ "positionTexture", "normalTexture", "materialColorTexture", "directIlluminationTexture", "globalIlluminationTexture",
            "lightPos", "lightColor", "lightMultiplicator", "camPos", "compositionType" });

        deferredFBOs_ = CreateOffscreenBuffers(deferredFBODesc_);
        deferredExportFBO_ = std::make_unique<FrameBuffer>(deferredFBOs_[0].GetWidth(), deferredFBOs_[0].GetHeight(), deferredFBODesc_);

        deferredDrawIndices_ = { 0, 1, 2, 3, 4 };
        distanceSumDrawIndices_ = { 4, 5 };
        deferredPositionIndices_ = { 0 };
        deferredNonPosIndices_ = { 1, 2, 3, 4, 5 };

        renderers_[0][0] = std::make_unique<pcViewer::AOPointCloudRenderer>(this);
        renderers_[0][1] = std::make_unique<pcViewer::AOPCOnMeshRenderer>(this);
        renderers_[0][2] = std::make_unique<pcViewer::AOMeshRenderer>(this);
        renderers_[0][3] = std::make_unique<pcViewer::HBAORenderer>(this);

        renderers_[1][0] = std::make_unique<pcViewer::GIPointCloudRenderer>(this);
        renderers_[1][1] = std::make_unique<pcViewer::GIPCOnMeshRenderer>(this);
        renderers_[1][2] = std::make_unique<pcViewer::GIMeshRenderer>(this);
        renderers_[1][3] = std::make_unique<pcViewer::SSGIRenderer>(this);

        renderers_[2][0] = std::make_unique<pcViewer::SSSPointCloudRenderer>(this);
        renderers_[2][1] = std::make_unique<pcViewer::SSSPCOnMeshRenderer>(this);
        renderers_[2][2] = std::make_unique<pcViewer::SSSMeshRenderer>(this);
        renderers_[2][3] = std::make_unique<pcViewer::SSSSSRenderer>(this);

        pointClouds_[0] = std::make_unique<pcViewer::AOPointCloudContainer>(this);
        pointClouds_[1] = std::make_unique<pcViewer::GIPointCloudContainer>(this);
        pointClouds_[2] = std::make_unique<pcViewer::SSSPointCloudContainer>(this);

        mesh_ = std::make_unique<pcViewer::MeshContainer>(this);


        {if (_import_array() < 0) { PyErr_Print(); PyErr_SetString(PyExc_ImportError, "numpy.core.multiarray failed to import"); return; } }
        
        std::array<npy_intp, 2> dims = { 1000, 4 };

        std::vector<float> test;
        constexpr float smallIncr = 0.1f;
        constexpr float bigIncr = 1.0f;

        float cVal = 0.0f;
        for (std::size_t i = 0; i < static_cast<std::size_t>(dims[0]); ++i) {
            float arrayVal = cVal;
            for (std::size_t j = 0; j < static_cast<std::size_t>(dims[1]); ++j) {
                test.push_back(arrayVal);
                arrayVal += smallIncr;
            }
            cVal += bigIncr;
        }

        auto pyArray = PyArray_SimpleNewFromData(2, dims.data(), NPY_FLOAT, test.data());

        using namespace std::string_literals;
        auto programName = Resource::FindResourceLocation("python/ptcltest.py", &GetApplication()->GetFramework());
        auto functionName = "test"s;

        namespace fs = std::filesystem;
        fs::path programPath(programName);
        programPath = fs::absolute(programPath);

        PyObject* sysPath = PySys_GetObject((char*)"path");
        auto programPyPath = PyUnicode_DecodeFSDefault(programPath.parent_path().string().c_str());
        PyList_Append(sysPath, programPyPath);
        Py_DECREF(programPyPath);



        auto pName = PyUnicode_DecodeFSDefault(programPath.stem().string().c_str());
        /* Error checking of pName left out */

        auto pModule = PyImport_Import(pName);
        Py_DECREF(pName);

        if (pModule != nullptr) {
            auto pFunc = PyObject_GetAttrString(pModule, functionName.c_str());
            /* pFunc is a new reference */

            if (pFunc && PyCallable_Check(pFunc)) {
                auto pArgs = PyTuple_New(1);
                PyTuple_SetItem(pArgs, 0, pyArray);
                auto pValue = PyObject_CallObject(pFunc, pArgs);
                Py_DECREF(pArgs);
                if (pValue != nullptr) {
                    printf("Result of call: %ld\n", PyLong_AsLong(pValue));
                    Py_DECREF(pValue);
                }
                else {
                    Py_DECREF(pFunc);
                    Py_DECREF(pModule);
                    PyErr_Print();
                    fprintf(stderr, "Call failed\n");
                }
            }
            else {
                if (PyErr_Occurred())
                    PyErr_Print();
                fprintf(stderr, "Cannot find function \"%s\"\n", functionName.c_str());
            }
            Py_XDECREF(pFunc);
            Py_DECREF(pModule);
        }
        else {
            PyErr_Print();
            fprintf(stderr, "Failed to load \"%s\"\n", programName.c_str());
        }

        // Py
    }

    void ApplicationNodeImplementation::UpdateFrame(double currentTime, double elapsedTime)
    {
        camera_.UpdateCamera(elapsedTime, this);
    }

    void ApplicationNodeImplementation::ClearBuffer(FrameBuffer& fbo)
    {
        fbo.DrawToFBO([]() {
            gl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);
        });
        SelectOffscreenBuffer(deferredFBOs_)->DrawToFBO([]() {
            gl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);
        });
    }

    void ApplicationNodeImplementation::ClearExportScreenPointCloud(const FrameBuffer& deferredFBO)
    {
        deferredFBO.DrawToFBO(deferredPositionIndices_, []() {
            gl::glClearColor(999999.0f, 999999.0f, 999999.0f, 0.0f);
            gl::glClear(gl::GL_COLOR_BUFFER_BIT);
        });

        deferredFBO.DrawToFBO(deferredNonPosIndices_, []() {
            gl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);
        });
    }

    void ApplicationNodeImplementation::RendererSelectionGUI()
    {
        if (baseRenderType_ == pcViewer::RenderType::SCREEN) {
            if (ImGui::RadioButton("Results only", screenRenderingComposition_ == 0)) screenRenderingComposition_ = 0;
            if (ImGui::RadioButton("Direct Illumination only", screenRenderingComposition_ == 1)) screenRenderingComposition_ = 1;
            if (ImGui::RadioButton("Global + Direct Illumination", screenRenderingComposition_ == 2)) screenRenderingComposition_ = 2;
            if (ImGui::RadioButton("Direct Illumination + AO", screenRenderingComposition_ == 3)) screenRenderingComposition_ = 3;
        }
        else {
            if (!currentRenderers_) return;
            for (std::size_t i = 0; i < currentRenderers_->size(); ++i) {
                auto& cR = (*currentRenderers_)[i];
                if (!cR) continue;
                if (cR->IsAvaialble() && ImGui::RadioButton(cR->GetRendererName().c_str(), i == static_cast<std::size_t>(baseRenderType_)))
                    baseRenderType_ = static_cast<pcViewer::RenderType>(i);
            }

            ImGui::Spacing();
            auto brt = static_cast<std::size_t>(baseRenderType_);
            if (brt < 4) (*currentRenderers_)[brt]->RenderGUI();
        }
    }

    void ApplicationNodeImplementation::SetBaseRenderType(pcViewer::RenderType type)
    {
        auto& cR = (*currentRenderers_)[static_cast<std::size_t>(type)];
        if (!cR || !cR->IsAvaialble()) throw std::invalid_argument("Render type currently not available.");

        baseRenderType_ = type;
    }

    void ApplicationNodeImplementation::SelectRenderers(pcViewer::PCType type)
    {
        currentPointCloudType_ = type;
        currentRenderers_ = &renderers_[static_cast<std::size_t>(type)];
        for (auto& renderer : *currentRenderers_) {
            if (!renderer) continue;
            renderer->SetPointCloud(nullptr);
            renderer->SetMesh(nullptr);
        }
        currentPointCloud_ = pointClouds_[static_cast<std::size_t>(type)].get();
    }

    void ApplicationNodeImplementation::RenderersLoadPointCloud(const std::string& pointCloudName, const std::string& pointCloud, const glm::mat4& modelMatrix)
    {
        currentPointCloud_->LoadPointCloud(pointCloudName, pointCloud, modelMatrix);
        for (const auto& renderer : *currentRenderers_) {
            if (!renderer) continue;
            renderer->SetPointCloud(currentPointCloud_);
        }
    }

    void ApplicationNodeImplementation::RenderersSetMesh(const std::string& meshName, std::shared_ptr<Mesh> mesh, float theta, float phi, bool doRescale)
    {
        mesh_->SetMesh(meshName, mesh, theta, phi, doRescale);
        for (const auto& renderer : *currentRenderers_) {
            if (!renderer) continue;
            renderer->SetMesh(mesh_.get());
        }
    }

    void ApplicationNodeImplementation::RenderersSetMesh(const std::string& meshName, std::shared_ptr<Mesh> mesh, const glm::mat4& modelMatrix, bool doRescale)
    {
        mesh_->SetMesh(meshName, mesh, modelMatrix, doRescale);
        for (const auto& renderer : *currentRenderers_) {
            if (!renderer) continue;
            renderer->SetMesh(mesh_.get());
        }
    }

    void ApplicationNodeImplementation::RenderersSetEnvironmentMap(std::shared_ptr<Texture> envMap)
    {
        for (const auto& renderer : *currentRenderers_) {
            if (!renderer) continue;
            renderer->SetEnvironmentMap(envMap);
        }
    }

    void ApplicationNodeImplementation::CurrentRendererDrawPointCloud(const FrameBuffer& fbo, const FrameBuffer& deferredFBO, bool batched) const
    {
        (*currentRenderers_)[static_cast<std::size_t>(baseRenderType_)]->DrawPointCloud(fbo, deferredFBO, batched);
    }

    void ApplicationNodeImplementation::DrawLoadedScreen(const FrameBuffer& fbo) const
    {
        fbo.DrawToFBO([this] {
            gl::glUseProgram(screenRenderQuad_->GetGPUProgram()->getProgramId());

            for (int i = 0; i < 5; ++i) {
                gl::glActiveTexture(gl::GL_TEXTURE0 + i);
                gl::glBindTexture(gl::GL_TEXTURE_2D, (*screenTexturesPtr_)[i]);
                gl::glUniform1i(screenRenderUniformLocations_[i], i);
            }

            gl::glUniform3fv(screenRenderUniformLocations_[5], 1, glm::value_ptr(lightPos_));
            gl::glUniform3fv(screenRenderUniformLocations_[6], 1, glm::value_ptr(lightColor_));
            gl::glUniform1f(screenRenderUniformLocations_[7], lightMultiplicator_);
            gl::glUniform3fv(screenRenderUniformLocations_[8], 1, glm::value_ptr(camera_.GetPosition()));
            gl::glUniform1i(screenRenderUniformLocations_[5], screenRenderingComposition_);
            screenRenderQuad_->Draw();

            gl::glUseProgram(0);
        });
    }

    void ApplicationNodeImplementation::UpdateBaseRendererType()
    {
        for (const auto& renderer : *currentRenderers_) {
            if (!renderer) continue;
            if (renderer->IsAvaialble()) {
                baseRenderType_ = renderer->GetRendererType();
                return;
            }
        }
        throw std::runtime_error("No renderer available.");
    }

    void ApplicationNodeImplementation::StartRenderScreen(enh::TexuturesRAII<5> textures)
    {
        baseRenderType_ = pcViewer::RenderType::SCREEN;
        (*screenTexturesPtr_) = std::move(textures);
    }

    void ApplicationNodeImplementation::DrawFrame(FrameBuffer& fbo)
    {
        // auto deferredFBO = SelectOffscreenBuffer(deferredFBOs_);
        // if (GetCameraEnh().IsFixed()) {
        //     fbo.SetStandardViewport(0, 0, viewportSize_.x, viewportSize_.y);
        //     for (auto& dfbo : deferredFBOs_) dfbo.SetStandardViewport(0, 0, viewportSize_.x, viewportSize_.y);
        // }
        // else {
        //     fbo.SetStandardViewport(0, 0, fbo.GetWidth(), fbo.GetHeight());
        //     for (auto& dfbo : deferredFBOs_) dfbo.SetStandardViewport(0, 0, fbo.GetWidth(), fbo.GetHeight());
        // }

        if (baseRenderType_ == pcViewer::RenderType::SCREEN) {
            DrawLoadedScreen(fbo);
        }
        else {
            if (!currentRenderers_) return;
            auto deferredFBO = SelectOffscreenBuffer(deferredFBOs_);
            (*currentRenderers_)[static_cast<std::size_t>(baseRenderType_)]->DrawPointCloud(fbo, *deferredFBO, false);
        }
        // sceneFBO->DrawToFBO([this]() {
        // });

        // dof_->ApplyEffect(*GetCamera(), sceneFBO->GetTextures()[0], sceneFBO->GetTextures()[2], sceneFBO, 1);
        // tm_->ApplyTonemapping(sceneFBO->GetTextures()[1], sceneFBO, 0);
        // bloom_->ApplyEffect(sceneFBO->GetTextures()[0], &fbo);

        // fbo.DrawToFBO([this]() {});
    }

    void ApplicationNodeImplementation::CleanUpInternal()
    {
        currentRenderers_ = nullptr;
        renderers_[0][0] = nullptr;
        renderers_[0][1] = nullptr;
        renderers_[0][2] = nullptr;
        renderers_[1][0] = nullptr;
        renderers_[1][1] = nullptr;
        renderers_[1][2] = nullptr;
        renderers_[2][0] = nullptr;
        renderers_[2][1] = nullptr;
        renderers_[2][2] = nullptr;

        // dof_ = nullptr;
        // tm_ = nullptr;
        // bloom_ = nullptr;
        // sceneFBOs_.clear();
        deferredFBOs_.clear();
    }

    bool ApplicationNodeImplementation::MouseButtonCallback(int button, int action)
    {
        camera_.HandleMouse(button, action, 0.0f, this);
        return true;
    }

    bool ApplicationNodeImplementation::MousePosCallback(double x, double y)
    {
        camera_.HandleMouse(-1, 0, 0.0f, this);
        return true;
    }

    bool ApplicationNodeImplementation::MouseScrollCallback(double xoffset, double yoffset)
    {
        camera_.HandleMouse(-1, 0, static_cast<float>(yoffset), this);
        return true;
    }

    bool ApplicationNodeImplementation::KeyboardCallback(int key, int scancode, int action, int mods)
    {
        if (key == GLFW_KEY_R) {
            GetGPUProgramManager().RecompileAll();
            return true;
        }
        return false;
    }

}
