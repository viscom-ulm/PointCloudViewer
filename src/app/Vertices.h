/**
 * @file   Vertices.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.30
 *
 * @brief  Defines all vertices used in the application.
 */

#pragma once

#include <glm/glm.hpp>

namespace viscom {

    class GPUProgram;
    class Mesh;

    struct GridVertex
    {
        glm::vec3 position_;
        glm::vec4 color_;

        GridVertex(const glm::vec3& pos, const glm::vec4& col) : position_(pos), color_(col) {}
    };

    struct SimpleMeshVertex
    {
        glm::vec3 position_;
        glm::vec3 normal_;
        glm::vec2 texCoords_;

        SimpleMeshVertex() : position_(0.0f), normal_(0.0f), texCoords_(0.0f) {}
        SimpleMeshVertex(const glm::vec3& pos, const glm::vec3& normal, const glm::vec2& tex) : position_(pos), normal_(normal), texCoords_(tex) {}
        static void SetVertexAttributes(const GPUProgram* program)
        {
            auto attribLoc = program->GetAttributeLocations({ "position", "normal", "texCoords" });

            if (attribLoc[0] != -1) {
                gl::glEnableVertexAttribArray(attribLoc[0]);
                gl::glVertexAttribPointer(attribLoc[0], 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(SimpleMeshVertex), reinterpret_cast<GLvoid*>(offsetof(SimpleMeshVertex, position_)));
            }
            if (attribLoc[1] != -1) {
                gl::glEnableVertexAttribArray(attribLoc[1]);
                gl::glVertexAttribPointer(attribLoc[1], 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(SimpleMeshVertex), reinterpret_cast<GLvoid*>(offsetof(SimpleMeshVertex, normal_)));
            }
            if (attribLoc[2] != -1) {
                gl::glEnableVertexAttribArray(attribLoc[2]);
                gl::glVertexAttribPointer(attribLoc[2], 2, gl::GL_FLOAT, gl::GL_FALSE, sizeof(SimpleMeshVertex), reinterpret_cast<GLvoid*>(offsetof(SimpleMeshVertex, texCoords_)));
            }
        }

        static GLuint CreateVertexBuffer(const Mesh* mesh)
        {
            GLuint vbo = 0;
            gl::glGenBuffers(1, &vbo);
            std::vector<SimpleMeshVertex> bufferMem(mesh->GetVertices().size());
            for (std::size_t i = 0; i < mesh->GetVertices().size(); ++i) {
                bufferMem[i].position_ = mesh->GetVertices()[i];
                bufferMem[i].normal_ = mesh->GetNormals()[i];
                bufferMem[i].texCoords_ = glm::vec2(mesh->GetTexCoords(0)[i]);
            }
            gl::glBindBuffer(gl::GL_ARRAY_BUFFER, vbo);
            gl::glBufferData(gl::GL_ARRAY_BUFFER, bufferMem.size() * sizeof(SimpleMeshVertex), bufferMem.data(), gl::GL_STATIC_DRAW);
            gl::glBindBuffer(gl::GL_ARRAY_BUFFER, 0);
            return vbo;
        }
    };
}
