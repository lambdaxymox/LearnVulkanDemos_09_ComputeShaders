#include "shaders_hlsl.h"

#include "shaders_hlsl.h.in"


std::unordered_map<std::string, std::vector<uint8_t>> shaders_hlsl::createHlslShaders() {
    const auto shaders = std::unordered_map<std::string, std::vector<uint8_t>> {
        { shader_compute_comp_hlsl, std::vector<uint8_t> { shader_compute_comp_hlsl_spv.begin(), shader_compute_comp_hlsl_spv.end() } },
        { shader_compute_frag_hlsl, std::vector<uint8_t> { shader_compute_frag_hlsl_spv.begin(), shader_compute_frag_hlsl_spv.end() } },
        { shader_compute_vert_hlsl, std::vector<uint8_t> { shader_compute_vert_hlsl_spv.begin(), shader_compute_vert_hlsl_spv.end() } },
    };

    return shaders;
}

