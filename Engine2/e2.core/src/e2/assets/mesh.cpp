
#include "e2/assets/mesh.hpp"
#include "e2/assets/material.hpp"

#include "e2/managers/assetmanager.hpp"
#include "e2/managers/rendermanager.hpp"

#include "e2/rhi/rendercontext.hpp"


namespace
{
	//uint64_t numMeshes{};
}

e2::Mesh::Mesh()
{
	//::numMeshes++;
	//LogNotice("Num meshes increased to {}", ::numMeshes);
}

e2::Mesh::~Mesh()
{
	//::numMeshes--;
	//LogNotice("Num meshes decreased to {}", ::numMeshes);

	for (uint8_t i = 0; i < m_specifications.size(); i++)
	{
		e2::SubmeshSpecification &spec = m_specifications[i];
		for (uint8_t j = 0; j < spec.vertexAttributes.size(); j++)
		{
			spec.vertexAttributes[j]->keepAround();
			e2::discard(spec.vertexAttributes[j]);
		}
		spec.indexBuffer->keepAround();
		e2::discard(spec.indexBuffer);
	}
}

void e2::Mesh::write(Buffer& destination) const
{

}

bool e2::Mesh::read(Buffer& source)
{
	if (!e2::Asset::read(source))
		return false;

	uint32_t submeshCount{};
	source >> submeshCount;

	for (uint32_t i = 0; i < submeshCount; i++)
	{
		// fetch material directly from deps array using submesh index
		e2::MaterialPtr material = assetManager()->get(dependencies[i].uuid).cast<e2::Material>();
		m_materials.push(material);

		e2::SubmeshSpecification newSpecification;

		source >> newSpecification.vertexCount;
		source >> newSpecification.indexCount;

		uint8_t attributeFlags{};
		source >> attributeFlags;
		newSpecification.attributeFlags = (e2::VertexAttributeFlags)attributeFlags;

		newSpecification.vertexLayout = renderManager()->getOrCreateVertexLayout(newSpecification.attributeFlags);

		auto readIndexBuffer = [this, &source, &newSpecification]() -> void {
			uint32_t bufferSize{};
			source >> bufferSize;

			e2::DataBufferCreateInfo bufferCreateInfo{};
			bufferCreateInfo.dynamic = false;
			bufferCreateInfo.size = bufferSize;
			bufferCreateInfo.type = BufferType::IndexBuffer;
			e2::IDataBuffer* newBuffer = renderContext()->createDataBuffer(bufferCreateInfo);

			uint32_t* indexBuffer = reinterpret_cast<uint32_t*>(source.current());
			for (uint32_t ti = 0; ti < (newSpecification.indexCount / 3); ti++)
			{
				uint32_t a = indexBuffer[ti * 3 + 0];
				uint32_t b = indexBuffer[ti * 3 + 1];
				uint32_t c = indexBuffer[ti * 3 + 2];

				if (a >= newSpecification.vertexCount || b >= newSpecification.vertexCount || c >= newSpecification.vertexCount)
					LogError("corrupt data detected");
			}

			// upload data direct from source buffer, and then use consume to skip 
			newBuffer->upload(source.current(), bufferSize, 0, 0);
			uint64_t oldCursor{};
			source.consume(bufferSize, oldCursor);

			newSpecification.indexBuffer = newBuffer;
		};

		auto readVertexBuffer = [this, &source, &newSpecification]() -> void {
			uint32_t bufferSize{};
			source >> bufferSize;

			e2::DataBufferCreateInfo bufferCreateInfo{};
			bufferCreateInfo.dynamic = false;
			bufferCreateInfo.size = bufferSize;
			bufferCreateInfo.type = BufferType::VertexBuffer;
			e2::IDataBuffer* newBuffer = renderContext()->createDataBuffer(bufferCreateInfo);

			// upload data direct from source buffer, and then use consume to skip 
			newBuffer->upload(source.current(), bufferSize, 0, 0);
			uint64_t oldCursor{};
			source.consume(bufferSize, oldCursor);

			newSpecification.vertexAttributes.push(newBuffer);
		};

		readIndexBuffer();

		// Position 
		readVertexBuffer();

		if ((newSpecification.attributeFlags & e2::VertexAttributeFlags::Normal) == e2::VertexAttributeFlags::Normal)
		{
			readVertexBuffer();
			readVertexBuffer();
		}
		
		if ((newSpecification.attributeFlags & e2::VertexAttributeFlags::TexCoords01) == e2::VertexAttributeFlags::TexCoords01)
		{
			readVertexBuffer();
		}

		if ((newSpecification.attributeFlags & e2::VertexAttributeFlags::TexCoords23) == e2::VertexAttributeFlags::TexCoords23)
		{
			readVertexBuffer();
		}

		if ((newSpecification.attributeFlags & e2::VertexAttributeFlags::Color) == e2::VertexAttributeFlags::Color)
		{
			readVertexBuffer();
		}

		if ((newSpecification.attributeFlags & e2::VertexAttributeFlags::Bones) == e2::VertexAttributeFlags::Bones)
		{
			readVertexBuffer();
			readVertexBuffer();
		}

		m_specifications.push(newSpecification);

	}

	flagDone();

	return true;
}

e2::SubmeshSpecification const& e2::Mesh::specification(uint8_t subIndex)
{
	return m_specifications[subIndex];
}

e2::MaterialPtr e2::Mesh::material(uint8_t subIndex) const
{
	return m_materials[subIndex];
}

uint8_t e2::Mesh::submeshCount() const
{
	return (uint8_t)m_specifications.size();
}

void e2::Mesh::addProceduralSubmesh(e2::ProceduralSubmesh const& sourceData)
{
	if (m_done)
	{
		LogError("attempted to add procedural submesh, but mesh is flagged as done, refusing.");
		return;
	}


	e2::SubmeshSpecification newSpecification;

	newSpecification.vertexCount = sourceData.numVertices;
	newSpecification.indexCount = sourceData.numIndices;
	newSpecification.attributeFlags = sourceData.attributes;
	newSpecification.vertexLayout = renderManager()->getOrCreateVertexLayout(newSpecification.attributeFlags);

	e2::IDataBuffer* newBuffer{};
	e2::DataBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.dynamic = false;

	// Indices
	bufferCreateInfo.type = BufferType::IndexBuffer;
	bufferCreateInfo.size = sizeof(uint32_t) * sourceData.numIndices;
	newBuffer = renderContext()->createDataBuffer(bufferCreateInfo);
	newBuffer->upload(reinterpret_cast<uint8_t const*>(sourceData.sourceIndices), bufferCreateInfo.size, 0, 0);
	newSpecification.indexBuffer = newBuffer;

	// Positions
	bufferCreateInfo.type = BufferType::VertexBuffer;
	bufferCreateInfo.size = sizeof(glm::vec4) * sourceData.numVertices;
	newBuffer = renderContext()->createDataBuffer(bufferCreateInfo);
	newBuffer->upload(reinterpret_cast<uint8_t const*>(sourceData.sourcePositions), bufferCreateInfo.size, 0, 0);
	newSpecification.vertexAttributes.push(newBuffer);

	// Note: The order here matters, and depends on sourceData.attributes

	// Normals + Tangents
	if ((sourceData.attributes & e2::VertexAttributeFlags::Normal) == e2::VertexAttributeFlags::Normal)
	{
		bufferCreateInfo.size = sizeof(glm::vec4) * sourceData.numVertices;
		newBuffer = renderContext()->createDataBuffer(bufferCreateInfo);
		newBuffer->upload(reinterpret_cast<uint8_t const*>(sourceData.sourceNormals), bufferCreateInfo.size, 0, 0);
		newSpecification.vertexAttributes.push(newBuffer);

		bufferCreateInfo.size = sizeof(glm::vec4) * sourceData.numVertices;
		newBuffer = renderContext()->createDataBuffer(bufferCreateInfo);
		newBuffer->upload(reinterpret_cast<uint8_t const*>(sourceData.sourceTangents), bufferCreateInfo.size, 0, 0);
		newSpecification.vertexAttributes.push(newBuffer);
	}

	// Uv01
	if ((sourceData.attributes & e2::VertexAttributeFlags::TexCoords01) == e2::VertexAttributeFlags::TexCoords01)
	{
		bufferCreateInfo.size = sizeof(glm::vec4) * sourceData.numVertices;
		newBuffer = renderContext()->createDataBuffer(bufferCreateInfo);
		newBuffer->upload(reinterpret_cast<uint8_t const*>(sourceData.sourceUv01), bufferCreateInfo.size, 0, 0);
		newSpecification.vertexAttributes.push(newBuffer);
	}

	// Uv23
	if ((sourceData.attributes & e2::VertexAttributeFlags::TexCoords23) == e2::VertexAttributeFlags::TexCoords23)
	{
		bufferCreateInfo.size = sizeof(glm::vec4) * sourceData.numVertices;
		newBuffer = renderContext()->createDataBuffer(bufferCreateInfo);
		newBuffer->upload(reinterpret_cast<uint8_t const*>(sourceData.sourceUv23), bufferCreateInfo.size, 0, 0);
		newSpecification.vertexAttributes.push(newBuffer);
	}

	// Colors
	if ((sourceData.attributes & e2::VertexAttributeFlags::Color) == e2::VertexAttributeFlags::Color)
	{
		bufferCreateInfo.size = sizeof(glm::vec4) * sourceData.numVertices;
		newBuffer = renderContext()->createDataBuffer(bufferCreateInfo);
		newBuffer->upload(reinterpret_cast<uint8_t const*>(sourceData.sourceColors), bufferCreateInfo.size, 0, 0);
		newSpecification.vertexAttributes.push(newBuffer);
	}

	// Weights + Ids
	if ((sourceData.attributes & e2::VertexAttributeFlags::Bones) == e2::VertexAttributeFlags::Bones)
	{
		bufferCreateInfo.size = sizeof(glm::vec4) * sourceData.numVertices;
		newBuffer = renderContext()->createDataBuffer(bufferCreateInfo);
		newBuffer->upload(reinterpret_cast<uint8_t const*>(sourceData.sourceWeights), bufferCreateInfo.size, 0, 0);
		newSpecification.vertexAttributes.push(newBuffer);

		bufferCreateInfo.size = sizeof(glm::uvec4) * sourceData.numVertices;
		newBuffer = renderContext()->createDataBuffer(bufferCreateInfo);
		newBuffer->upload(reinterpret_cast<uint8_t const*>(sourceData.sourceBones), bufferCreateInfo.size, 0, 0);
		newSpecification.vertexAttributes.push(newBuffer);
	}

	m_specifications.push(newSpecification);
	m_materials.push(sourceData.material);
}

void e2::Mesh::flagDone()
{
	m_done = true;
}
