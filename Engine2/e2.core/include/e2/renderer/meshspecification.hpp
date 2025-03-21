
#pragma once 

#include <e2/export.hpp>
#include <e2/utils.hpp>
#include <e2/rhi/enums.hpp>
#include <e2/rhi/databuffer.hpp>
#include <e2/rhi/shader.hpp>

#include <cstdint> 
#include <vector>

namespace e2 
{


     /**
      * This is not really a build configuration, so kept outside of buildcfg.hpp.
      * This specifies how many actual GLSL vertex attributes/vertexbuffers we have
      */
    constexpr uint8_t maxNumSpecificationAttributes = 8;

    /** How many possible permutations we have given VertexAttributeFlags */
    constexpr uint8_t maxNumAttributePermutations = 32; // 2^maxNumSpecificationAttributes

    /** Order of data follows this order */
    enum class VertexAttributeFlags : uint8_t
    {
        None = 0, // Position always implied, so regardless of flags we always have vec4 
        Normal = 1 << 0, // Normal implies normals AND tangents (bitangent always derived in shader), vec4 + vec4
        TexCoords01 = 1 << 1, // Has at least 1 texcoords, vec4
        TexCoords23 = 1 << 2, // Has at least 3 texcoords, vec4
        Color = 1 << 3, // Has vertex colors, vec4
        Bones = 1 << 4, // Has bone weights and ids , vec4 + uvec4

        All = Normal | TexCoords01 | TexCoords23 | Color | Bones
    };

    E2_API void applyVertexAttributeDefines(e2::VertexAttributeFlags flags, e2::ShaderCreateInfo& outInfo);

    class IVertexLayout;

    /** Specification for mesh data for a single submesh */
    struct E2_API SubmeshSpecification
    {
        // non-owning, cached via attribute flags in rendermanager
        e2::IVertexLayout* vertexLayout{};

        e2::VertexAttributeFlags attributeFlags {  };

        // predictably ordered attribute buffers, based on attribute flags and layout
        // optimized to be used directly in renderer
        e2::StackVector<e2::IDataBuffer*, e2::maxNumSpecificationAttributes> vertexAttributes;

        e2::IDataBuffer* indexBuffer{};

        uint32_t indexCount{};
        uint32_t vertexCount{};


    };


    EnumFlagsDeclaration(e2::VertexAttributeFlags);
}

