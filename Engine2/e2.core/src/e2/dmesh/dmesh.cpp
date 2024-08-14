
#include "e2/dmesh/dmesh.hpp"

#include "mikktspace.h"

#include <map>

e2::DynamicMesh::DynamicMesh()
{

}

e2::DynamicMesh::DynamicMesh(e2::MeshPtr mesh, uint8_t submeshIndex, e2::VertexAttributeFlags requestedFlags)
{
	e2::SubmeshSpecification const& spec = mesh->specification(submeshIndex);

	e2::VertexAttributeFlags existingFlags = spec.attributeFlags;
	e2::VertexAttributeFlags relevantFlags = existingFlags & requestedFlags;

	uint32_t indexCount = spec.indexCount;
	uint32_t vertCount = spec.vertexCount;
	uint32_t triCount = indexCount / 3;

	uint32_t* sourceIndices = new uint32_t[indexCount];
	spec.indexBuffer->download(reinterpret_cast<uint8_t*>(sourceIndices), sizeof(uint32_t) * indexCount, sizeof(uint32_t) * indexCount, 0);

	uint32_t attributeCounter = 0;
	glm::vec4* sourcePositions = new glm::vec4[vertCount];
	spec.vertexAttributes[attributeCounter++]->download(reinterpret_cast<uint8_t*>(sourcePositions), sizeof(glm::vec4) * vertCount, sizeof(glm::vec4) * vertCount, 0);

	glm::vec4* sourceNormals{};
	glm::vec4* sourceTangents{};
	if ((relevantFlags & e2::VertexAttributeFlags::Normal) == e2::VertexAttributeFlags::Normal)
	{
		sourceNormals = new glm::vec4[vertCount];
		spec.vertexAttributes[attributeCounter++]->download(reinterpret_cast<uint8_t*>(sourceNormals), sizeof(glm::vec4) * vertCount, sizeof(glm::vec4) * vertCount, 0);

		sourceTangents = new glm::vec4[vertCount];
		spec.vertexAttributes[attributeCounter++]->download(reinterpret_cast<uint8_t*>(sourceTangents), sizeof(glm::vec4) * vertCount, sizeof(glm::vec4) * vertCount, 0);
	}
	else if ((existingFlags & e2::VertexAttributeFlags::Normal) == e2::VertexAttributeFlags::Normal)
	{
		// If this attribute exists in the specification/mesh, but wasnt requested, we need to skip the counter
		attributeCounter++;
		attributeCounter++;
	}

	glm::vec4* sourceUv01{};
	if ((relevantFlags & e2::VertexAttributeFlags::TexCoords01) == e2::VertexAttributeFlags::TexCoords01)
	{
		sourceUv01 = new glm::vec4[vertCount];
		spec.vertexAttributes[attributeCounter++]->download(reinterpret_cast<uint8_t*>(sourceUv01), sizeof(glm::vec4) * vertCount, sizeof(glm::vec4) * vertCount, 0);
	}
	else if ((existingFlags & e2::VertexAttributeFlags::TexCoords01) == e2::VertexAttributeFlags::TexCoords01)
	{
		// If this attribute exists in the specification/mesh, but wasnt requested, we need to skip the counter
		attributeCounter++;
	}

	glm::vec4* sourceUv23{};
	if ((relevantFlags & e2::VertexAttributeFlags::TexCoords23) == e2::VertexAttributeFlags::TexCoords23)
	{
		sourceUv23 = new glm::vec4[vertCount];
		spec.vertexAttributes[attributeCounter++]->download(reinterpret_cast<uint8_t*>(sourceUv23), sizeof(glm::vec4) * vertCount, sizeof(glm::vec4) * vertCount, 0);
	}
	else if ((existingFlags & e2::VertexAttributeFlags::TexCoords23) == e2::VertexAttributeFlags::TexCoords23)
	{
		// If this attribute exists in the specification/mesh, but wasnt requested, we need to skip the counter
		attributeCounter++;
	}

	glm::vec4* sourceColors{};
	if ((relevantFlags & e2::VertexAttributeFlags::Color) == e2::VertexAttributeFlags::Color)
	{
		sourceColors = new glm::vec4[vertCount];
		spec.vertexAttributes[attributeCounter++]->download(reinterpret_cast<uint8_t*>(sourceColors), sizeof(glm::vec4) * vertCount, sizeof(glm::vec4) * vertCount, 0);
	}
	else if ((existingFlags & e2::VertexAttributeFlags::Color) == e2::VertexAttributeFlags::Color)
	{
		// If this attribute exists in the specification/mesh, but wasnt requested, we need to skip the counter
		attributeCounter++;
	}

	glm::vec4* sourceWeights{};
	glm::uvec4* sourceBones{};
	if ((relevantFlags & e2::VertexAttributeFlags::Bones) == e2::VertexAttributeFlags::Bones)
	{
		sourceWeights = new glm::vec4[vertCount];
		spec.vertexAttributes[attributeCounter++]->download(reinterpret_cast<uint8_t*>(sourceWeights), sizeof(glm::vec4) * vertCount, sizeof(glm::vec4) * vertCount, 0);

		sourceBones = new glm::uvec4[vertCount];
		spec.vertexAttributes[attributeCounter++]->download(reinterpret_cast<uint8_t*>(sourceBones), sizeof(glm::uvec4) * vertCount, sizeof(glm::uvec4) * vertCount, 0);
	}
	else if ((existingFlags & e2::VertexAttributeFlags::Bones) == e2::VertexAttributeFlags::Bones)
	{
		// If this attribute exists in the specification/mesh, but wasnt requested, we need to skip the counter
		attributeCounter++;
		attributeCounter++;
	}

	// mux vertexdata
	m_vertices.resize(vertCount, {});
	for (uint32_t vertexId = 0; vertexId < vertCount; vertexId++)
	{
		e2::Vertex& vertex = m_vertices[vertexId];

		vertex.position = sourcePositions[vertexId];

		if (sourceNormals)
			vertex.normal = sourceNormals[vertexId];

		if (sourceTangents)
			vertex.tangent = sourceTangents[vertexId];

		if (sourceUv01)
			vertex.uv01 = sourceUv01[vertexId];

		if (sourceUv23)
			vertex.uv23 = sourceUv23[vertexId];

		if (sourceColors)
			vertex.color = sourceColors[vertexId];

		if(sourceWeights)
			vertex.weights = sourceWeights[vertexId];

		if (sourceBones)
			vertex.bones = sourceBones[vertexId];
	}

	// indices to tris
	m_triangles.resize(triCount, {});
	for (uint32_t triangleId = 0; triangleId < triCount; triangleId++)
	{
		e2::Triangle& triangle = m_triangles[triangleId];
		triangle.a = sourceIndices[triangleId * 3];
		triangle.b = sourceIndices[triangleId * 3 + 1];
		triangle.c = sourceIndices[triangleId * 3 + 2];

		if (triangle.a >= vertCount || triangle.b >= vertCount || triangle.c >= vertCount)
			LogError("corrupt data detected");
	}

	if (sourceBones)
	{
		delete[] sourceBones;
	}

	if (sourceWeights)
	{
		delete[] sourceWeights;
	}

	if (sourceColors)
	{
		delete[] sourceColors;
	}

	if (sourceUv23)
	{
		delete[] sourceUv23;
	}

	if (sourceUv01)
	{
		delete[] sourceUv01;
	}

	if (sourceTangents)
	{
		delete[] sourceTangents;
	}

	if (sourceNormals)
	{
		delete[] sourceNormals;
	}

	delete[] sourcePositions;
	delete[] sourceIndices;
	
}

e2::DynamicMesh::~DynamicMesh()
{

}

e2::MeshPtr e2::DynamicMesh::bake(e2::MaterialPtr material, e2::VertexAttributeFlags attributeFlags)
{
	e2::ProceduralSubmesh submesh;
	submesh.material = material;
	submesh.attributes = attributeFlags;
	submesh.numIndices = (uint32_t)m_triangles.size() * 3;
	submesh.numVertices = (uint32_t)m_vertices.size();

	submesh.sourceIndices = new uint32_t[submesh.numIndices];
	uint32_t indexCounter = 0;
	for (e2::Triangle const& triangle : m_triangles)
	{
		submesh.sourceIndices[indexCounter++] = triangle.a;
		submesh.sourceIndices[indexCounter++] = triangle.b;
		submesh.sourceIndices[indexCounter++] = triangle.c;
	}

	submesh.sourcePositions = new glm::vec4[submesh.numVertices];

	bool hasNormals = (attributeFlags & e2::VertexAttributeFlags::Normal) == e2::VertexAttributeFlags::Normal;
	if (hasNormals)
	{
		submesh.sourceNormals = new glm::vec4[submesh.numVertices];
		submesh.sourceTangents = new glm::vec4[submesh.numVertices];
	}

	bool hasUv01 = (attributeFlags & e2::VertexAttributeFlags::TexCoords01) == e2::VertexAttributeFlags::TexCoords01;
	if (hasUv01)
	{
		submesh.sourceUv01 = new glm::vec4[submesh.numVertices];
	}

	bool hasUv23 = (attributeFlags & e2::VertexAttributeFlags::TexCoords23) == e2::VertexAttributeFlags::TexCoords23;
	if (hasUv23)
	{
		submesh.sourceUv23 = new glm::vec4[submesh.numVertices];
	}

	bool hasColor = (attributeFlags & e2::VertexAttributeFlags::Color) == e2::VertexAttributeFlags::Color;
	if (hasColor)
	{
		submesh.sourceColors = new glm::vec4[submesh.numVertices];
	}

	bool hasWeights = (attributeFlags & e2::VertexAttributeFlags::Bones) == e2::VertexAttributeFlags::Bones;
	if (hasWeights)
	{
		submesh.sourceWeights = new glm::vec4[submesh.numVertices];
		submesh.sourceBones = new glm::uvec4[submesh.numVertices];
	}


	uint32_t vertexCounter = 0;
	for (e2::Vertex const& vertex : m_vertices)
	{
		submesh.sourcePositions[vertexCounter] = glm::vec4(vertex.position, 1.0f);

		if (hasNormals)
		{
			submesh.sourceNormals[vertexCounter] = glm::vec4(vertex.normal, 0.0f);
			submesh.sourceTangents[vertexCounter] = glm::vec4(vertex.tangent, vertex.tangentSign);
		}

		if (hasUv01)
		{
			submesh.sourceUv01[vertexCounter] = vertex.uv01;
		}

		if (hasUv23)
		{
			submesh.sourceUv23[vertexCounter] = vertex.uv23;
		}

		if (hasColor)
		{
			submesh.sourceColors[vertexCounter] = vertex.color;
		}

		if (hasWeights)
		{
			submesh.sourceWeights[vertexCounter] = vertex.weights;
			submesh.sourceBones[vertexCounter] = vertex.bones;
		}

		vertexCounter++;
	}


	e2::MeshPtr newMesh = e2::MeshPtr::create();
	newMesh->postConstruct(material.get(), e2::UUID());
	newMesh->addProceduralSubmesh(submesh);
	newMesh->flagDone();

	if (submesh.sourceBones)
	{
		delete[] submesh.sourceBones;
	}

	if (submesh.sourceWeights)
	{
		delete[] submesh.sourceWeights;
	}

	if (submesh.sourceColors)
	{
		delete[] submesh.sourceColors;
	}

	if (submesh.sourceUv23)
	{
		delete[] submesh.sourceUv23;
	}

	if (submesh.sourceUv01)
	{
		delete[] submesh.sourceUv01;
	}

	if (submesh.sourceTangents)
	{
		delete[] submesh.sourceTangents;
	}

	if (submesh.sourceNormals)
	{
		delete[] submesh.sourceNormals;
	}

	delete[] submesh.sourcePositions;
	delete[] submesh.sourceIndices;

	return newMesh;
}

void e2::DynamicMesh::reserve(uint32_t numVertices, uint32_t numTriangles)
{
	m_vertices.reserve(numVertices);
	m_triangles.reserve(numTriangles);
}

void e2::DynamicMesh::cutSlack()
{
	//m_vertices.resize(m_vertexOffset);
	//m_triangles.resize(m_triangleOffset);
}

using _Lookup = std::map<std::pair<uint32_t, uint32_t>, uint32_t>;
using _VertexList = std::vector<e2::Vertex>;
using _TriangleList = std::vector<e2::Triangle>;


namespace ico
{
	const float X = .525731112119133606f;
	const float Z = .850650808352039932f;
	const float N = 0.f;

	static const _VertexList vertices =
	{
	  {{-X,N,Z}}, {{X,N,Z}}, {{-X,N,-Z}}, {{X,N,-Z}},
	  {{N,Z,X}}, {{N,Z,-X}}, {{N,-Z,X}}, {{N,-Z,-X}},
	  {{Z,X,N}}, {{-Z,X, N}}, {{Z,-X,N}}, {{-Z,-X, N}}
	};

	static const _TriangleList triangles =
	{
	  {0,4,1},{0,9,4},{9,5,4},{4,5,8},{4,8,1},
	  {8,10,1},{8,3,10},{5,3,8},{5,2,3},{2,7,3},
	  {7,10,3},{7,6,10},{7,11,6},{11,0,6},{0,1,6},
	  {6,1,10},{9,0,11},{9,11,2},{9,2,5},{7,2,11}
	};
}


uint32_t _vertexForEdge(_Lookup& lookup, _VertexList& vertices, uint32_t first, uint32_t second)
{
	_Lookup::key_type key(first, second);
	if (key.first > key.second)
		std::swap(key.first, key.second);

	auto inserted = lookup.insert({ key, vertices.size() });
	if (inserted.second)
	{
		auto& edge0 = vertices[first];
		auto& edge1 = vertices[second];
		auto point = glm::normalize(edge0.position + edge1.position);
		vertices.push_back({ point });
	}

	return inserted.first->second;
}



_TriangleList subdivide(_VertexList& vertices, _TriangleList triangles)
{
	_Lookup lookup;
	_TriangleList result;

	for (auto&& each : triangles)
	{
		std::array<uint32_t, 3> mid;
		for (int edge = 0; edge < 3; ++edge)
		{
			mid[edge] = _vertexForEdge(lookup, vertices, each.vertexIds[edge], each.vertexIds[(edge + 1) % 3]);
		}

		result.push_back({ each.vertexIds[0], mid[0], mid[2] });
		result.push_back({ each.vertexIds[1], mid[1], mid[0] });
		result.push_back({ each.vertexIds[2], mid[2], mid[1] });
		result.push_back({ mid[0], mid[1], mid[2] });
	}

	return result;
}



void e2::DynamicMesh::buildIcoSphere(uint32_t subdivisions, bool flipWinding, glm::mat4 const& transform /*= glm::identity<glm::mat4>()*/)
{
	glm::mat3 normalMatrix = glm::transpose(glm::inverse(transform));

	_VertexList verts = ico::vertices;
	_TriangleList tris = ico::triangles;

	for (uint32_t i = 0; i < subdivisions; ++i)
	{
		tris = subdivide(verts, tris);
	}

	for (auto& v : verts)
	{
		e2::Vertex newVertex = v;

		newVertex.position = transform * glm::vec4(newVertex.position, 1.0f);
		newVertex.normal = normalMatrix * glm::vec4(newVertex.normal, 0.0f);
		newVertex.tangent = normalMatrix * glm::vec4(newVertex.tangent, 0.0f);
		m_vertices.push_back(newVertex);
	}

	if (flipWinding)
	{
		for (auto& t : tris)
		{
			e2::Triangle newTriangle = t;
			newTriangle.flipWinding();
			m_triangles.push_back(newTriangle);
		}
	}
	else
	{
		m_triangles = tris;
	}

	mergeDuplicateVertices();

	calculateFaceNormals();
	calculateVertexNormals();
	calculateVertexTangents();
}

void e2::DynamicMesh::addMesh(e2::DynamicMesh const* other, glm::mat4 const& transform)
{
	glm::mat3 normalMatrix = glm::transpose(glm::inverse(transform));

	uint32_t vertexIndexOffset = (uint32_t)m_vertices.size();

	for (e2::Triangle const& otherTriangle : other->m_triangles)
	{
		e2::Triangle newTriangle = otherTriangle;

		newTriangle.a += vertexIndexOffset;
		newTriangle.b += vertexIndexOffset;
		newTriangle.c += vertexIndexOffset;

		m_triangles.push_back(newTriangle);
	}

	for (e2::Vertex const& otherVertex : other->m_vertices)
	{
		e2::Vertex newVertex = otherVertex;

		newVertex.position = transform * glm::vec4(newVertex.position, 1.0f);
		newVertex.normal = normalMatrix * glm::vec4(newVertex.normal, 0.0f);
		newVertex.tangent = normalMatrix * glm::vec4(newVertex.tangent, 0.0f);

		m_vertices.push_back(newVertex);
	}
}

void e2::DynamicMesh::addMeshWithShaderFunction(e2::DynamicMesh const* other, glm::mat4 const& transform, VertexShaderFunction shaderFunc, void* shaderFuncData)
{
	glm::mat3 normalMatrix = glm::transpose(glm::inverse(transform));

	size_t vertexIndexOffset = m_vertices.size();
	size_t triangleIndexOffset = m_triangles.size();

	m_vertices.resize(size_t(vertexIndexOffset) + other->m_vertices.size());
	m_triangles.resize(size_t(triangleIndexOffset) + other->m_triangles.size());
	

	size_t i = triangleIndexOffset;
	for (e2::Triangle const& otherTriangle : other->m_triangles)
	{
		m_triangles[i].a = otherTriangle.a + (uint32_t)vertexIndexOffset;
		m_triangles[i].b = otherTriangle.b + (uint32_t)vertexIndexOffset;
		m_triangles[i].c = otherTriangle.c + (uint32_t)vertexIndexOffset;

		i++;
	}

	i = vertexIndexOffset;
	for (e2::Vertex const& otherVertex : other->m_vertices)
	{
		m_vertices[i] = otherVertex;

		m_vertices[i].position = transform * glm::vec4(m_vertices[i].position, 1.0f);
		m_vertices[i].normal = normalMatrix * glm::vec4(m_vertices[i].normal, 0.0f);
		m_vertices[i].tangent = normalMatrix * glm::vec4(m_vertices[i].tangent, 0.0f);

		shaderFunc(&m_vertices[i], shaderFuncData);

		i++;
	}
}

uint32_t e2::DynamicMesh::addVertex(Vertex const& newVertex)
{
	m_vertices.push_back(newVertex);
	return (uint32_t)m_vertices.size() - 1;
}

uint32_t e2::DynamicMesh::addTriangle(Vertex const& vertA, Vertex const& vertB, Vertex const& vertC)
{
	uint32_t a = addVertex(vertA);
	uint32_t b = addVertex(vertB);
	uint32_t c = addVertex(vertC);

	Triangle newTriangle;
	newTriangle.a = a;
	newTriangle.b = b;
	newTriangle.c = c;

	return addTriangle(newTriangle);
}

uint32_t e2::DynamicMesh::addTriangle(Triangle const& newTriangle)
{
	m_triangles.push_back(newTriangle);
	return (uint32_t)m_triangles.size() - 1;
}

void e2::DynamicMesh::swap(DynamicMesh& rhs)
{
	m_vertices.swap(rhs.m_vertices);
	m_triangles.swap(rhs.m_triangles);
}

void e2::DynamicMesh::calculateFaceNormals()
{
	for (uint32_t triangleId = 0; triangleId < m_triangles.size(); triangleId++)
	{
		Triangle& triangle = m_triangles[triangleId];

		if (triangle.a >= m_vertices.size() || triangle.b >= m_vertices.size() || triangle.c >= m_vertices.size())
		{
			LogError("Vertex index out of bounds, refusing to calculate face normal");
		}

		Vertex& c = m_vertices[triangle.a];
		Vertex& b = m_vertices[triangle.b];
		Vertex& a = m_vertices[triangle.c];

		glm::vec3 u = glm::normalize(b.position - a.position);
		glm::vec3 v = glm::normalize(c.position - a.position);

		// normalize here too for good measure
		triangle.faceNormal = glm::normalize(glm::cross(u, v));
	}
}

void e2::DynamicMesh::calculateVertexNormals()
{
	// Clear normals and tangents
	for (uint32_t vertexId = 0; vertexId < m_vertices.size(); vertexId++)
	{
		Vertex& vertex = m_vertices[vertexId];
		vertex.normal = {};
		vertex.tangent = {};
	}

	// Add face contributions to normals and tangents
	for (uint32_t triangleId = 0; triangleId < m_triangles.size(); triangleId++)
	{
		Triangle& triangle = m_triangles[triangleId];
		Vertex& a = m_vertices[triangle.a];
		Vertex& b = m_vertices[triangle.b];
		Vertex& c = m_vertices[triangle.c];

		a.normal += triangle.faceNormal;
		b.normal += triangle.faceNormal;
		c.normal += triangle.faceNormal;
	}

	// Normalize
	for (uint32_t vertexId = 0; vertexId < m_vertices.size(); vertexId++)
	{
		Vertex& vertex = m_vertices[vertexId];
		vertex.normal = glm::normalize(vertex.normal);
		vertex.tangent = glm::normalize(vertex.tangent);
	}
}

/** MikkTSpace integration */
namespace
{
	int getNumFaces(const SMikkTSpaceContext* pContext)
	{
		e2::DynamicMesh *mesh = reinterpret_cast<e2::DynamicMesh*>(pContext->m_pUserData);
		return mesh->numTriangles();
	}

	int getNumVerticesOfFace(const SMikkTSpaceContext* pContext, const int iFace)
	{
		return 3;
	}

	void getPosition(const SMikkTSpaceContext* pContext, float fvPosOut[], const int iFace, const int iVert)
	{
		e2::DynamicMesh* mesh = reinterpret_cast<e2::DynamicMesh*>(pContext->m_pUserData);
		e2::Triangle &triangle = mesh->getTriangle(iFace);
		uint32_t vertexId = triangle.vertexIds[iVert];
		glm::vec3 pos = mesh->getVertex(vertexId).position;
		fvPosOut[0] = pos.x;
		fvPosOut[1] = pos.y;
		fvPosOut[2] = pos.z;

	}

	void getNormal(const SMikkTSpaceContext* pContext, float fvNormOut[], const int iFace, const int iVert)
	{
		e2::DynamicMesh* mesh = reinterpret_cast<e2::DynamicMesh*>(pContext->m_pUserData);
		e2::Triangle& triangle = mesh->getTriangle(iFace);
		uint32_t vertexId = triangle.vertexIds[iVert];
		glm::vec3 nrm = mesh->getVertex(vertexId).normal;
		fvNormOut[0] = nrm.x;
		fvNormOut[1] = nrm.y;
		fvNormOut[2] = nrm.z;
	}

	void getTexCoord(const SMikkTSpaceContext* pContext, float fvTexcOut[], const int iFace, const int iVert)
	{
		e2::DynamicMesh* mesh = reinterpret_cast<e2::DynamicMesh*>(pContext->m_pUserData);
		e2::Triangle& triangle = mesh->getTriangle(iFace);
		uint32_t vertexId = triangle.vertexIds[iVert];
		glm::vec4 uv01 = mesh->getVertex(vertexId).uv01;
		fvTexcOut[0] = uv01.x;
		fvTexcOut[1] = uv01.y;
	}

	void setTSpaceBasic(const SMikkTSpaceContext* pContext, const float fvTangent[], const float fSign, const int iFace, const int iVert)
	{
		e2::DynamicMesh* mesh = reinterpret_cast<e2::DynamicMesh*>(pContext->m_pUserData);
		e2::Triangle& triangle = mesh->getTriangle(iFace);
		uint32_t vertexId = triangle.vertexIds[iVert];
		e2::Vertex& vertex = mesh->getVertex(vertexId);
		vertex.tangent.x = fvTangent[0];
		vertex.tangent.y = fvTangent[1];
		vertex.tangent.z = fvTangent[2];
		vertex.tangentSign = fSign;
	}

}



bool e2::DynamicMesh::calculateVertexTangents()
{
	// Integrate MikkTSpace (https://github.com/mmikk/MikkTSpace)
	SMikkTSpaceInterface mikkIf{};
	mikkIf.m_getNumFaces = ::getNumFaces;
	mikkIf.m_getNumVerticesOfFace = ::getNumVerticesOfFace;
	mikkIf.m_getPosition = ::getPosition;
	mikkIf.m_getNormal = ::getNormal;
	mikkIf.m_getTexCoord = ::getTexCoord;
	mikkIf.m_setTSpaceBasic = ::setTSpaceBasic;
	mikkIf.m_setTSpace = nullptr;

	SMikkTSpaceContext mikkCtx;
	mikkCtx.m_pInterface = &mikkIf;
	mikkCtx.m_pUserData = reinterpret_cast<void*>(this);

	// guess this is how its supposed to be done ¯\_(ツ)_/¯
	if (!genTangSpaceDefault(&mikkCtx))
	{
		LogError("mikktspace failed");
		return false;
	}

	return true;
}

void e2::DynamicMesh::mergeDuplicateVertices()
{
	e2::DynamicMesh newMesh;
	newMesh.reserve((uint32_t)m_vertices.size(), (uint32_t)m_triangles.size());

	// maps old vertex ids to new vertex ids 
	//std::unordered_map<uint32_t, uint32_t> vertexIdMap;
	std::vector<uint32_t> vertexIdMap;
	vertexIdMap.resize(m_vertices.size());

	{
		std::map<Vertex, uint32_t> existingVertices;

		// create unique vertices, and map ids
		for (uint32_t id = 0; id < m_vertices.size(); id++)
		{
			Vertex& vertex = m_vertices[id];
			auto existingIter = existingVertices.find(vertex);
			if (existingIter == existingVertices.end())
			{
				uint32_t newId = newMesh.addVertex(vertex);
				vertexIdMap[id] = newId;
				existingVertices[vertex] = newId;
			}
			else
			{
				vertexIdMap[id] = existingVertices.at(vertex);
			}
		}
	}

	// recreate old triangles with the new ids
	for (uint32_t id = 0; id < m_triangles.size(); id++)
	{
		Triangle& oldTriangle = m_triangles[id];

		Triangle newTriangle;
		newTriangle.a = vertexIdMap[oldTriangle.a];
		newTriangle.b = vertexIdMap[oldTriangle.b];
		newTriangle.c = vertexIdMap[oldTriangle.c];

		newMesh.addTriangle(newTriangle);
	}

	// swap the new data for the old
	swap(newMesh);
}

 glm::vec3 e2::snapToSteps(glm::vec3 const& in, float stepNumerator, float stepDenominator)
{
	return glm::round(in * stepDenominator / stepNumerator) * stepNumerator / stepDenominator;
}
