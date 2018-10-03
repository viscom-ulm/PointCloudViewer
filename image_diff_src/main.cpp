/**
 * @file   main.cpp
 * @author %VA_USERNAME% <%VA_USER_EMAIL%>
 * @date   2018.10.03
 *
 * @brief  
 */

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
#include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
#include <vector>
#include <glm/vec4.hpp>

int main(int argc, char** argv)
{
    if (argc < 4) {
        std::cout << "Program wants 3 parameters (file A, file B, output Filename for A-B)" << std::endl;
        return -1;
    }

    int widthA, widthB, heightA, heightB, channelsA, channelsB;
    auto imgA = stbi_load(argv[1], &widthA, &heightA, &channelsA, 4);
    auto imgB = stbi_load(argv[2], &widthB, &heightB, &channelsB, 4);

    if (widthA != widthB || heightA != heightB || channelsA != channelsB) {
        std::cout << "Images are not compatible to each other." << std::endl;
    }

    using color4 = glm::tvec4<std::uint8_t, glm::precision::highp>;
    std::vector<color4> imgDataA, imgDataB, imgDataAB;
    imgDataA.resize(widthA * static_cast<std::size_t>(heightA));
    imgDataB.resize(widthB * static_cast<std::size_t>(heightB));
    imgDataAB.resize(widthA * static_cast<std::size_t>(heightA));
    memcpy(imgDataA.data(), imgA, widthA * static_cast<std::size_t>(heightA) * sizeof(color4));
    memcpy(imgDataB.data(), imgB, widthA * static_cast<std::size_t>(heightA) * sizeof(color4));

    for (std::size_t y = 0; y < heightA; ++y) {
        for (std::size_t x = 0; x < widthA; ++x) {
            imgDataAB[x + y * widthA] = imgDataA[x + y * widthA] - imgDataB[x + y * widthA];
            if (imgDataA[x + y * widthA].a != 0) imgDataAB[x + y * widthA].a = 255;
        }
    }

    stbi_write_png(argv[3], widthA, heightA, 4, imgDataAB.data(), static_cast<std::size_t>(widthA) * sizeof(color4));

    return 0;
}