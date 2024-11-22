
#include "game/hex.hpp"
#include "e2/e2.hpp"
#include "e2/game/gamesession.hpp"
#include "e2/managers/asyncmanager.hpp"
#include "e2/managers/uimanager.hpp"
#include "e2/managers/audiomanager.hpp"
#include "e2/transform.hpp"
#include "game/entities/turnbasedentity.hpp"
#include "game/game.hpp"

#include <glm/gtc/noise.hpp>

#include <glm/gtx/easing.hpp>


glm::vec2 e2::TreeState::localOffset(e2::TileData *tile, ForestState* forestState)
{

	glm::mat4 forestRotation = glm::rotate(glm::identity<glm::mat4>(), tile->forestRotation, e2::worldUpf());

	glm::mat4 treeTranslation = glm::translate(glm::identity<glm::mat4>(), glm::vec3(planarOffset.x, 0.0f, planarOffset.y));
	

	glm::mat4 treeTransform = forestRotation * treeTranslation;

	glm::vec4 localOffset = treeTransform * glm::vec4(0.0, 0.0f, 0.0, 1.0f);
	return glm::vec2(localOffset.x, localOffset.z);
}


namespace
{
	void grassShader(e2::Vertex* vertex, void* shaderFuncData)
	{
		glm::vec2* offset = reinterpret_cast<glm::vec2*>(shaderFuncData);
		vertex->uv01.x = offset->x;
		vertex->uv01.y = offset->y;
	}

	void treeShader(e2::Vertex* vertex, void* shaderFuncData)
	{
		glm::vec2* offset = reinterpret_cast<glm::vec2*>(shaderFuncData);
		vertex->uv23.x = offset->x;
		vertex->uv23.y = offset->y;
	}
}

void e2::HexGrid::rebuildForestMeshes()
{

	constexpr float minDist = 0.15f;

	uint32_t forestNumTrees[3] = { e2::treeNum1, e2::treeNum2, e2::treeNum3 };

	for (uint32_t f = 0; f < 3; f++)
	{
		uint32_t numTrees = forestNumTrees[f];
		ForestState& forestState = m_forestStates[f];

		forestState.trees.clear();
		
		e2::DynamicMesh combinedMesh;

		for (uint32_t i = 0; i < numTrees; i++)
		{
			e2::TreeState treeState;
			treeState.meshIndex = (uint32_t)e2::randomInt(0, 3);
			float closestDist;
			do
			{
				closestDist = 1000000.0f;
				treeState.planarOffset = e2::randomInUnitCircle() * e2::treeSpread;
				for (e2::TreeState& t : forestState.trees)
				{
					float dist = glm::distance(treeState.planarOffset, t.planarOffset);
					if (dist < closestDist)
						closestDist = dist;

					if (closestDist < minDist)
						break;
				}

			} while (closestDist < minDist);




			treeState.rotation = glm::radians(e2::randomFloat(0.0f, 359.9999f));
			treeState.scale = e2::randomFloat(0.9f, 1.1f) * e2::treeScale;
			forestState.trees.push_back(treeState);

			glm::mat4 treeTranslation = glm::translate(glm::identity<glm::mat4>(), glm::vec3(treeState.planarOffset.x, 0.0f, treeState.planarOffset.y));
			glm::mat4 treeScale = glm::scale(glm::identity<glm::mat4>(), glm::vec3(treeState.scale));
			glm::mat4 treeRotation = glm::rotate(glm::identity<glm::mat4>(), treeState.rotation, e2::worldUpf());

			glm::mat4 treeRotationFix = glm::rotate(glm::identity<glm::mat4>(), glm::radians(90.0f), e2::worldRightf());

			glm::mat4 treeTransform = treeTranslation * treeRotation * treeRotationFix * treeScale;

			glm::vec4 offset4 =  treeTransform* glm::vec4(0.0, 0.0, 0.0, 1.0f);
			glm::vec2 offset{ offset4.x, offset4.z };

			combinedMesh.addMeshWithShaderFunction(&m_dynamicTreeMeshes[treeState.meshIndex], treeTransform, ::treeShader, &offset);
			//combinedMesh.addMesh(&m_dynamicTreeMeshes[treeState.meshIndex], treeTransform);
		}

		forestState.mesh = combinedMesh.bake(m_treeMaterial, VertexAttributeFlags::All);
	}

	


}


void e2::HexGrid::prescribeAssets(e2::Context* ctx, e2::ALJDescription& desc)
{
	e2::AssetManager* am = ctx->assetManager();
	am->prescribeALJ(desc, "S_Wood_Chop.e2a");
	am->prescribeALJ(desc, "S_Wood_Fall.e2a");
	am->prescribeALJ(desc, "S_Wood_Spawn.e2a");
	am->prescribeALJ(desc, "M_Tree.e2a");
	am->prescribeALJ(desc, "SM_Tree0.e2a");
	am->prescribeALJ(desc, "SM_Tree1.e2a");
	am->prescribeALJ(desc, "SM_Tree2.e2a");
	am->prescribeALJ(desc, "SM_Tree3.e2a");
	am->prescribeALJ(desc, "SM_Stone.e2a");
	am->prescribeALJ(desc, "T_Minimap_Water.e2a");
	am->prescribeALJ(desc, "T_Minimap_Forest.e2a");
	am->prescribeALJ(desc, "T_Minimap_Mountains.e2a");
	am->prescribeALJ(desc, "SM_Hex.e2a");
	am->prescribeALJ(desc, "SM_HexHigh.e2a");
	am->prescribeALJ(desc, "S_Wood_Die.e2a");
	am->prescribeALJ(desc, "M_Grass.e2a");
}


e2::HexGrid::HexGrid(e2::GameContext* gameCtx)
	: m_game(gameCtx->game())
{
	constexpr size_t prewarmSize = 16'384;
	m_tiles.reserve(prewarmSize);
	m_tileIndex.reserve(prewarmSize);

	// max num of threads to use for chunk streaming
	m_numThreads = std::thread::hardware_concurrency();
	if (m_numThreads > 1)
	{
		if (m_numThreads <= 4)
			m_numThreads = 2;
		else if (m_numThreads <= 6)
			m_numThreads = 4;
		else if (m_numThreads <= 8)
			m_numThreads = 6;
		else if (m_numThreads <= 12)
			m_numThreads = 10;
		else if (m_numThreads <= 16)
			m_numThreads = 14;
		else
			m_numThreads = 16;
	}


	auto am = game()->assetManager();

	m_treeChopSound = am->get("S_Wood_Chop.e2a")->cast<e2::Sound>();
	m_treeFallSound = am->get("S_Wood_Fall.e2a")->cast<e2::Sound>();
	m_woodDieSound = am->get("S_Wood_Die.e2a")->cast<e2::Sound>();
		

	m_treeMeshes[0] = am->get("SM_Tree0.e2a")->cast<e2::Mesh>();
	m_treeMeshes[1] = am->get("SM_Tree1.e2a")->cast<e2::Mesh>();
	m_treeMeshes[2] = am->get("SM_Tree2.e2a")->cast<e2::Mesh>();
	m_treeMeshes[3] = am->get("SM_Tree3.e2a")->cast<e2::Mesh>();
	m_treeMaterial = am->get("M_Tree.e2a")->cast<e2::Material>();
	m_treeMaterialProxy = gameSession()->getOrCreateDefaultMaterialProxy(m_treeMaterial);

	m_grassMaterial = am->get("M_Grass.e2a")->cast<e2::Material>();

	e2::MeshProxyConfiguration meshCfg = {
		.lods = {
			{
				.maxDistance = 0.0f,
				.mesh = m_treeMeshes[0],
				.materials = { m_treeMaterialProxy }
			}
		}
	};
	// preload this to avoid hitch when you unpack forest tiles
	auto tmp = e2::create<e2::MeshProxy>(gameSession(), meshCfg);
	e2::destroy(tmp);



	//for (uint32_t i = 0; i < 4; i++)
	//{
	//	e2::DynamicMesh src(m_treeMeshes[i], 0);
	//	glm::vec2 offset{};
	//	m_dynamicTreeMeshes[i].addMeshWithShaderFunction(&src, glm::identity<glm::mat4>(), ::treeShader, &offset);
	//}

	m_dynamicTreeMeshes[0] = e2::DynamicMesh(m_treeMeshes[0], 0);
	m_dynamicTreeMeshes[1] = e2::DynamicMesh(m_treeMeshes[1], 0);
	m_dynamicTreeMeshes[2] = e2::DynamicMesh(m_treeMeshes[2], 0);
	m_dynamicTreeMeshes[3] = e2::DynamicMesh(m_treeMeshes[3], 0);




	m_resourceMeshStone = am->get("SM_Stone.e2a").cast<e2::Mesh>();

	m_waterTexture = am->get("T_Minimap_Water.e2a")->cast<e2::Texture2D>();
	m_forestTexture = am->get("T_Minimap_Forest.e2a")->cast<e2::Texture2D>();
	m_mountainTexture = am->get("T_Minimap_Mountains.e2a")->cast<e2::Texture2D>();

	m_baseHex = am->get("SM_Hex.e2a")->cast<e2::Mesh>();
	m_fastHex.initializeFrom(m_baseHex, 0);
	//m_dynaHex = e2::DynamicMesh(m_baseHex, 0, VertexAttributeFlags::Color);

 	m_baseHexHigh = am->get("SM_HexHigh.e2a")->cast<e2::Mesh>();
	m_fastHexHigh.initializeFrom(m_baseHexHigh, 0);
	//m_dynaHexHigh = e2::DynamicMesh(m_baseHexHigh, 0, VertexAttributeFlags::Color);


	glm::vec2 waterOrigin = e2::Hex(glm::ivec2(0)).planarCoords();
	glm::vec2 waterEnd = e2::Hex(glm::ivec2(e2::hexChunkResolution)).planarCoords();
	glm::vec2 waterSize = waterEnd - waterOrigin;
	uint32_t waterResolution = 16;
	glm::vec2 waterTileSize = waterSize / float(waterResolution);


	e2::DynamicMesh dynaFog;
	{
		e2::Vertex tl;
		tl.position.x = 0.0f;
		tl.position.z = 0.0f;

		e2::Vertex tr;
		tr.position.x = waterSize.x;
		tr.position.z = 0.0f;

		e2::Vertex bl;
		bl.position.x = 0.0f;
		bl.position.z = waterSize.y;

		e2::Vertex br;
		br.position.x = waterSize.x;
		br.position.z = waterSize.y;

		uint32_t tli = dynaFog.addVertex(tl);
		uint32_t tri = dynaFog.addVertex(tr);
		uint32_t bli = dynaFog.addVertex(bl);
		uint32_t bri = dynaFog.addVertex(br);

		e2::Triangle a;
		a.a = bri;
		a.b = tri;
		a.c = tli;


		e2::Triangle b;
		b.a = bli;
		b.b = bri;
		b.c = tli;

		dynaFog.addTriangle(a);
		dynaFog.addTriangle(b);
	}
	dynaFog.mergeDuplicateVertices();

	e2::DynamicMesh dynaWater;
	for (uint32_t y = 0; y < waterResolution; y++)
	{
		for (uint32_t x = 0; x < waterResolution; x++)
		{
			e2::Vertex tl;
			tl.position.x = float(x) * waterTileSize.x;
			tl.position.z = float(y) * waterTileSize.y;

			e2::Vertex tr;
			tr.position.x = float(x + 1) * waterTileSize.x;
			tr.position.z = float(y) * waterTileSize.y;

			e2::Vertex bl;
			bl.position.x = float(x) * waterTileSize.x;
			bl.position.z = float(y + 1 ) * waterTileSize.y;

			e2::Vertex br;
			br.position.x = float(x + 1)  * waterTileSize.x;
			br.position.z = float(y + 1)  * waterTileSize.y;

			uint32_t tli = dynaWater.addVertex(tl);
			uint32_t tri = dynaWater.addVertex(tr);
			uint32_t bli = dynaWater.addVertex(bl);
			uint32_t bri = dynaWater.addVertex(br);

			e2::Triangle a;
			a.a = bri;
			a.b = tri;
			a.c = tli;


			e2::Triangle b;
			b.a = bli;
			b.b = bri;
			b.c = tli;

			dynaWater.addTriangle(a);
			dynaWater.addTriangle(b);
		}
	}

	dynaWater.mergeDuplicateVertices();

	auto rm = game()->renderManager();
	auto session = game()->gameSession();

	m_waterMaterial = e2::MaterialPtr::create();
	m_waterMaterial->postConstruct(game(), {});
	m_waterMaterial->overrideModel(rm->getShaderModel("e2::WaterModel"));
	m_waterProxy = session->getOrCreateDefaultMaterialProxy(m_waterMaterial)->unsafeCast<e2::WaterProxy>();
	m_waterChunk = dynaWater.bake(m_waterMaterial, VertexAttributeFlags::None);


	m_fogMaterial = e2::MaterialPtr::create();
	m_fogMaterial->postConstruct(game(), {});
	m_fogMaterial->overrideModel(rm->getShaderModel("e2::FogModel"));
	m_fogProxy = session->getOrCreateDefaultMaterialProxy(m_fogMaterial)->unsafeCast<e2::FogProxy>();
	m_fogChunk = dynaFog.bake(m_fogMaterial, VertexAttributeFlags::None);

	m_terrainMaterial = e2::MaterialPtr::create();
	m_terrainMaterial->postConstruct(game(), {});
	m_terrainMaterial->overrideModel(rm->getShaderModel("e2::TerrainModel"));
	m_terrainProxy = session->getOrCreateDefaultMaterialProxy(m_terrainMaterial)->unsafeCast<e2::TerrainProxy>();





	m_grassLeafMesh = e2::DynamicMesh();
	e2::Vertex protovert;
	protovert.normal = -e2::worldForwardf();
	constexpr float leafHeight = 0.185f;
	constexpr float leafWidthBase = 0.025f;
	constexpr int32_t leafSegments = 3;
	for (int32_t i = 0; i < leafSegments; i++)
	{
		float heightCoeff = float(i) / float(leafSegments - 1);
		float height = heightCoeff * -leafHeight;
		float halfWidth = glm::sineEaseOut(1.0f - heightCoeff) * (leafWidthBase * 0.5f);

		float nextHeightCoeff = float(i + 1) / float(leafSegments - 1);
		float nextHeight = nextHeightCoeff * -leafHeight;

		if (i < leafSegments - 1)
		{
			float nextHalfWidth = glm::sineEaseOut(1.0f - nextHeightCoeff) * (leafWidthBase * 0.5f);
			e2::Vertex vert0 = protovert, vert1 = protovert, vert2 = protovert, vert3 = protovert;
			vert3.position.x = -halfWidth;
			vert3.position.y = height;

			vert2.position.x = +halfWidth;
			vert2.position.y = height;

			vert0.position.x = -nextHalfWidth;
			vert0.position.y = nextHeight;

			vert1.position.x = +nextHalfWidth;
			vert1.position.y = nextHeight;

			m_grassLeafMesh.addTriangle(vert3, vert0, vert2);
			m_grassLeafMesh.addTriangle(vert0, vert1, vert2);
		}
		else
		{
			e2::Vertex vert0 = protovert, vert2 = protovert, vert3 = protovert;
			vert3.position.x = -halfWidth;
			vert3.position.y = height;

			vert2.position.x = +halfWidth;
			vert2.position.y = height;

			vert0.position.x = 0.0f;
			vert0.position.y = nextHeight;

			m_grassLeafMesh.addTriangle(vert3, vert0, vert2);
		}
	}

	m_grassLeafMesh.calculateFaceNormals();
	m_grassLeafMesh.calculateVertexNormals();



	constexpr int32_t numLeafs = 2048;
	for (int32_t i = 0; i < 4; i++)
	{
		m_dynamicGrassMeshes[i] = e2::DynamicMesh();
		e2::DynamicMesh* curr = &m_dynamicGrassMeshes[i];

		for (int32_t g = 0; g < numLeafs; g++)
		{
			glm::mat4 transform = glm::identity<glm::mat4>();
			glm::vec2 offset = e2::randomInUnitCircle();
			glm::vec3 pos{ offset.x, 0.0f, offset.y };

			transform = glm::translate(transform, pos);
			transform = glm::rotate(transform, glm::radians(e2::randomFloat(0.0f, 359.99f)), e2::worldUpf());
			transform = glm::scale(transform, glm::vec3(e2::randomFloat(0.87f, 1.12f)));

			curr->addMeshWithShaderFunction(&m_grassLeafMesh, transform, ::grassShader, &offset);
		}

		m_grassMeshes[i] = curr->bake(m_grassMaterial, e2::VertexAttributeFlags::Normal | e2::VertexAttributeFlags::TexCoords01);
	}




	initializeFogOfWar();

	rebuildForestMeshes();
}

e2::HexGrid::~HexGrid()
{
	clearAllChunks();
	destroyFogOfWar();
}


e2::Engine* e2::HexGrid::engine()
{
	return game()->engine();
}

e2::Game* e2::HexGrid::game()
{
	return m_game;
}




void e2::HexGrid::saveToBuffer(e2::IStream& toBuffer)
{
	toBuffer << uint64_t(m_discoveredChunks.size());
	for (glm::ivec2 const& vec : m_discoveredChunks)
	{
		toBuffer << vec;
	}

	toBuffer << m_discoveredChunksAABB;

	toBuffer << uint64_t(m_tiles.size());
	for (uint64_t i = 0; i < m_tiles.size(); i++)
	{
		toBuffer << int32_t(m_tileVisibility[i]);
		toBuffer << uint16_t(m_tiles[i].empireId);
		toBuffer << uint16_t(m_tiles[i].flags);
	}

	toBuffer << uint64_t(m_tileIndex.size());
	for (auto& [hex, ind]: m_tileIndex)
	{
		toBuffer << uint64_t(ind) << hex;
	}

	toBuffer << m_worldBounds;
	toBuffer << m_minimapViewBounds;
	toBuffer << m_minimapViewOffset;
	toBuffer << m_minimapViewZoom;
}

void e2::HexGrid::loadFromBuffer(e2::IStream& fromBuffer)
{
	uint64_t numDiscoveredChunks{};
	fromBuffer >> numDiscoveredChunks;
	for (uint64_t i = 0; i < numDiscoveredChunks; i++)
	{
		glm::ivec2 chunkIndex;
		fromBuffer >> chunkIndex;
		m_discoveredChunks.insert(chunkIndex);
	}

	fromBuffer >> m_discoveredChunksAABB;

	uint64_t numTiles{};
	fromBuffer >> numTiles;
	for (uint64_t i = 0; i < numTiles; i++)
	{
		int32_t newVis{};
		fromBuffer >> newVis;
		m_tileVisibility.push_back(newVis);

		TileData newTile;
		uint16_t empId{};
		fromBuffer >> empId;
		newTile.empireId = (EmpireId)empId;
		uint16_t flags{};
		fromBuffer >> flags;
		newTile.flags = (e2::TileFlags)flags;

		newTile.resourceMesh = getResourceMeshForFlags(newTile.flags);

		m_tiles.push_back(newTile);

	}

	uint64_t numTileIndex{};
	fromBuffer >> numTileIndex;

	for (uint64_t i = 0; i < numTileIndex; i++)
	{
		glm::ivec2 hex;
		uint64_t ind;
		fromBuffer >> ind >> hex;
		m_tileIndex[hex] = ind;
	}



	fromBuffer >> m_worldBounds;
	fromBuffer >> m_minimapViewBounds;
	fromBuffer >> m_minimapViewOffset;
	fromBuffer >> m_minimapViewZoom;
}

e2::Aabb2D e2::HexGrid::getChunkAabb(glm::ivec2 const& chunkIndex)
{
	glm::vec2 _chunkSize = chunkSize();
	constexpr uint32_t r = e2::hexChunkResolution;
	glm::vec2 chunkBoundsOffset = e2::Hex(glm::ivec2(chunkIndex.x * r, chunkIndex.y * r)).planarCoords();
	e2::Aabb2D chunkAabb;
	chunkAabb.min = chunkBoundsOffset - glm::vec2(2.0);
	chunkAabb.max = chunkBoundsOffset + _chunkSize + glm::vec2(4.0);
	return chunkAabb;
}

glm::vec2 e2::HexGrid::chunkSize()
{
	return e2::Hex(glm::ivec2(e2::hexChunkResolution)).planarCoords();
}

glm::ivec2 e2::HexGrid::chunkIndexFromPlanarCoords(glm::vec2 const& planarCoords)
{
	glm::ivec2 offset = e2::Hex(planarCoords).offsetCoords();
	glm::ivec2 off2{ 0, 0 };

	if (glm::sign(offset.x) < 0)
		off2.x--;
	if (glm::sign(offset.y) < 0)
		off2.y--;

	return ((offset - off2) / glm::ivec2(e2::hexChunkResolution)) + off2;

	//return glm::ivec2(planarCoords / chunkSize());
}

glm::vec3 e2::HexGrid::chunkOffsetFromIndex(glm::ivec2 const& index)
{
	return e2::Hex(index * glm::ivec2(e2::hexChunkResolution)).localCoords();
}



constexpr uint32_t squirrelNoise5(int32_t x, uint32_t seed)
{
	constexpr uint32_t SQ5_BIT_NOISE1 = 0xd2a80a3f;	// 11010010101010000000101000111111
	constexpr uint32_t SQ5_BIT_NOISE2 = 0xa884f197;	// 10101000100001001111000110010111
	constexpr uint32_t SQ5_BIT_NOISE3 = 0x6C736F4B; // 01101100011100110110111101001011
	constexpr uint32_t SQ5_BIT_NOISE4 = 0xB79F3ABB;	// 10110111100111110011101010111011
	constexpr uint32_t SQ5_BIT_NOISE5 = 0x1b56c4f5;	// 00011011010101101100010011110101

	uint32_t mangledBits = (uint32_t)x;
	mangledBits *= SQ5_BIT_NOISE1;
	mangledBits += seed;
	mangledBits ^= (mangledBits >> 9);
	mangledBits += SQ5_BIT_NOISE2;
	mangledBits ^= (mangledBits >> 11);
	mangledBits *= SQ5_BIT_NOISE3;
	mangledBits ^= (mangledBits >> 13);
	mangledBits += SQ5_BIT_NOISE4;
	mangledBits ^= (mangledBits >> 15);
	mangledBits *= SQ5_BIT_NOISE5;
	mangledBits ^= (mangledBits >> 17);
	return mangledBits;
}

constexpr uint32_t sqNoise2(int32_t x, int32_t y, uint32_t seed)
{
	constexpr int PRIME_NUMBER = 198491317; // Large prime number with non-boring bits
	return squirrelNoise5(x + (PRIME_NUMBER * y), seed);
}


namespace
{
	struct HexShaderData
	{
		e2::HexGrid* grid{};
		e2::Hex hex;
		glm::vec2 chunkOffset;
		glm::ivec2 cacheOffset;

		e2::TileData tileCache[64];

		e2::TileData& getTileData(glm::ivec2 const& offsetCoords)
		{
			glm::ivec2 cacheAlignedCoords = offsetCoords - cacheOffset + 1;

			int32_t index = cacheAlignedCoords.y * ((int32_t)e2::hexChunkResolution + 2) + cacheAlignedCoords.x;

			return tileCache[index];
		}

		//e2::TileData& getTileDataOld(glm::ivec2 const& offsetCoords)
		//{
		//	if (offsetCoords == hex.offsetCoords())
		//		return tileData;

		//	auto finder = tileCacheOld.find(offsetCoords);
		//	if (finder == tileCacheOld.end())
		//	{
		//		e2::TileData newTileData = grid->calculateTileData(offsetCoords);
		//		return tileCacheOld[offsetCoords] = newTileData;
		//	}

		//	return finder->second;

		//}
		//std::unordered_map<glm::ivec2, e2::TileData> tileCacheOld;

	};


	inline float ridge(float v, float offset)
	{
		v = offset - glm::abs(v);
		return v < 0.5f ? 4.0f * v * v * v : (v - 1.0f) * (2.0f * v - 2.0f) * (2.0f * v - 2.0f) + 1.0f;
	}

	inline float ridgedMF(glm::vec2 const& position)
	{
		static const struct {
			uint32_t octaves{ 6 }; // 2
			float frequency{ 2.0f }; // 0.3
			float lacunarity{ 2.0f }; // 2.0
			float gain{ 0.5f }; // 0.5
			float ridgeOffset{ 1.0f }; // 0.95
		} cfg;

		float sum = 0.0f;
		float amp = 0.5f;
		float prev = 1.0f;
		float freq = cfg.frequency;
		for (uint32_t i = 0; i < cfg.octaves; i++)
		{
			float h = glm::simplex(position * freq);
			//float h = 0.5f;
			float n = ridge(h, cfg.ridgeOffset);
			sum += n * amp * prev;
			prev = n;
			freq *= cfg.lacunarity;
			amp *= cfg.gain;
		}

		return sum;
	}

	inline float sampleHeight(glm::vec2 const& worldPosition, ::HexShaderData* shaderData)
	{
		float outHeight = 0.0f;

		e2::Hex& hex = shaderData->hex;
		glm::vec2 hexCenter = hex.planarCoords();

		e2::TileData &tile = shaderData->getTileData(hex.offsetCoords());


		bool currIsMountain = (tile.flags & e2::TileFlags::FeatureMountains) != e2::TileFlags::FeatureNone;
		bool currIsShallow = (tile.flags & e2::TileFlags::WaterMask) == e2::TileFlags::WaterShallow;
		bool currIsOcean = (tile.flags & e2::TileFlags::WaterMask) == e2::TileFlags::WaterDeep;
		bool currIsWater = (tile.flags & e2::TileFlags::WaterMask) != e2::TileFlags::WaterNone;

		float distanceFromVertexToTile = glm::distance(worldPosition, hexCenter);
		float mountainDistanceCoeff = 1.0f - glm::clamp(distanceFromVertexToTile * 1.4f, 0.0f, 1.0f);
		float waterDistanceCoeff = 1.0f - glm::clamp(distanceFromVertexToTile * 1.4f, 0.0f, 1.0f);

		float coeffMountains = 0.0f;
		float coeffWater = 0.0f;

		if (currIsMountain || currIsWater)
		{
			coeffMountains = mountainDistanceCoeff;
			coeffWater = waterDistanceCoeff;

			auto neighbours = hex.neighbours();
			for (uint8_t ni = 0; ni < neighbours.size(); ni++)
			{
				uint8_t nextNi = ni + 1;
				if (nextNi >= neighbours.size())
					nextNi = 0;

				e2::Hex &neighbourHex = neighbours[ni];
				e2::TileData& neighbourTile = shaderData->getTileData(neighbourHex.offsetCoords());
				glm::vec2 neighbourWorldPosition = neighbourHex.planarCoords();

				e2::Hex& nextHex = neighbours[nextNi];
				e2::TileData& nextTile = shaderData->getTileData(nextHex.offsetCoords());
				glm::vec2 nextWorldPosition = nextHex.planarCoords();

				bool neighbourIsMountain = (neighbourTile.flags & e2::TileFlags::FeatureMountains) != e2::TileFlags::FeatureNone;
				bool neighbourIsWater = (neighbourTile.flags & e2::TileFlags::WaterMask) != e2::TileFlags::WaterNone;

				bool nextIsMountain = (nextTile.flags & e2::TileFlags::FeatureMountains) != e2::TileFlags::FeatureNone;
				bool nextIsWater = (nextTile.flags & e2::TileFlags::WaterMask) != e2::TileFlags::WaterNone;

				glm::vec2 midpoint = (hexCenter + neighbourWorldPosition + nextWorldPosition) / 3.0f;
				float distanceToMidpoint = glm::distance(worldPosition, midpoint);

				if (neighbourIsMountain && nextIsMountain)
				{
					float x = 1.0f - glm::clamp(distanceToMidpoint, 0.0f, 1.0f);
					coeffMountains += x;
				}

				if (neighbourIsWater && nextIsWater)
				{
					float x = 1.0f - glm::clamp(distanceToMidpoint, 0.0f, 1.0f);
					coeffWater += x;
				}
			}

			if (currIsMountain)
			{
				coeffMountains = glm::clamp(coeffMountains, 0.0f, 1.0f);
				//outHeight -= coeffMountains;
				outHeight -= ridgedMF(worldPosition * 0.1f/** e2::mtnFreqScale*/) * coeffMountains * 0.9f;// e2::mtnScale;
			}

			coeffWater = glm::clamp(coeffWater, 0.0f, 1.0f);
			if (currIsWater)
			{
				outHeight += coeffWater * 1.0f;

				float flattenHeight = 1.0f;
				float distFromBottom = glm::distance(outHeight, flattenHeight);
				float flattenCoeff = 1.0f - glm::smoothstep(0.0f, 0.3f, distFromBottom);
				outHeight = glm::mix(outHeight, flattenHeight, flattenCoeff);
			}
		}

		return outHeight;

	}

	inline glm::vec3 sampleNormal(glm::vec2 const& worldPosition, ::HexShaderData* shaderData)
	{

		float eps = 0.1f;
		float eps2 = eps * 2.0f;

		float hL = sampleHeight(worldPosition - glm::vec2(1.0f, 0.0f) * eps, shaderData);
		float hR = sampleHeight(worldPosition + glm::vec2(1.0f, 0.0f) * eps, shaderData);
		
		float hU = sampleHeight(worldPosition - glm::vec2(0.0f, 1.0f) * eps, shaderData);
		float hD = sampleHeight(worldPosition + glm::vec2(0.0f, 1.0f) * eps, shaderData);

		return glm::normalize(glm::vec3(hR - hL, -eps2, hD - hU));

	}

	// r = desert, g = grassland, b = tundra, a = forest
	inline glm::vec4 sampleBiomeAtVertex(glm::vec2 const& worldPosition, ::HexShaderData* shaderData)
	{
		auto tileFlagsToValues = [](e2::TileFlags tileFlags)  -> glm::vec4 {
			glm::vec4 returner;
			returner.x = ((tileFlags & e2::TileFlags::BiomeMask) == e2::TileFlags::BiomeDesert) ? 1.0f : 0.0f;
			returner.y = ((tileFlags & e2::TileFlags::BiomeMask) == e2::TileFlags::BiomeGrassland) ? 1.0f : 0.0f;
			returner.z = ((tileFlags & e2::TileFlags::BiomeMask) == e2::TileFlags::BiomeTundra) ? 1.0f : 0.0f;
			returner.w = ((tileFlags & e2::TileFlags::FeatureForest) == e2::TileFlags::FeatureNone) ? 0.0f : 1.0f;

			return returner;
		};

		constexpr float hexDistBlendLen = 2.0f;

		e2::Hex& hex = shaderData->hex;
		glm::vec2 hexCenter = hex.planarCoords();
		float hexDistance = glm::distance(worldPosition, hexCenter);
		float hexWeight = 1.0f - glm::smoothstep(0.0f, hexDistBlendLen, hexDistance);
		e2::TileData& hexTile = shaderData->getTileData(hex.offsetCoords());// @todo only need biome info here! @perf

		glm::vec4 finalValues = tileFlagsToValues(hexTile.flags) * hexWeight;

		for (e2::Hex const& neighbourHex : hex.neighbours())
		{
			glm::vec2 neighbourCenter = neighbourHex.planarCoords();
			float neighbourDistance = glm::distance(worldPosition, neighbourCenter);
			float neighbourWeight = 1.0f - glm::smoothstep(0.0f, hexDistBlendLen, neighbourDistance);
			e2::TileData neighbourTile = shaderData->getTileData(neighbourHex.offsetCoords()); // @todo only need biome info here! @perf

			finalValues += tileFlagsToValues(neighbourTile.flags) * neighbourWeight;
		}

		finalValues = glm::normalize(finalValues);
		
		//finalValues.y = glm::max(finalValues.y, finalValues.z);

		return finalValues;
	}


	//void hexShader(e2::Vertex* vertex, void* shaderData)
	//{
	//	::HexShaderData* data = reinterpret_cast<::HexShaderData*>(shaderData);

	//	e2::TileData curr = data->tileData;

	//	glm::vec2 localVertexPosition = glm::vec2(vertex->position.x, vertex->position.z);
	//	glm::vec2 worldVertexPosition = data->chunkOffset + localVertexPosition;

	//	vertex->uv01 = { worldVertexPosition, 0.0f, 0.0f };
	//	vertex->position.y = sampleHeight(worldVertexPosition, data);
	//	vertex->normal = sampleNormal(worldVertexPosition, data);

	//	//vertex->tangent = glm::rotate(glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, -1.0f)) * vertex->normal;
	//	
	//	glm::vec4 biomeValues = sampleBiomeAtVertex(worldVertexPosition, data);

	//	// save hex grid thingy
	//	vertex->color.a = vertex->color.r;

	//	// forest 
	//	vertex->color.r = biomeValues.a;

	//	// tundra
	//	vertex->color.g = biomeValues.g;

	//	vertex->color.b = biomeValues.b;

	//	
	//}


	//void hexShaderFast(glm::vec4* pos, glm::vec4* nrm, glm::vec4* col, void* shaderFuncData)
	//{
	//	::HexShaderData* data = reinterpret_cast<::HexShaderData*>(shaderFuncData);

	//	e2::TileData curr = data->tileData;

	//	glm::vec2 localVertexPosition = glm::vec2(pos->x, pos->z);
	//	glm::vec2 worldVertexPosition = data->chunkOffset + localVertexPosition;

	//	pos->y = sampleHeight(worldVertexPosition, data);
	//	*nrm = { sampleNormal(worldVertexPosition, data), 0.0f};

	//	glm::vec4 biomeValues = sampleBiomeAtVertex(worldVertexPosition, data);

	//	*col = { biomeValues.a,biomeValues.g, biomeValues.b, 0.0f};
	//}

	void hexShaderFast2(e2::FastVertex2 *vertex, void* shaderFuncData)
	{
		::HexShaderData* data = reinterpret_cast<::HexShaderData*>(shaderFuncData);

		glm::vec2 localVertexPosition = glm::vec2(vertex->position.x, vertex->position.z);
		glm::vec2 worldVertexPosition = data->chunkOffset + localVertexPosition;

		vertex->position.y = sampleHeight(worldVertexPosition, data);
		vertex->normal = { sampleNormal(worldVertexPosition, data), 0.0f };

		glm::vec4 biomeValues = sampleBiomeAtVertex(worldVertexPosition, data);

		vertex->color = { biomeValues.a,biomeValues.g, biomeValues.b, 0.0f };

	}
}


template< typename T, typename Pred >
typename std::vector<T>::iterator
insert_sorted(std::vector<T>& vec, T const& item, Pred pred)
{
	return vec.insert
	(
		std::upper_bound(vec.begin(), vec.end(), item, pred),
		item
	);
}

void e2::HexGrid::forceStreamView(e2::Viewpoints2D const& view)
{
	m_forceStreamQueue.push_back(view);
}

void e2::HexGrid::updateStreaming(glm::vec2 const& streamCenter, e2::Viewpoints2D const& viewPoints, glm::vec2 const& viewVelocity)
{
	e2::Renderer* renderer = gameSession()->renderer();
	float viewSpeed = glm::length(viewVelocity);

	m_streamingCenter = streamCenter;
	m_streamingView = viewPoints;
	m_streamingViewVelocity= viewVelocity;
	m_streamingViewAabb = m_streamingView.toAabb();

	auto gatherChunkStatesInView = [this](e2::Viewpoints2D const& streamPoints, std::unordered_set<e2::ChunkState*> & outChunks, bool forLookAhead) {
		e2::Aabb2D streamAabb = streamPoints.toAabb();
		glm::ivec2 lowerIndex = chunkIndexFromPlanarCoords(streamAabb.min) - glm::ivec2(1, 1);
		glm::ivec2 upperIndex = chunkIndexFromPlanarCoords(streamAabb.max);
		for (int32_t y = lowerIndex.y - 1; y <= upperIndex.y + 1; y++)
		{
			for (int32_t x = lowerIndex.x - 1; x <= upperIndex.x + 1; x++)
			{
				glm::ivec2 chunkIndex = glm::ivec2(x, y);
				e2::Aabb2D chunkAabb = getChunkAabb(chunkIndex);

				if (streamPoints.test(chunkAabb))
				{
					e2::ChunkState* newState = getOrCreateChunk(chunkIndex);
					if(!forLookAhead || ((!newState->inView) && newState->streamState == StreamState::Poked))
						outChunks.insert(newState);
				}
			}
		}
	};

	// gather all chunks in view, and pop them in if theyre ready, or queue them for streaming
	for (auto& [chunkIndex, chunk] : m_chunkIndex)
	{
		chunk->inView = false;
	}

	m_chunksInView.clear();

	gatherChunkStatesInView(m_streamingView, m_chunksInView, false);
	for (e2::ChunkState* chunk : m_chunksInView)
	{
		chunk->inView = true;
		chunk->lastTimeInView = e2::timeNow();

		if (!chunk->visibilityState)
			popInChunk(chunk);

		if (!m_streamingPaused)
		{
			if (chunk->streamState == StreamState::Poked)
			{
				queueStreamingChunk(chunk);
			}
		}
	}

	// force stream requested
	for (e2::Viewpoints2D const& vp : m_forceStreamQueue)
	{
		std::unordered_set<e2::ChunkState*> forceChunks;
		gatherChunkStatesInView(vp, forceChunks, true);
		for (e2::ChunkState* chunk : forceChunks)
		{
			chunk->inView = true;
			chunk->lastTimeInView = e2::timeNow();

			if (chunk->streamState == StreamState::Poked)
			{
				queueStreamingChunk(chunk);
			}

			if (chunk->streamState == StreamState::Queued)
			{
				startStreamingChunk(chunk);
			}

			
		}
	}
	m_forceStreamQueue.clear();

	// gather chunks via trace 
	constexpr float lookAheadTreshold = 0.1f;
	const float lookAheadLength = chunkSize().x * 1.5f;
	constexpr int32_t maxLookaheads = 3;

	glm::vec2 lookDir = glm::normalize(m_streamingViewVelocity);
	float lookSpeed = glm::length(m_streamingViewVelocity);
	lookSpeed = glm::clamp(lookSpeed, 0.0f, maxLookaheads * lookAheadTreshold);




	if (!m_streamingPaused)
	{
		m_lookAheadChunks.clear();
		uint32_t numLookAheads = uint32_t(lookSpeed / lookAheadTreshold);
		for (uint32_t i = 0; i < numLookAheads; i++)
		{
			glm::vec2 lookAheadOffset = lookDir * (lookAheadLength * float(i));
			e2::Viewpoints2D lookAheadView = m_streamingView;
			lookAheadView.bottomLeft += lookAheadOffset;
			lookAheadView.bottomRight += lookAheadOffset;
			lookAheadView.topLeft += lookAheadOffset;
			lookAheadView.topRight += lookAheadOffset;
			lookAheadView.view.origin.x += lookAheadOffset.x;
			lookAheadView.view.origin.z += lookAheadOffset.y;
			lookAheadView.calculateDerivatives();
			gatherChunkStatesInView(lookAheadView, m_lookAheadChunks, true);

			//renderer->debugLine({ 1.0f, 0.0f, 1.0f }, lookAheadView.corners[0], lookAheadView.corners[1]);
			//renderer->debugLine({ 1.0f, 0.0f, 1.0f }, lookAheadView.corners[2], lookAheadView.corners[3]);
			//renderer->debugLine({ 1.0f, 0.0f, 1.0f }, lookAheadView.corners[0], lookAheadView.corners[2]);
			//renderer->debugLine({ 1.0f, 0.0f, 1.0f }, lookAheadView.corners[1], lookAheadView.corners[3]);

		}

		// queue look ahead chunks
		if (!m_streamingPaused)
		{
			for (e2::ChunkState* chunk : m_lookAheadChunks)
				queueStreamingChunk(chunk);
		}
	}

	// go through all invalidated chunks(newly fresh steaming), and pop them in in case they arent 
	for (e2::ChunkState* newChunk : m_invalidatedChunks)
	{
		if (newChunk->inView)
			popInChunk(newChunk);
	}
	m_invalidatedChunks.clear();

	// go through outdated chunks and refresh their meshes 
	for (e2::ChunkState* newChunk : m_outdatedChunks)
	{
		//if (newChunk->inView)
			//refreshChunkMeshes(newChunk);
	}
	m_outdatedChunks.clear();


	// go through all chunks, check if they arent in view, but are in visible, if so pop them out
	for (auto& [chunkIndex, chunk] : m_chunkIndex)
	{
		if (!chunk->inView && m_visibleChunks.contains(chunk))
			popOutChunk(chunk);
	}
	

	if (m_streamingPaused)
	{
		auto cpy = m_chunkIndex;
		for (auto& [chunkIndex, chunk] : cpy)
		{
			if (!chunk->inView && !(chunk->streamState == StreamState::Streaming || chunk->streamState == StreamState::Ready))
				nukeChunk(chunk);
		}

		return;
	}

	// Proritize queue and then cap its size 
	glm::vec2 halfChunkSize = chunkSize() / 2.0f;

	std::vector<e2::ChunkState*> sortedQueued;
	for (e2::ChunkState* queuedChunk : m_queuedChunks)
	{
		insert_sorted(sortedQueued, queuedChunk, [this, halfChunkSize](e2::ChunkState* a, e2::ChunkState* b) {
			// a in view but b is not, then a is always first
			if (a->inView && !b->inView)
				return true;
			// a not in view but b is, then a is always last
			if (!a->inView && b->inView)
				return false;
			// both in view, sort by distance to streaming center 
			if (a->inView && b->inView)
			{
				glm::vec3 ac = chunkOffsetFromIndex(a->chunkIndex) + glm::vec3(halfChunkSize.x, 0.0f, halfChunkSize.y);
				glm::vec3 bc = chunkOffsetFromIndex(b->chunkIndex) + glm::vec3(halfChunkSize.x, 0.0f, halfChunkSize.y);
				float ad = glm::distance(ac, glm::vec3(m_streamingCenter.x, 0.0f, m_streamingCenter.y));
				float bd = glm::distance(bc, glm::vec3(m_streamingCenter.x, 0.0f, m_streamingCenter.y));
				return ad < bd;
			}

			// none in view, go by time since they were in view
			double ageA = a->lastTimeInView.durationSince().seconds();
			double ageB = b->lastTimeInView.durationSince().seconds();

			return ageA < ageB;
			});
	}

	int32_t currStreaming = (int32_t)m_streamingChunks.size();
	int32_t numFreeSlots = m_numThreads - currStreaming;
	constexpr int32_t maxQueued = 32;
	for (int32_t i = 0; i < sortedQueued.size(); i++)
	{
		// if we are in range of free slots, start streaming from queue
		if (i < numFreeSlots)
			startStreamingChunk(sortedQueued[i]);
		else if (i > maxQueued)
			nukeChunk(sortedQueued[i]);
	}

	// prioritize and cull old chunks
	int32_t numHidden = (int32_t)m_hiddenChunks.size();
	int32_t numToCull = numHidden - e2::maxNumExtraChunks;
	int32_t numToKeep = numHidden - numToCull;
	if (numToCull > 0)
	{

		glm::vec2 halfChunkSize = chunkSize() / 2.0f;

		std::vector<e2::ChunkState*> sortedQueued;
		for (e2::ChunkState* queuedChunk : m_hiddenChunks)
		{
			insert_sorted(sortedQueued, queuedChunk, [this, halfChunkSize](e2::ChunkState* a, e2::ChunkState* b) {
				// a in view but b is not, then a is always first
				if (a->inView && !b->inView)
					return true;
				// a not in view but b is, then a is always last
				if (!a->inView && b->inView)
					return false;
				// both in view, sort by distance to streaming center 
				if (a->inView && b->inView)
				{
					glm::vec3 ac = chunkOffsetFromIndex(a->chunkIndex) + glm::vec3(halfChunkSize.x, 0.0f, halfChunkSize.y);
					glm::vec3 bc = chunkOffsetFromIndex(b->chunkIndex) + glm::vec3(halfChunkSize.x, 0.0f, halfChunkSize.y);
					float ad = glm::distance(ac, glm::vec3(m_streamingCenter.x, 0.0f, m_streamingCenter.y));
					float bd = glm::distance(bc, glm::vec3(m_streamingCenter.x, 0.0f, m_streamingCenter.y));
					return ad < bd;
				}

				// none in view, go by time since they were in view
				double ageA = a->lastTimeInView.durationSince().seconds();
				double ageB = b->lastTimeInView.durationSince().seconds();

				return ageA < ageB;
				});
		}

		for (int32_t i = numToKeep; i < numToKeep + numToCull; i++)
		{
			if (i >= sortedQueued.size())
				break;
			nukeChunk(sortedQueued[i]);
		}
	}

	if(game()->isRealtime())
		updateUnpackedForests();

	debugDraw();

}


void e2::HexGrid::clearQueue()
{
	auto copy = m_queuedChunks;
	for (e2::ChunkState* state : copy)
	{
		state->streamState = StreamState::Poked;
		m_queuedChunks.erase(state);
	}
}

e2::ChunkState* e2::HexGrid::getOrCreateChunk(glm::ivec2 const& index)
{
	auto finder = m_chunkIndex.find(index);
	if (finder == m_chunkIndex.end())
	{
		e2::ChunkState* newState = e2::create<e2::ChunkState>();
		newState->chunkIndex = index;

		m_chunkIndex[index] = newState;
		m_hiddenChunks.insert(newState);

		return newState;
	}

	return finder->second;
}

void e2::HexGrid::nukeChunk(e2::ChunkState* chunk)
{
	popOutChunk(chunk);

	m_queuedChunks.erase(chunk);
	m_streamingChunks.erase(chunk);
	m_invalidatedChunks.erase(chunk);
	m_visibleChunks.erase(chunk);
	m_hiddenChunks.erase(chunk);
	m_chunksInView.erase(chunk);
	m_lookAheadChunks.erase(chunk);

	m_chunkIndex.erase(chunk->chunkIndex);
	chunk->task = nullptr;
	e2::discard(chunk);
}


float e2::HexGrid::sampleSimplex(glm::vec2 const& position)
{
	return glm::simplex(position) * 0.5f + 0.5f;
}

void e2::HexGrid::calculateBiome(glm::vec2 const& planarCoords, e2::TileFlags& outFlags)
{
	float biomeCoeff = sampleSimplex((planarCoords + glm::vec2(81.44f, 93.58f)) * 0.01f);

	if (biomeCoeff > 0.8f)
		outFlags |= TileFlags::BiomeTundra;
	else if (biomeCoeff > 0.4f)
		outFlags |= TileFlags::BiomeGrassland;
	else
		outFlags |= TileFlags::BiomeDesert;
}


float e2::HexGrid::calculateBaseHeight(glm::vec2 const& planarCoords)
{
	float baseHeight = sampleSimplex((planarCoords + glm::vec2(32.16f, 64.32f)) * 0.0135f);
	return baseHeight;

	/*float h1p = 0.42;
	float scale1 = 0.058;
	float h1 = glm::pow(sampleSimplex(position * scale1), h1p);

	float semiStart = 0.31;
	float semiSize = 0.47;
	float h2p = 0.013;
	float h2 = glm::smoothstep(semiStart, semiStart + semiSize, sampleSimplex(position * h2p));

	float semiStart2 = 0.65;
	float semiSize2 = 0.1;
	float h3p = (0.75 * 20) / 5000;
	float h3 = 1.0 - glm::smoothstep(semiStart2, semiStart2 + semiSize2, sampleSimplex(position * h3p));

	return h1 * h2 * h3;*/
}

void e2::HexGrid::calculateResources(glm::vec2 const& planarCoords, e2::TileFlags& outFlags)
{
	
	if ((outFlags & e2::TileFlags::FeatureMask) != e2::TileFlags::FeatureNone)
		return;

	float abundanceCoeff = sampleSimplex((planarCoords + glm::vec2(41.44f, 73.28f)) * 2.0f);
	if (abundanceCoeff > 0.97)
		outFlags |= TileFlags::Abundance4;
	else if (abundanceCoeff > 0.86)
		outFlags |= TileFlags::Abundance3;
	else if (abundanceCoeff > 0.75)
		outFlags |= TileFlags::Abundance2;
	else
		outFlags |= TileFlags::Abundance1;

	float resourceBase = sampleSimplex((planarCoords + glm::vec2(11.44f, 53.28f)) * 8.0f);

	if ((outFlags & e2::TileFlags::WaterMask) == e2::TileFlags::WaterDeep)
	{
		if (resourceBase > 0.865)
			outFlags |= TileFlags::ResourceStone;
		return;
	}
	if ((outFlags & e2::TileFlags::WaterMask) == e2::TileFlags::WaterShallow)
	{
		if (resourceBase > 0.92)
			outFlags |= TileFlags::ResourceStone;
		return;
	}

	if (abundanceCoeff > 0.5f)
	{
		/*if (resourceBase > 0.97)
			outFlags |= TileFlags::ResourceUranium;
		else if (resourceBase > 0.85)
			outFlags |= TileFlags::ResourceGold;
		else if (resourceBase > 0.7)
			outFlags |= TileFlags::ResourceOre;*/
		// else if
		if (resourceBase > 0.85)
			outFlags |= TileFlags::ResourceStone;
	}
}

void e2::HexGrid::calculateFeaturesAndWater(glm::vec2 const& planarCoords, float baseHeight, e2::TileFlags& outFlags)
{
	if (baseHeight > 0.75f)
	{

		if (baseHeight > 0.6f)
		{
			float forestCoeff = sampleSimplex((planarCoords + glm::vec2(32.14f, 29.28f)) * 7.5f);

			if (forestCoeff > 0.6f && (baseHeight <= 0.90f) && ((outFlags & TileFlags::BiomeMask) != TileFlags::BiomeDesert))
				outFlags |= TileFlags::FeatureForest;

			if (forestCoeff > 0.950f)
				outFlags |= TileFlags::WoodAbundance4;
			else if (forestCoeff > 0.90f)
				outFlags |= TileFlags::WoodAbundance3;
			else if (forestCoeff > 0.80f)
				outFlags |= TileFlags::WoodAbundance2;
			else
				outFlags |= TileFlags::WoodAbundance1;

			if (baseHeight > 0.90f)
				outFlags |= TileFlags::FeatureMountains;
		}
	}
	else if (baseHeight > 0.57f)
	{
		outFlags |= TileFlags::WaterShallow;
	}
	else
	{
		outFlags |= TileFlags::WaterDeep;
	}
}

e2::MeshPtr e2::HexGrid::getResourceMeshForFlags(e2::TileFlags flags)
{
	//return nullptr;
	switch (flags & e2::TileFlags::ResourceMask)
	{
		case e2::TileFlags::ResourceStone:
			return m_resourceMeshStone;
		case e2::TileFlags::ResourceGold:
			return m_resourceMeshStone; // @todo
		case e2::TileFlags::ResourceOre:
			return m_resourceMeshStone; // @todo
		case e2::TileFlags::ResourceUranium:
			return m_resourceMeshStone; // @todo
		default:
			return nullptr;
	}
}

int32_t e2::HexGrid::getForestIndexForFlags(e2::TileFlags flags)
{

	if ((flags & e2::TileFlags::FeatureForest) != e2::TileFlags::FeatureNone)
	{
		// @todo nested switch on biome
		switch ((flags & e2::TileFlags::WoodAbundanceMask))
		{
		case TileFlags::WoodAbundance1:
			return 0;
			break;
		case TileFlags::WoodAbundance2:
			return 1;
			break;
		default:
		case TileFlags::WoodAbundance3:
		case TileFlags::WoodAbundance4:
			return 2;
			break;
		}
	}
	return -1;
}

e2::ForestState* e2::HexGrid::getForestState(int32_t index)
{
	if(index < 0 || index > 2)
		return nullptr;

	return &m_forestStates[index];

}

void e2::HexGrid::damageTree(glm::ivec2 const& hex, uint32_t treeIndex, float dmg)
{
	e2::UnpackedForestState *state = unpackForestState(hex);
	if (!state)
		return;

	if(treeIndex >= state->trees.size())
		return;

	state->trees[treeIndex].health -= dmg;
	state->trees[treeIndex].shake = 0.4f;

	glm::vec2 to = state->trees[treeIndex].worldOffset;
	glm::vec3 two{to.x, -0.25f ,to.y};
	game()->spawnHitLabel(two, std::format("{}", uint32_t(dmg*100.0f)));

	if (state->trees[treeIndex].health <= 0.0f)
	{
		audioManager()->playOneShot(m_woodDieSound, 1.2f, 0.25f, e2::Hex(hex).localCoords());
		audioManager()->playOneShot(m_treeFallSound, 1.0f, 1.0f, e2::Hex(hex).localCoords());
	}
	
}

e2::UnpackedForestState* e2::HexGrid::getUnpackedForestState(glm::ivec2 const& hex)
{
	auto finder = m_unpackedForests.find(hex);
	if (finder == m_unpackedForests.end())
		return nullptr;

	return &finder->second;
}

e2::UnpackedForestState* e2::HexGrid::unpackForestState(glm::ivec2 const& hex)
{
	// return if already unpacked
	e2::UnpackedForestState* unpacked = getUnpackedForestState(hex);
	if (unpacked)
		return unpacked;

	// return null if not forested, or if not discovered
	e2::TileData* tile = getExistingTileData(hex);
	if (!tile || !tile->isForested() || tile->forestIndex < 0)
		return nullptr;

	e2::ForestState* originalForest = getForestState(tile->forestIndex);
	e2::UnpackedForestState newForest;
	newForest.hex = hex;
	newForest.trees.reserve(originalForest->trees.size());

	uint32_t i = 0;
	for (e2::TreeState& originalTree : originalForest->trees)
	{
		e2::MeshProxyConfiguration treeCfg;
		e2::MeshLodConfiguration lodCfg;
		lodCfg.mesh = m_treeMeshes[originalTree.meshIndex];
		lodCfg.materials.push(m_treeMaterialProxy);
		treeCfg.lods.push(lodCfg);

		e2::UnpackedTreeState newTree;
		newTree.meshIndex = originalTree.meshIndex;
		newTree.totalLumber = (uint32_t)(e2::randomFloat(3.5f * originalTree.scale, 4.5 * originalTree.scale)/ e2::treeScale);
		newTree.mesh = e2::create<e2::MeshProxy>(gameSession(), treeCfg);

		glm::vec3 fallDirection = glm::rotate(glm::identity<glm::quat>(), glm::radians(e2::randomFloat(-45.0f, 45.0f)), e2::worldUpf()) * -e2::worldRightf();
		newTree.fallDir = glm::vec2(fallDirection.x, fallDirection.z);
		newTree.rotation = originalTree.rotation + tile->forestRotation;
		newTree.scale = originalTree.scale;

		glm::vec2 treeOffset = e2::Hex(hex).planarCoords() + originalTree.localOffset(tile, originalForest);

		newTree.worldOffset = treeOffset;

		glm::mat4 treeTranslation = glm::translate(glm::identity<glm::mat4>(), glm::vec3(treeOffset.x, 0.0f, treeOffset.y));
		glm::mat4 treeScale = glm::scale(glm::identity<glm::mat4>(), glm::vec3(newTree.scale));
		glm::mat4 treeRotation = glm::rotate(glm::identity<glm::mat4>(), newTree.rotation, e2::worldUpf());

		glm::mat4 treeRotationFix = glm::rotate(glm::identity<glm::mat4>(), glm::radians(90.0f), e2::worldRightf());

		newTree.mesh->modelMatrix = treeTranslation * treeRotation * treeRotationFix * treeScale;
		newTree.mesh->modelMatrixDirty = { true };
		newForest.trees.push_back(newTree);


		/*
			glm::mat4 treeTranslation = glm::translate(glm::identity<glm::mat4>(), glm::vec3(treeState.planarOffset.x, 0.0f, treeState.planarOffset.y));
			glm::mat4 treeScale = glm::scale(glm::identity<glm::mat4>(), glm::vec3(treeState.scale));
			glm::mat4 treeRotation = glm::rotate(glm::identity<glm::mat4>(), treeState.rotation, e2::worldUpf());

			glm::mat4 treeRotationFix = glm::rotate(glm::identity<glm::mat4>(), glm::radians(90.0f), e2::worldRightf());

			glm::mat4 treeTransform = treeTranslation * treeRotation * treeRotationFix * treeScale;
		
		
		*/

	}

	removeWood(hex);

	m_unpackedForests[hex] = newForest;

	return getUnpackedForestState(hex);

}


void e2::HexGrid::updateUnpackedForests()
{
	static std::vector<glm::ivec2> removeList;

	removeList.clear();
	for (auto& pair : m_unpackedForests)
	{
		uint32_t deadTrees = 0;
		for (auto& t : pair.second.trees)
		{
			if (t.health <= 0.0f)
			{
				if (t.fallTime <= 0.0f)
				{
					if (t.killTime > 0.0f)
					{

						glm::vec3 fallRot = glm::vec3(t.fallDir.y, 0.0f, -t.fallDir.x);

						t.killTime -= game()->timeDelta();

						if (t.killTime < 0.4f && t.mesh)
						{
							e2::destroy(t.mesh);
							t.mesh = nullptr;
						}

						if (t.killTime < 0.3f)
						{
							
							if (t.spawnedLumber < t.totalLumber)
							{
								float nextTimeRef = (1.0f - (((float)t.spawnedLumber + 1.0f) / (float)t.totalLumber)) * 0.3f;
								if (t.killTime < nextTimeRef)
								{
									e2::ItemEntity* newLumber = game()->spawnEntity("lumber", glm::vec3(t.worldOffset.x, 0.0f, t.worldOffset.y) + fallRot * (t.spawnedLumber * 0.2f))->cast<e2::ItemEntity>();
									newLumber->setLifetime(4.0);
									newLumber->playSound("S_Wood_Spawn.e2a", 1.0f, 1.0f);
									t.spawnedLumber++;
								}
							}
						}
					}
					else
					{
						deadTrees++;
					}
				}
				else
				{
					t.fallTime -= game()->timeDelta();
										
					glm::mat4 treeTranslation = glm::translate(glm::identity<glm::mat4>(), glm::vec3(t.worldOffset.x, 0.0f, t.worldOffset.y));
					glm::mat4 treeScale = glm::scale(glm::identity<glm::mat4>(), glm::vec3(t.scale));
					glm::mat4 treeRotation = glm::rotate(glm::identity<glm::mat4>(), t.rotation, e2::worldUpf());

					glm::mat4 treeRotationFix = glm::rotate(glm::identity<glm::mat4>(), glm::radians(90.0f), e2::worldRightf());


					glm::mat4 shakeRotation = glm::identity<glm::mat4>();
					if (t.shake >= 0.0f)
					{
						float shakeDelta = glm::clamp(t.shake / 0.4f, 0.0f, 1.0f);
						glm::vec2 shakeDirection = e2::randomOnUnitCircle();
						shakeRotation = glm::rotate(glm::identity<glm::mat4>(), glm::radians(3.0f * shakeDelta), glm::vec3(shakeDirection.x, 0.0f, shakeDirection.y));
						t.shake -= game()->timeDelta();
					}

					float fallAlpha = (e2::maxTreeFallTime - glm::clamp(t.fallTime, 0.0f, e2::maxTreeFallTime)) / e2::maxTreeFallTime;
					float fallDelta = glm::circularEaseIn(glm::clamp(fallAlpha, 0.0f, 1.0f));
					glm::mat4 fallRotation = glm::rotate(glm::identity<glm::mat4>(), glm::radians(90.0f * fallDelta), glm::vec3(t.fallDir.x, 0.0f, t.fallDir.y));

					t.mesh->modelMatrix = treeTranslation * fallRotation * shakeRotation * treeRotation * treeRotationFix * treeScale;
					t.mesh->modelMatrixDirty = true;

					if (t.fallTime <= 0.0f)
					{
						game()->addScreenShake(0.2f);
					}
				}
			}
			else
			{
				if (t.shake > 0.0f)
				{
					glm::mat4 treeTranslation = glm::translate(glm::identity<glm::mat4>(), glm::vec3(t.worldOffset.x, 0.0f, t.worldOffset.y));
					glm::mat4 treeScale = glm::scale(glm::identity<glm::mat4>(), glm::vec3(t.scale));
					glm::mat4 treeRotation = glm::rotate(glm::identity<glm::mat4>(), t.rotation, e2::worldUpf());

					glm::mat4 treeRotationFix = glm::rotate(glm::identity<glm::mat4>(), glm::radians(90.0f), e2::worldRightf());

					float shakeDelta = glm::clamp(t.shake / 0.4f, 0.0f, 1.0f);
					glm::vec2 shakeDirection = e2::randomOnUnitCircle();
					glm::mat4 shakeRotation = glm::rotate(glm::identity<glm::mat4>(), glm::radians(3.0f * shakeDelta), glm::vec3(shakeDirection.x, 0.0f, shakeDirection.y));

					t.mesh->modelMatrix = treeTranslation * shakeRotation * treeRotation * treeRotationFix * treeScale;
					t.mesh->modelMatrixDirty = true;

					t.shake -= game()->timeDelta();
					if (t.shake <= 0.0f)
					{
						glm::mat4 treeTranslation = glm::translate(glm::identity<glm::mat4>(), glm::vec3(t.worldOffset.x, 0.0f, t.worldOffset.y));
						glm::mat4 treeScale = glm::scale(glm::identity<glm::mat4>(), glm::vec3(t.scale));
						glm::mat4 treeRotation = glm::rotate(glm::identity<glm::mat4>(), t.rotation, e2::worldUpf());

						glm::mat4 treeRotationFix = glm::rotate(glm::identity<glm::mat4>(), glm::radians(90.0f), e2::worldRightf());

						t.mesh->modelMatrix = treeTranslation * treeRotation * treeRotationFix * treeScale;
						t.mesh->modelMatrixDirty = true;
					}
				}
			}
		}

		if (deadTrees >= pair.second.trees.size())
		{
			removeList.push_back(pair.first);
		}
	}

	for (auto h : removeList)
	{
		m_unpackedForests.erase(h);
	}
}

e2::MeshProxy* e2::HexGrid::createGrassProxy(e2::TileData* tileData, glm::ivec2 const& hex)
{
	if (!tileData)
		return nullptr;

	e2::MeshProxyConfiguration meshConf;
	e2::MeshLodConfiguration lodConf;
	lodConf.mesh = m_grassMeshes[(hex.x * hex.x - (hex.y * 4)) % 4];
	meshConf.lods.push(lodConf);

	e2::MeshProxy* newMeshProxy = e2::create<e2::MeshProxy>(gameSession(), meshConf);

	glm::vec3 meshOffset = e2::Hex(hex).localCoords();

	glm::mat4 transform = glm::mat4(1.0f);
	transform = glm::translate(transform, meshOffset);

	newMeshProxy->modelMatrix = transform;
	newMeshProxy->modelMatrixDirty = { true };

	return newMeshProxy;
}

e2::MeshProxy* e2::HexGrid::createResourceProxyForTile(e2::TileData* tileData, glm::ivec2 const& hex)
{
	if (!tileData)
		return nullptr; 

	if (!tileData->resourceMesh)
		return nullptr;

	e2::MeshProxyConfiguration meshConf;
	e2::MeshLodConfiguration lodConf;
	lodConf.mesh = tileData->resourceMesh;
	meshConf.lods.push(lodConf);

	e2::MeshProxy* newMeshProxy = e2::create<e2::MeshProxy>(gameSession(), meshConf);

	if ((tileData->flags & e2::TileFlags::WaterMask) != e2::TileFlags::WaterNone)
	{


		glm::vec3 meshOffset = e2::Hex(hex).localCoords() + e2::worldUpf() * -1.0f;

		glm::mat4 transform = glm::mat4(1.0f);
		transform = glm::translate(transform, meshOffset);
		transform = glm::rotate(transform, glm::radians(e2::randomFloat(0.0f, 359.f)), glm::vec3(e2::worldUp()));
		transform = glm::scale(transform, glm::vec3(e2::randomFloat(1.4f, 2.2f)));
		newMeshProxy->modelMatrix = transform;
		newMeshProxy->modelMatrixDirty = { true };

		return newMeshProxy;
	}



	
	glm::vec3 meshOffset = e2::Hex(hex).localCoords();

	glm::mat4 transform = glm::mat4(1.0f);
	transform = glm::translate(transform, meshOffset);
	transform = glm::rotate(transform, glm::radians(hex.x * 20.0f), glm::vec3(e2::worldUp()));
	transform = glm::scale(transform, glm::vec3(0.675f));
	newMeshProxy->modelMatrix = transform;
	newMeshProxy->modelMatrixDirty = { true };

	return newMeshProxy;
}

e2::TileData e2::HexGrid::calculateTileData(glm::ivec2 const& hex)
{
	TileData newTileData;

	glm::vec2 planarCoords = e2::Hex(hex).planarCoords() * 1.05f;
	float baseHeight = calculateBaseHeight(planarCoords);
	calculateBiome(planarCoords, newTileData.flags);
	calculateFeaturesAndWater(planarCoords, baseHeight, newTileData.flags);
	calculateResources(planarCoords, newTileData.flags);
	newTileData.forestIndex = getForestIndexForFlags(newTileData.flags);
	newTileData.forestRotation = glm::radians(hex.x * hex.y * 1.12f);
	newTileData.resourceMesh = getResourceMeshForFlags(newTileData.flags);

	e2::TurnbasedEntity* existingStructure = game()->entityAtHex(EntityLayerIndex::Structure, hex);
	if (existingStructure)
	{
		newTileData.empireId = existingStructure->getEmpireId();
	}

	return newTileData;
}

e2::TileData e2::HexGrid::getTileData(glm::ivec2 const& hex)
{
	e2::TileData* src = getExistingTileData(hex);
	if (src)
		return *src;

	return calculateTileData(hex);
}

e2::MeshProxy* e2::HexGrid::createForestProxyForTile(e2::TileData* tileData, glm::ivec2 const& hex)
{
	if (!tileData)
		return nullptr;

	if (!tileData->isForested() || tileData->forestIndex < 0)
		return nullptr;
		
	e2::MeshProxyConfiguration proxyConfig{
		.lods = {
			{
				.maxDistance = 0.0f,
				.mesh = m_forestStates[tileData->forestIndex].mesh,
				.materials = {m_treeMaterialProxy}
			}
		}
	};

	e2::MeshProxy* newMeshProxy = e2::create<e2::MeshProxy>(gameSession(), proxyConfig);
	newMeshProxy->modelMatrix = glm::translate(glm::mat4(1.0f), e2::Hex(hex).localCoords());
	newMeshProxy->modelMatrix = glm::rotate(newMeshProxy->modelMatrix, tileData->forestRotation, glm::vec3(e2::worldUp()));
	newMeshProxy->modelMatrixDirty = { true };

	return newMeshProxy;
}

size_t e2::HexGrid::discover(glm::ivec2 const& hex)
{
	// there are no checks to see if hex is already discovered, use with care!! 
#if defined(E2_DEVELOPMENT)
	if (m_tileIndex.contains(hex))
	{
		LogError("DISCOVERING ALREADY DISCOVERED TILE, EXPECT BREAKAGE");
	}
#endif
	m_tiles.push_back(calculateTileData(hex));
	m_tileVisibility.push_back(0);
	m_tileIndex[hex] = m_tiles.size() - 1;

	glm::ivec2 chunkIndex = chunkIndexFromPlanarCoords(e2::Hex(hex).planarCoords());
	auto chunkFinder = m_discoveredChunks.find(chunkIndex);
	if (chunkFinder == m_discoveredChunks.end())
	{
		m_discoveredChunks.insert(chunkIndex);
		
		e2::Aabb2D chunkAabb = getChunkAabb(chunkIndex);
		for (glm::vec2 p : chunkAabb.points())
			m_discoveredChunksAABB.push(p);
	}
	
	
	e2::TileData* tileData = &m_tiles[m_tiles.size() - 1];

	flagChunkOutdated(chunkIndex);
	//
	//if (tileData->empireId < 255)
	//{
	//	game()->discoverEmpire(tileData->empireId);
	//}


	return m_tiles.size() - 1;
}

size_t e2::HexGrid::getTileIndexFromHex(glm::ivec2 const& hex)
{
	size_t tileIndex = 0;

	auto finder = m_tileIndex.find(hex);
	if (finder == m_tileIndex.end())
	{
		tileIndex = discover(hex);
	}
	else
	{
		tileIndex = finder->second;
	}

	return tileIndex;
}

e2::TileData* e2::HexGrid::getExistingTileData(glm::ivec2 const& hex)
{
	auto finder = m_tileIndex.find(hex);
	if (finder == m_tileIndex.end())
		return nullptr;

	return &m_tiles[finder->second];
}

e2::TileData& e2::HexGrid::getTileFromIndex(size_t index)
{
	static TileData empty;
	if (index > m_tiles.size())
	{
		LogError("out of range");
		return empty;
	}
	return m_tiles[index];
}

void e2::HexGrid::clearAllChunks()
{
	auto chunkIndexCopy = m_chunkIndex;
	for (auto [chunkIndex, chunkState] : chunkIndexCopy)
	{
		nukeChunk(chunkState);
	}

	m_chunkIndex.clear();
}

uint32_t e2::HexGrid::numChunks()
{
	return (uint32_t)m_chunkIndex.size();
}

uint32_t e2::HexGrid::numVisibleChunks()
{
	return (uint32_t)m_visibleChunks.size();
}

uint32_t e2::HexGrid::numJobsInFlight()
{
	return (uint32_t)m_streamingChunks.size();
}

uint32_t e2::HexGrid::numJobsInQueue()
{
	return (uint32_t)m_queuedChunks.size();
}

void e2::HexGrid::flagChunkOutdated(glm::ivec2 const& chunkIndex)
{
	m_outdatedChunks.insert(getOrCreateChunk(chunkIndex));
}

double e2::HexGrid::highLoadTime()
{
	return m_highLoadTime;
}

void e2::HexGrid::clearLoadTime()
{
	m_highLoadTime = 0.0;
}


void e2::HexGrid::initializeFogOfWar()
{
	auto renderCtx = game()->renderContext();
	auto mainThreadCtx = game()->mainThreadContext();

	// fogofwar stuff 
	e2::PipelineLayoutCreateInfo layInf{};
	layInf.pushConstantSize = sizeof(e2::FogOfWarConstants);
	m_fogOfWarPipelineLayout = renderCtx->createPipelineLayout(layInf);

	// outline stuff
	layInf = e2::PipelineLayoutCreateInfo();
	layInf.pushConstantSize = sizeof(e2::OutlineConstants);
	m_outlinePipelineLayout = renderCtx->createPipelineLayout(layInf);

	// blur stuff 
	e2::DescriptorSetLayoutCreateInfo setLayoutInf{};
	setLayoutInf.bindings = {
		{e2::DescriptorBindingType::Texture},
		{e2::DescriptorBindingType::Sampler},
	};
	m_blurSetLayout = renderCtx->createDescriptorSetLayout(setLayoutInf);

	layInf = e2::PipelineLayoutCreateInfo();
	layInf.pushConstantSize = sizeof(e2::BlurConstants);
	layInf.sets.push(m_blurSetLayout);
	m_blurPipelineLayout = renderCtx->createPipelineLayout(layInf);

	e2::DescriptorPoolCreateInfo poolCreateInfo{};
	poolCreateInfo.maxSets = 2 * e2::maxNumSessions;
	poolCreateInfo.numSamplers = 2 * e2::maxNumSessions;
	poolCreateInfo.numTextures = 2 * e2::maxNumSessions;
	m_blurPool = mainThreadCtx->createDescriptorPool(poolCreateInfo);

	// minimap stuff
	e2::DescriptorSetLayoutCreateInfo miniSetLayoutInf{};
	miniSetLayoutInf.bindings = {
		{e2::DescriptorBindingType::Texture},
		{e2::DescriptorBindingType::Texture},
		{e2::DescriptorBindingType::Texture},
		{e2::DescriptorBindingType::Texture},
		{e2::DescriptorBindingType::Texture},
		{e2::DescriptorBindingType::Sampler},
		{e2::DescriptorBindingType::Sampler},
	};
	m_minimapLayout = renderCtx->createDescriptorSetLayout(miniSetLayoutInf);

	e2::DescriptorPoolCreateInfo miniPoolCreateInfo{};
	miniPoolCreateInfo.maxSets = 2 * e2::maxNumSessions;
	miniPoolCreateInfo.numSamplers = 2 * e2::maxNumSessions * 2;
	miniPoolCreateInfo.numTextures = 2 * e2::maxNumSessions * 5;
	m_minimapPool = mainThreadCtx->createDescriptorPool(miniPoolCreateInfo);
	// 480x320
	//240x160
	m_minimapSize = {240, 160};
	e2::TextureCreateInfo minimapTexInf{};
	minimapTexInf.initialLayout = e2::TextureLayout::ShaderRead;
	minimapTexInf.format = TextureFormat::SRGB8A8;
	minimapTexInf.resolution = { m_minimapSize, 1 };
	m_frameData[0].minimapTexture = renderCtx->createTexture(minimapTexInf);
	m_frameData[1].minimapTexture = renderCtx->createTexture(minimapTexInf);

	e2::RenderTargetCreateInfo minimapTargetInfo{};
	minimapTargetInfo.areaExtent = m_minimapSize;

	e2::RenderAttachment minimapAttachment{};
	minimapAttachment.clearMethod = ClearMethod::ColorFloat;
	minimapAttachment.clearValue.clearColorf32 = { 0.f, 0.f, 0.f, 1.0f };
	minimapAttachment.loadOperation = LoadOperation::Clear;
	minimapAttachment.storeOperation = StoreOperation::Store;

	minimapAttachment.target = m_frameData[0].minimapTexture;
	minimapTargetInfo.colorAttachments = { minimapAttachment };
	m_frameData[0].minimapTarget = renderCtx->createRenderTarget(minimapTargetInfo);

	minimapAttachment.target = m_frameData[1].minimapTexture;
	minimapTargetInfo.colorAttachments = { minimapAttachment };
	m_frameData[1].minimapTarget = renderCtx->createRenderTarget(minimapTargetInfo);

	e2::PipelineLayoutCreateInfo minilayInf{};
	minilayInf.pushConstantSize = sizeof(e2::MiniMapConstants);
	minilayInf.sets = { m_minimapLayout };
	m_minimapPipelineLayout = renderCtx->createPipelineLayout(minilayInf);


	// mapvis stuff 
	glm::uvec2 mapVisSize = glm::vec2(m_minimapSize) * 0.25f;
	e2::TextureCreateInfo mapvisTexInf{};
	mapvisTexInf.initialLayout = e2::TextureLayout::ShaderRead;
	mapvisTexInf.format = TextureFormat::RGBA8;
	mapvisTexInf.resolution = { mapVisSize, 1 };
	m_frameData[0].mapVisTextures[0] = renderCtx->createTexture(mapvisTexInf);
	m_frameData[0].mapVisTextures[1] = renderCtx->createTexture(mapvisTexInf);
	m_frameData[1].mapVisTextures[0] = renderCtx->createTexture(mapvisTexInf);
	m_frameData[1].mapVisTextures[1] = renderCtx->createTexture(mapvisTexInf);

	e2::RenderTargetCreateInfo mapVisTargetInfo{};
	mapVisTargetInfo.areaExtent = mapVisSize;

	e2::RenderAttachment mapVisAttachemnt{};
	mapVisAttachemnt.clearMethod = ClearMethod::ColorFloat;
	mapVisAttachemnt.clearValue.clearColorf32 = { 0.f, 0.f, 0.f, 0.0f };
	mapVisAttachemnt.loadOperation = LoadOperation::Clear;
	mapVisAttachemnt.storeOperation = StoreOperation::Store;

	mapVisAttachemnt.target = m_frameData[0].mapVisTextures[0];
	mapVisTargetInfo.colorAttachments = { mapVisAttachemnt };
	m_frameData[0].mapVisTargets[0] = renderCtx->createRenderTarget(mapVisTargetInfo);


	mapVisAttachemnt.target = m_frameData[0].mapVisTextures[1];
	mapVisTargetInfo.colorAttachments = { mapVisAttachemnt };
	m_frameData[0].mapVisTargets[1] = renderCtx->createRenderTarget(mapVisTargetInfo);


	mapVisAttachemnt.target = m_frameData[1].mapVisTextures[0];
	mapVisTargetInfo.colorAttachments = { mapVisAttachemnt };
	m_frameData[1].mapVisTargets[0] = renderCtx->createRenderTarget(mapVisTargetInfo);


	mapVisAttachemnt.target = m_frameData[1].mapVisTextures[1];
	mapVisTargetInfo.colorAttachments = { mapVisAttachemnt };
	m_frameData[1].mapVisTargets[1] = renderCtx->createRenderTarget(mapVisTargetInfo);



	// Map units 
	e2::TextureCreateInfo unitTexInf{};
	unitTexInf.initialLayout = e2::TextureLayout::ShaderRead;
	unitTexInf.format = TextureFormat::RGBA8;
	unitTexInf.resolution = { m_minimapSize, 1 };
	m_frameData[0].mapUnitsTexture = renderCtx->createTexture(unitTexInf);
	m_frameData[1].mapUnitsTexture = renderCtx->createTexture(unitTexInf);

	e2::RenderTargetCreateInfo unitTargetInfo{};
	unitTargetInfo.areaExtent = m_minimapSize;

	e2::RenderAttachment unitAttachment{};
	unitAttachment.clearMethod = ClearMethod::ColorFloat;
	unitAttachment.clearValue.clearColorf32 = { 0.f, 0.f, 0.f, 0.0f };
	unitAttachment.loadOperation = LoadOperation::Clear;
	unitAttachment.storeOperation = StoreOperation::Store;

	unitAttachment.target = m_frameData[0].mapUnitsTexture;
	unitTargetInfo.colorAttachments = { unitAttachment };
	m_frameData[0].mapUnitsTarget = renderCtx->createRenderTarget(unitTargetInfo);

	unitAttachment.target = m_frameData[1].mapUnitsTexture;
	unitTargetInfo.colorAttachments = { unitAttachment };
	m_frameData[1].mapUnitsTarget = renderCtx->createRenderTarget(unitTargetInfo);


	auto rm = game()->renderManager();


	// setup descriptor sets that binds mapvis to minimap 
	m_frameData[0].minimapSet = m_minimapPool->createDescriptorSet(m_minimapLayout);
	m_frameData[1].minimapSet = m_minimapPool->createDescriptorSet(m_minimapLayout);

	m_frameData[0].minimapSet->writeTexture(0, m_frameData[0].mapVisTextures[0]); // set it to texture0 since that's the one thatll be used (render->blur->blur)
	m_frameData[0].minimapSet->writeTexture(1, m_frameData[0].mapUnitsTexture);
	m_frameData[0].minimapSet->writeTexture(2, m_waterTexture->handle());
	m_frameData[0].minimapSet->writeTexture(3, m_forestTexture->handle());
	m_frameData[0].minimapSet->writeTexture(4, m_mountainTexture->handle());
	m_frameData[0].minimapSet->writeSampler(5, rm->clampSampler());
	m_frameData[0].minimapSet->writeSampler(6, rm->repeatSampler());

	m_frameData[1].minimapSet->writeTexture(0, m_frameData[1].mapVisTextures[0]); // set it to texture0 since that's the one thatll be used (render->blur->blur)
	m_frameData[1].minimapSet->writeTexture(1, m_frameData[1].mapUnitsTexture);
	m_frameData[1].minimapSet->writeTexture(2, m_waterTexture->handle());
	m_frameData[1].minimapSet->writeTexture(3, m_forestTexture->handle());
	m_frameData[1].minimapSet->writeTexture(4, m_mountainTexture->handle());
	m_frameData[1].minimapSet->writeSampler(5, rm->clampSampler());
	m_frameData[1].minimapSet->writeSampler(6, rm->repeatSampler());


	// setup descriptor sets that binds fogofwar masks for the blur shader 
	m_frameData[0].fogOfWarMaskBlurSets[0] = m_blurPool->createDescriptorSet(m_blurSetLayout);
	m_frameData[0].fogOfWarMaskBlurSets[1] = m_blurPool->createDescriptorSet(m_blurSetLayout);
	m_frameData[1].fogOfWarMaskBlurSets[0] = m_blurPool->createDescriptorSet(m_blurSetLayout);
	m_frameData[1].fogOfWarMaskBlurSets[1] = m_blurPool->createDescriptorSet(m_blurSetLayout);

	// setup descriptor sets that binds mapvis texture for the blur shader 
	m_frameData[0].mapVisBlurSets[0] = m_blurPool->createDescriptorSet(m_blurSetLayout);
	m_frameData[0].mapVisBlurSets[1] = m_blurPool->createDescriptorSet(m_blurSetLayout);
	m_frameData[1].mapVisBlurSets[0] = m_blurPool->createDescriptorSet(m_blurSetLayout);
	m_frameData[1].mapVisBlurSets[1] = m_blurPool->createDescriptorSet(m_blurSetLayout);

	// can just write to mapvis blur sets here because we never resize these textures
	m_frameData[0].mapVisBlurSets[0]->writeTexture(0, m_frameData[0].mapVisTextures[0]);
	m_frameData[0].mapVisBlurSets[1]->writeTexture(0, m_frameData[0].mapVisTextures[1]);
	m_frameData[0].mapVisBlurSets[0]->writeSampler(1, rm->clampSampler());
	m_frameData[0].mapVisBlurSets[1]->writeSampler(1, rm->clampSampler());
	m_frameData[1].mapVisBlurSets[0]->writeTexture(0, m_frameData[1].mapVisTextures[0]);
	m_frameData[1].mapVisBlurSets[1]->writeTexture(0, m_frameData[1].mapVisTextures[1]);
	m_frameData[1].mapVisBlurSets[0]->writeSampler(1, rm->clampSampler());
	m_frameData[1].mapVisBlurSets[1]->writeSampler(1, rm->clampSampler());




	// setup command buffers
	m_fogOfWarCommandBuffers[0] = rm->framePool(0)->createBuffer({});
	m_fogOfWarCommandBuffers[1] = rm->framePool(1)->createBuffer({});


	invalidateFogOfWarShaders();
	invalidateFogOfWarRenderTarget({192,108});
}

void e2::HexGrid::invalidateFogOfWarRenderTarget(glm::uvec2 const& newResolution)
{
	if (m_frameData[0].outlineTarget)
		e2::discard(m_frameData[0].outlineTarget);
	if (m_frameData[1].outlineTarget)
		e2::discard(m_frameData[1].outlineTarget);

	if (m_frameData[0].outlineTexture)
		e2::discard(m_frameData[0].outlineTexture);
	if (m_frameData[1].outlineTexture)
		e2::discard(m_frameData[1].outlineTexture);

	if (m_frameData[0].fogOfWarMasks[0])
		e2::discard(m_frameData[0].fogOfWarMasks[0]);
	if (m_frameData[0].fogOfWarMasks[1])
		e2::discard(m_frameData[0].fogOfWarMasks[1]);
	if (m_frameData[1].fogOfWarMasks[0])
		e2::discard(m_frameData[1].fogOfWarMasks[0]);
	if (m_frameData[1].fogOfWarMasks[1])
		e2::discard(m_frameData[1].fogOfWarMasks[1]);

	if (m_frameData[0].fogOfWarTargets[0])
		e2::discard(m_frameData[0].fogOfWarTargets[0]);
	if (m_frameData[0].fogOfWarTargets[1])
		e2::discard(m_frameData[0].fogOfWarTargets[1]);
	if (m_frameData[1].fogOfWarTargets[0])
		e2::discard(m_frameData[1].fogOfWarTargets[0]);
	if (m_frameData[1].fogOfWarTargets[1])
		e2::discard(m_frameData[1].fogOfWarTargets[1]);

	auto renderCtx = game()->renderContext();

	float resDiv = 8;

	e2::TextureCreateInfo texInf{};
	texInf.initialLayout = e2::TextureLayout::ShaderRead;
	texInf.format = TextureFormat::RGBA8;
	texInf.resolution = { glm::vec2(newResolution) / resDiv, 1 };
	texInf.mips = 1;
	m_frameData[0].fogOfWarMasks[0] = renderCtx->createTexture(texInf);
	m_frameData[0].fogOfWarMasks[1] = renderCtx->createTexture(texInf);
	m_frameData[1].fogOfWarMasks[0] = renderCtx->createTexture(texInf);
	m_frameData[1].fogOfWarMasks[1] = renderCtx->createTexture(texInf);


	e2::RenderTargetCreateInfo renderTargetInfo{};
	renderTargetInfo.areaExtent = glm::vec2(newResolution) / resDiv;

	e2::RenderAttachment colorAttachment{};
	colorAttachment.clearMethod = ClearMethod::ColorFloat;
	colorAttachment.clearValue.clearColorf32 = { 0.f, 0.f, 0.f, 1.0f };
	colorAttachment.loadOperation = LoadOperation::Clear;
	colorAttachment.storeOperation = StoreOperation::Store;

	colorAttachment.target = m_frameData[0].fogOfWarMasks[0];
	renderTargetInfo.colorAttachments = { colorAttachment };
	m_frameData[0].fogOfWarTargets[0] = renderCtx->createRenderTarget(renderTargetInfo);

	colorAttachment.target = m_frameData[0].fogOfWarMasks[1];
	renderTargetInfo.colorAttachments = { colorAttachment };
	m_frameData[0].fogOfWarTargets[1] = renderCtx->createRenderTarget(renderTargetInfo);

	colorAttachment.target = m_frameData[1].fogOfWarMasks[0];
	renderTargetInfo.colorAttachments = { colorAttachment };
	m_frameData[1].fogOfWarTargets[0] = renderCtx->createRenderTarget(renderTargetInfo);

	colorAttachment.target = m_frameData[1].fogOfWarMasks[1];
	renderTargetInfo.colorAttachments = { colorAttachment };
	m_frameData[1].fogOfWarTargets[1] = renderCtx->createRenderTarget(renderTargetInfo);



	m_fogProxy->visibilityMasks[0].set(m_frameData[0].fogOfWarMasks[0]);
	m_fogProxy->visibilityMasks[1].set(m_frameData[1].fogOfWarMasks[0]);

	m_waterProxy->visibilityMasks[0].set(m_frameData[0].fogOfWarMasks[0]);
	m_waterProxy->visibilityMasks[1].set(m_frameData[1].fogOfWarMasks[0]);

	//m_waterProxy->visibilityMask.set(m_fogOfWarMask[0]);
	//m_terrainProxy->visibilityMask.set(m_fogOfWarMask[0]);

	auto rm = game()->renderManager();
	
	m_frameData[0].fogOfWarMaskBlurSets[0]->writeTexture(0, m_frameData[0].fogOfWarMasks[0]);
	m_frameData[0].fogOfWarMaskBlurSets[1]->writeTexture(0, m_frameData[0].fogOfWarMasks[1]);
	m_frameData[0].fogOfWarMaskBlurSets[0]->writeSampler(1, rm->clampSampler());
	m_frameData[0].fogOfWarMaskBlurSets[1]->writeSampler(1, rm->clampSampler());
	m_frameData[1].fogOfWarMaskBlurSets[0]->writeTexture(0, m_frameData[1].fogOfWarMasks[0]);
	m_frameData[1].fogOfWarMaskBlurSets[1]->writeTexture(0, m_frameData[1].fogOfWarMasks[1]);
	m_frameData[1].fogOfWarMaskBlurSets[0]->writeSampler(1, rm->clampSampler());
	m_frameData[1].fogOfWarMaskBlurSets[1]->writeSampler(1, rm->clampSampler());


	// outline target
	texInf.initialLayout = e2::TextureLayout::ShaderRead;
	texInf.format = TextureFormat::RGBA8;
	texInf.resolution = { newResolution, 1 };
	texInf.mips = 1;
	m_frameData[0].outlineTexture = renderCtx->createTexture(texInf);
	m_frameData[1].outlineTexture = renderCtx->createTexture(texInf);

	renderTargetInfo.areaExtent = newResolution;
	colorAttachment.clearMethod = ClearMethod::ColorFloat;
	colorAttachment.clearValue.clearColorf32 = { 0.f, 0.f, 0.f, 0.0f };
	colorAttachment.loadOperation = LoadOperation::Clear;
	colorAttachment.storeOperation = StoreOperation::Store;

	colorAttachment.target = m_frameData[0].outlineTexture;
	renderTargetInfo.colorAttachments = { colorAttachment };
	m_frameData[0].outlineTarget = renderCtx->createRenderTarget(renderTargetInfo);

	colorAttachment.target = m_frameData[1].outlineTexture;
	renderTargetInfo.colorAttachments = { colorAttachment };
	m_frameData[1].outlineTarget = renderCtx->createRenderTarget(renderTargetInfo);
}

void e2::HexGrid::invalidateFogOfWarShaders()
{
	auto renderCtx = game()->renderContext();
	auto rm = game()->renderManager();

	if (m_fogOfWarVertexShader)
		e2::discard(m_fogOfWarVertexShader);

	if (m_fogOfWarFragmentShader)
		e2::discard(m_fogOfWarVertexShader);

	if (m_fogOfWarPipeline)
		e2::discard(m_fogOfWarPipeline);

	std::string vertexSource;
	if (!e2::readFileWithIncludes("shaders/fogofwar.vertex.glsl", vertexSource))
	{
		LogError("Failed to read shader file.");
	}

	e2::ShaderCreateInfo shdrInf{};
	e2::applyVertexAttributeDefines(m_baseHex->specification(0).attributeFlags, shdrInf);

	shdrInf.source = vertexSource.c_str();
	shdrInf.stage = ShaderStage::Vertex;
	m_fogOfWarVertexShader = renderCtx->createShader(shdrInf);

	std::string fragmentSource;
	if (!e2::readFileWithIncludes("shaders/fogofwar.fragment.glsl", fragmentSource))
	{
		LogError("Failed to read shader file.");
	}

	shdrInf.source = fragmentSource.c_str();
	shdrInf.stage = ShaderStage::Fragment;
	m_fogOfWarFragmentShader = renderCtx->createShader(shdrInf);

	e2::PipelineCreateInfo pipeInf{};
	pipeInf.shaders.push(m_fogOfWarVertexShader);
	pipeInf.shaders.push(m_fogOfWarFragmentShader);
	pipeInf.colorFormats = { e2::TextureFormat::RGBA8 };
	pipeInf.layout = m_fogOfWarPipelineLayout;
	m_fogOfWarPipeline = renderCtx->createPipeline(pipeInf);








	if (m_blurFragmentShader)
		e2::discard(m_blurFragmentShader);

	if (m_blurPipeline)
		e2::discard(m_blurPipeline);

	std::string blurSource;
	if (!e2::readFileWithIncludes("shaders/blur.fragment.glsl", blurSource))
	{
		LogError("Failed to read shader file.");
	}

	shdrInf = e2::ShaderCreateInfo();
	shdrInf.source = blurSource.c_str();
	shdrInf.stage = ShaderStage::Fragment;
	m_blurFragmentShader = renderCtx->createShader(shdrInf);


	pipeInf = e2::PipelineCreateInfo();
	pipeInf.shaders.push(rm->fullscreenTriangleShader());
	pipeInf.shaders.push(m_blurFragmentShader);
	pipeInf.colorFormats = { e2::TextureFormat::RGBA8 };
	pipeInf.layout = m_blurPipelineLayout;
	m_blurPipeline = renderCtx->createPipeline(pipeInf);





	if (m_outlineVertexShader)
		e2::discard(m_outlineVertexShader);

	if (m_outlineFragmentShader)
		e2::discard(m_outlineFragmentShader);

	if (m_outlinePipeline)
		e2::discard(m_outlinePipeline);

	std::string outlineVertexSource;
	if (!e2::readFileWithIncludes("shaders/outline.vertex.glsl", outlineVertexSource))
	{
		LogError("Failed to read shader file.");
	}

	shdrInf = e2::ShaderCreateInfo();
	shdrInf.source = outlineVertexSource.c_str();
	shdrInf.stage = ShaderStage::Vertex;
	m_outlineVertexShader = renderCtx->createShader(shdrInf);

	std::string outlineFragmentSource;
	if (!e2::readFileWithIncludes("shaders/outline.fragment.glsl", outlineFragmentSource))
	{
		LogError("Failed to read shader file.");
	}

	shdrInf = e2::ShaderCreateInfo();
	shdrInf.source = outlineFragmentSource.c_str();
	shdrInf.stage = ShaderStage::Fragment;
	m_outlineFragmentShader = renderCtx->createShader(shdrInf);


	pipeInf = e2::PipelineCreateInfo();
	pipeInf.shaders.push(m_outlineVertexShader);
	pipeInf.shaders.push(m_outlineFragmentShader);
	pipeInf.colorFormats = { e2::TextureFormat::RGBA8 };
	pipeInf.layout = m_outlinePipelineLayout;
	m_outlinePipeline = renderCtx->createPipeline(pipeInf);



	if (m_minimapFragmentShader)
		e2::discard(m_minimapFragmentShader);

	if (m_minimapPipeline)
		e2::discard(m_minimapPipeline);

	std::string miniSourceData;
	if (!e2::readFileWithIncludes("shaders/minimap.fragment.glsl", miniSourceData))
	{
		LogError("Failed to read shader file.");
	}

	e2::ShaderCreateInfo miniShdrInf{};
	miniShdrInf.source = miniSourceData.c_str();
	miniShdrInf.stage = ShaderStage::Fragment;
	m_minimapFragmentShader = renderCtx->createShader(miniShdrInf);

	e2::PipelineCreateInfo miniPipeInfo{};
	miniPipeInfo.shaders.push(rm->fullscreenTriangleShader());
	miniPipeInfo.shaders.push(m_minimapFragmentShader);
	miniPipeInfo.colorFormats = { e2::TextureFormat::SRGB8A8 };
	miniPipeInfo.layout = m_minimapPipelineLayout;
	m_minimapPipeline = renderCtx->createPipeline(miniPipeInfo);

}

void e2::HexGrid::renderFogOfWar()
{
	auto rm = game()->renderManager();
	uint8_t frameIndex = rm->frameIndex();
	FrameData& frameData = m_frameData[frameIndex];

	glm::uvec2 newResolution = m_streamingView.resolution;
	if (newResolution != m_fogOfWarMaskSize || !m_frameData[0].fogOfWarMasks[0])
	{
		invalidateFogOfWarRenderTarget(newResolution);
		m_fogOfWarMaskSize = newResolution;
	}

	e2::SubmeshSpecification const& hexSpec = m_baseHex->specification(0);

	// outlines first 
	e2::OutlineConstants outlineConstants;
	glm::mat4 vpMatrix = m_streamingView.view.calculateProjectionMatrix(m_streamingView.resolution) * m_streamingView.view.calculateViewMatrix();

	e2::ICommandBuffer* buff = m_fogOfWarCommandBuffers[frameIndex];
	e2::PipelineSettings defaultSettings;
	defaultSettings.frontFace = e2::FrontFace::CCW;
	buff->beginRecord(true, defaultSettings);
	buff->useAsAttachment(frameData.outlineTexture);
	buff->beginRender(frameData.outlineTarget);
	buff->bindPipeline(m_outlinePipeline);
	buff->bindVertexLayout(hexSpec.vertexLayout);
	buff->bindIndexBuffer(hexSpec.indexBuffer);
	for (uint8_t i = 0; i < hexSpec.vertexAttributes.size(); i++)
		buff->bindVertexBuffer(i, hexSpec.vertexAttributes[i]);



	const glm::vec4 layerColors[2] = {
		{ 0.0f, 0.5f, 0.15f, 0.1f },
		{ 0.5f, 0.0f, 0.15f, 0.1f }
	};

	for (uint8_t i = 0; i < size_t(e2::OutlineLayer::Count); i++)
	{

		// base
		for (glm::ivec2 const& hexIndex : m_outlineTiles[i])
		{
			//e2::TileData* td = getExistingTileData(hexIndex);
			glm::vec3 worldOffset = e2::Hex(hexIndex).localCoords();
			//if (td && td->getWater() != e2::TileFlags::WaterNone)
			//	worldOffset.y += 0.1f;

			glm::mat4 transform = glm::identity<glm::mat4>();
			transform = glm::translate(transform, worldOffset);
			transform = glm::scale(transform, { 1.1f, 1.1f, 1.1f });


			outlineConstants.mvpMatrix = vpMatrix * transform;
			outlineConstants.color = e2::gameAccent.toVec4();

			buff->pushConstants(m_outlinePipelineLayout, 0, sizeof(e2::OutlineConstants), reinterpret_cast<uint8_t*>(&outlineConstants));
			buff->draw(hexSpec.indexCount, 1);
		}
		// subtractive
		for (glm::ivec2 const& hexIndex : m_outlineTiles[i])
		{
			//e2::TileData* td = getExistingTileData(hexIndex);
			glm::vec3 worldOffset = e2::Hex(hexIndex).localCoords();

			//if (td && td->getWater() != e2::TileFlags::WaterNone)
			//	worldOffset.y += 0.1f;

			glm::mat4 transform = glm::identity<glm::mat4>();
			transform = glm::translate(transform, worldOffset);
			transform = glm::scale(transform, { 1.04f, 1.04f, 1.04f });

			outlineConstants.mvpMatrix = vpMatrix * transform;
			outlineConstants.color = {};

			buff->pushConstants(m_outlinePipelineLayout, 0, sizeof(e2::OutlineConstants), reinterpret_cast<uint8_t*>(&outlineConstants));
			buff->draw(hexSpec.indexCount, 1);
		}
	}

	buff->endRender();
	buff->useAsDefault(frameData.outlineTexture);







	//fog of war
	e2::FogOfWarConstants fogOfWarConstants;

	buff->useAsAttachment(frameData.fogOfWarMasks[0]);
	buff->beginRender(frameData.fogOfWarTargets[0]);
	buff->bindPipeline(m_fogOfWarPipeline);

	// Bind vertex states
	buff->bindVertexLayout(hexSpec.vertexLayout);
	buff->bindIndexBuffer(hexSpec.indexBuffer);
	for (uint8_t i = 0; i < hexSpec.vertexAttributes.size(); i++)
		buff->bindVertexBuffer(i, hexSpec.vertexAttributes[i]);

	for (e2::ChunkState* chunk : m_chunksInView)
	{
		glm::ivec2 chunkIndex = chunk->chunkIndex;

		glm::ivec2 chunkTileOffset = chunkIndex * glm::ivec2(e2::hexChunkResolution);

		glm::mat4 vpMatrix = m_streamingView.view.calculateProjectionMatrix(m_streamingView.resolution) * m_streamingView.view.calculateViewMatrix();

		for (int32_t y = 0; y < e2::hexChunkResolution; y++)
		{
			for (int32_t x = 0; x < e2::hexChunkResolution; x++)
			{
				glm::ivec2 worldIndex = chunkTileOffset + glm::ivec2(x, y);
				auto finder = m_tileIndex.find(worldIndex);
				if (finder == m_tileIndex.end())
					continue;

				e2::Hex currentHex(worldIndex);
				glm::vec3 worldOffset = currentHex.localCoords();

				glm::mat4 transform = glm::identity<glm::mat4>();
				transform = glm::translate(transform, worldOffset);

				float spooky = m_tiles[finder->second].getWoodAbundanceAsFloat() / 4.0f;
				
				fogOfWarConstants.mvpMatrix = vpMatrix * transform;

				if (m_debugDraw)
					fogOfWarConstants.visibility = {1.0f, 1.0f, 0.0f, 1.0f};
				else
				{
					fogOfWarConstants.visibility.x = 1.0;
					fogOfWarConstants.visibility.y = m_tileVisibility[finder->second] > 0.0f ? 1.0f : 0.0f;
					fogOfWarConstants.visibility.z = spooky;
					fogOfWarConstants.visibility.w = m_tiles[finder->second].getWater() == TileFlags::WaterDeep ? 1.0f : 0.0f;
				}


				buff->pushConstants(m_fogOfWarPipelineLayout, 0, sizeof(e2::FogOfWarConstants), reinterpret_cast<uint8_t*>(&fogOfWarConstants));
				buff->draw(hexSpec.indexCount, 1);
			}
		}


	}
	buff->endRender();	
	buff->useAsDefault(frameData.fogOfWarMasks[0]);


	buff->setFrontFace(e2::FrontFace::CW);
	
	// blur 
	BlurConstants bc;

	uint32_t numBlurs = 24;
	for (uint32_t i = 0; i < numBlurs; i++)
	{
		bc.direction = e2::rotate2d({ 0.0f, 1.0f }, i * 360.0f / float(numBlurs));
		uint32_t backIndex = i % 2 == 0 ? 1 : 0;
		uint32_t frontIndex = backIndex == 0 ? 1 : 0;
		buff->useAsAttachment(frameData.fogOfWarMasks[backIndex]);
		buff->beginRender(frameData.fogOfWarTargets[backIndex]);
		buff->bindPipeline(m_blurPipeline);
		buff->nullVertexLayout();
		buff->bindDescriptorSet(m_blurPipelineLayout, 0, frameData.fogOfWarMaskBlurSets[frontIndex]);
		buff->pushConstants(m_blurPipelineLayout, 0, sizeof(e2::BlurConstants), reinterpret_cast<uint8_t*>(&bc));
		buff->drawNonIndexed(3, 1);
		buff->endRender();
		buff->useAsDefault(frameData.fogOfWarMasks[backIndex]);
	}

	/*

	bc.direction = { 1.0f, 0.0f };
	buff->useAsAttachment(frameData.fogOfWarMasks[1]);
	buff->beginRender(frameData.fogOfWarTargets[1]);
	buff->bindPipeline(m_blurPipeline);
	buff->nullVertexLayout();
	buff->bindDescriptorSet(m_blurPipelineLayout, 0, frameData.fogOfWarMaskBlurSets[0]);
	buff->pushConstants(m_blurPipelineLayout, 0, sizeof(e2::BlurConstants), reinterpret_cast<uint8_t*>(&bc));
	buff->drawNonIndexed(3, 1);
	buff->endRender();
	buff->useAsDefault(frameData.fogOfWarMasks[1]);

	bc.direction = { 0.0f, 1.0f };
	buff->useAsAttachment(frameData.fogOfWarMasks[0]);
	buff->beginRender(frameData.fogOfWarTargets[0]);
	buff->bindPipeline(m_blurPipeline);
	buff->nullVertexLayout();
	buff->bindDescriptorSet(m_blurPipelineLayout, 0, frameData.fogOfWarMaskBlurSets[1]);
	buff->pushConstants(m_blurPipelineLayout, 0, sizeof(e2::BlurConstants), reinterpret_cast<uint8_t*>(&bc));
	buff->drawNonIndexed(3, 1);
	buff->endRender();
	buff->useAsDefault(frameData.fogOfWarMasks[0]);
	*/

	// minimap




	// update minimap view bounds 
	updateViewBounds();

	// refresh these variables as we use them fuirther down 
	glm::vec2 worldSize = m_minimapViewBounds.max - m_minimapViewBounds.min;
	glm::vec2 worldOffset = m_minimapViewBounds.min;
	glm::vec2 worldCenter = worldOffset + worldSize / 2.0f;

	glm::vec2 cs = chunkSize();
	glm::vec2 csPixels = (cs / worldSize) * glm::vec2(m_minimapSize);


	e2::UIQuadPushConstants visConstants;
	visConstants.quadColor = {1.0f, 1.0f, 1.0f, 1.0f};
	visConstants.quadSize = csPixels;
	visConstants.surfaceSize = m_minimapSize;
	visConstants.quadZ = 0.0f;

	
	e2::UIManager* ui = game()->uiManager();

	// map vis 
	buff->useAsAttachment(frameData.mapVisTextures[0]);
	buff->beginRender(frameData.mapVisTargets[0]);

	buff->bindVertexLayout(ui->quadVertexLayout);
	buff->bindIndexBuffer(ui->quadIndexBuffer);
	buff->bindVertexBuffer(0, ui->quadVertexBuffer);
	buff->bindPipeline(ui->quadPipeline.pipeline);

	for (glm::ivec2 const& chunkIndex : m_discoveredChunks) 
	{
		glm::vec3 chunkPositionWorld = chunkOffsetFromIndex(chunkIndex);
		glm::vec2 planarChunkPosition{chunkPositionWorld.x, chunkPositionWorld.z};
		planarChunkPosition -= cs/2.0f;

		glm::vec2 chunkPosNormalized = (planarChunkPosition - worldOffset) / worldSize;
		glm::vec2 chunkPosPixels = chunkPosNormalized * glm::vec2(m_minimapSize);

		visConstants.quadPosition = chunkPosPixels;
		buff->pushConstants(ui->quadPipeline.layout, 0, sizeof(e2::UIQuadPushConstants), reinterpret_cast<uint8_t*>(&visConstants));

		buff->draw(6, 1);
	}

	buff->endRender();
	buff->useAsDefault(frameData.mapVisTextures[0]);



	// map vis blur 
	bc.direction = { 1.0f, 0.0f };
	buff->useAsAttachment(frameData.mapVisTextures[1]);
	buff->beginRender(frameData.mapVisTargets[1]);
	buff->bindPipeline(m_blurPipeline);
	buff->nullVertexLayout();
	buff->bindDescriptorSet(m_blurPipelineLayout, 0, frameData.mapVisBlurSets[0]);
	buff->pushConstants(m_blurPipelineLayout, 0, sizeof(e2::BlurConstants), reinterpret_cast<uint8_t*>(&bc));
	buff->drawNonIndexed(3, 1);
	buff->endRender();
	buff->useAsDefault(frameData.mapVisTextures[1]);

	bc.direction = { 0.0f, 1.0f };
	buff->useAsAttachment(frameData.mapVisTextures[0]);
	buff->beginRender(frameData.mapVisTargets[0]);
	buff->bindPipeline(m_blurPipeline);
	buff->nullVertexLayout();
	buff->bindDescriptorSet(m_blurPipelineLayout, 0, frameData.mapVisBlurSets[1]);
	buff->pushConstants(m_blurPipelineLayout, 0, sizeof(e2::BlurConstants), reinterpret_cast<uint8_t*>(&bc));
	buff->drawNonIndexed(3, 1);
	buff->endRender();
	buff->useAsDefault(frameData.mapVisTextures[0]);


	// map units 
	e2::UIQuadPushConstants unitConstants;
	unitConstants.quadColor = e2::UIColor(0xf59b14FF).toVec4();
	unitConstants.quadSize = {3.0f, 3.0f};
	unitConstants.surfaceSize = m_minimapSize;
	unitConstants.quadZ = 0.0f;

	buff->useAsAttachment(frameData.mapUnitsTexture);
	buff->beginRender(frameData.mapUnitsTarget);

	buff->bindVertexLayout(ui->quadVertexLayout);
	buff->bindIndexBuffer(ui->quadIndexBuffer);
	buff->bindVertexBuffer(0, ui->quadVertexBuffer);
	buff->bindPipeline(ui->quadPipeline.pipeline);

	e2::GameEmpire* localEmpire = game()->localEmpire();
	if (localEmpire)
	{
		for (e2::TurnbasedEntity* entity: localEmpire->entities)
		{
			glm::vec2 planarUnitPosition = entity->planarCoords();

			glm::vec2 unitPosNormalized = (planarUnitPosition - worldOffset) / worldSize;
			glm::vec2 unitPosPixels = unitPosNormalized * glm::vec2(m_minimapSize);

			if (entity->turnbasedSpecification()->layerIndex == EntityLayerIndex::Structure)
				unitConstants.quadSize = { 6.0f, 6.0f };
			else 
				unitConstants.quadSize = { 3.0f, 3.0f };

			unitConstants.quadPosition = unitPosPixels - (unitConstants.quadSize / 2.0f);
			buff->pushConstants(ui->quadPipeline.layout, 0, sizeof(e2::UIQuadPushConstants), reinterpret_cast<uint8_t*>(&unitConstants));

			buff->draw(6, 1);
		}

	}


	buff->endRender();
	buff->useAsDefault(frameData.mapUnitsTexture);



	// actual minimap
	e2::MiniMapConstants minimapConstants;
	minimapConstants.viewCornerTL = m_streamingView.topLeft;
	minimapConstants.viewCornerTR = m_streamingView.topRight;
	minimapConstants.viewCornerBL = m_streamingView.bottomLeft;
	minimapConstants.viewCornerBR = m_streamingView.bottomRight;
	minimapConstants.resolution = m_minimapSize;
	minimapConstants.worldMin = m_minimapViewBounds.min;
	minimapConstants.worldMax = m_minimapViewBounds.max;

	buff->useAsAttachment(frameData.minimapTexture);
	buff->beginRender(frameData.minimapTarget);
	buff->bindPipeline(m_minimapPipeline);

	// Bind vertex states
	buff->nullVertexLayout();
	buff->bindDescriptorSet(m_minimapPipelineLayout, 0, frameData.minimapSet);
	buff->pushConstants(m_minimapPipelineLayout, 0, sizeof(e2::MiniMapConstants), reinterpret_cast<uint8_t*>(&minimapConstants));
	buff->drawNonIndexed(3, 1);


	buff->endRender();

	buff->useAsDefault(frameData.minimapTexture);




	buff->endRecord();

	game()->renderManager()->queue(buff, nullptr, nullptr);
}

void e2::HexGrid::initializeWorldBounds(glm::vec2 const& center)
{
	m_discoveredChunksAABB.min = center;
	m_discoveredChunksAABB.max = center;

	updateWorldBounds();
}

void e2::HexGrid::updateWorldBounds()
{
	glm::vec2 cs = chunkSize();

	m_worldBounds = m_discoveredChunksAABB;
	m_worldBounds.min -= cs;
	m_worldBounds.max += cs;

	updateViewBounds();
}


void e2::HexGrid::destroyFogOfWar()
{
	if (m_frameData[0].minimapTexture)
		e2::discard(m_frameData[0].minimapTexture);
	if (m_frameData[1].minimapTexture)
		e2::discard(m_frameData[1].minimapTexture);
	if (m_frameData[0].minimapTarget)
		e2::discard(m_frameData[0].minimapTarget);
	if (m_frameData[1].minimapTarget)
		e2::discard(m_frameData[1].minimapTarget);
	if (m_minimapPipelineLayout)
		e2::discard(m_minimapPipelineLayout);
	if (m_minimapPipeline)
		e2::discard(m_minimapPipeline);
	if (m_minimapFragmentShader)
		e2::discard(m_minimapFragmentShader);

	if (m_frameData[0].mapUnitsTexture)
		e2::discard(m_frameData[0].mapUnitsTexture);
	if (m_frameData[1].mapUnitsTexture)
		e2::discard(m_frameData[1].mapUnitsTexture);
	if (m_frameData[0].mapUnitsTarget)
		e2::discard(m_frameData[0].mapUnitsTarget);
	if (m_frameData[1].mapUnitsTarget)
		e2::discard(m_frameData[1].mapUnitsTarget);

	if (m_minimapLayout)
		e2::discard(m_minimapLayout);
	if (m_frameData[0].minimapSet)
		e2::discard(m_frameData[0].minimapSet);
	if (m_frameData[1].minimapSet)
		e2::discard(m_frameData[1].minimapSet);
	if (m_minimapPool)
		e2::discard(m_minimapPool);

	if (m_frameData[0].mapVisTargets[0])
		e2::discard(m_frameData[0].mapVisTargets[0]);
	if (m_frameData[0].mapVisTargets[1])
		e2::discard(m_frameData[0].mapVisTargets[1]);
	if (m_frameData[1].mapVisTargets[0])
		e2::discard(m_frameData[1].mapVisTargets[0]);
	if (m_frameData[1].mapVisTargets[1])
		e2::discard(m_frameData[1].mapVisTargets[1]);

	if (m_frameData[0].mapVisTextures[0])
		e2::discard(m_frameData[0].mapVisTextures[0]);
	if (m_frameData[0].mapVisTextures[1])
		e2::discard(m_frameData[0].mapVisTextures[1]);
	if (m_frameData[1].mapVisTextures[0])
		e2::discard(m_frameData[1].mapVisTextures[0]);
	if (m_frameData[1].mapVisTextures[1])
		e2::discard(m_frameData[1].mapVisTextures[1]);


	if (m_frameData[0].fogOfWarMaskBlurSets[0])
		e2::discard(m_frameData[0].fogOfWarMaskBlurSets[0]);
	if (m_frameData[0].fogOfWarMaskBlurSets[1])
		e2::discard(m_frameData[0].fogOfWarMaskBlurSets[1]);
	if (m_frameData[1].fogOfWarMaskBlurSets[0])
		e2::discard(m_frameData[1].fogOfWarMaskBlurSets[0]);
	if (m_frameData[1].fogOfWarMaskBlurSets[1])
		e2::discard(m_frameData[1].fogOfWarMaskBlurSets[1]);

	if (m_frameData[0].mapVisBlurSets[0])
		e2::discard(m_frameData[0].mapVisBlurSets[0]);
	if (m_frameData[0].mapVisBlurSets[1])
		e2::discard(m_frameData[0].mapVisBlurSets[1]);
	if (m_frameData[1].mapVisBlurSets[0])
		e2::discard(m_frameData[1].mapVisBlurSets[0]);
	if (m_frameData[1].mapVisBlurSets[1])
		e2::discard(m_frameData[1].mapVisBlurSets[1]);

	if (m_blurPool)
		e2::discard(m_blurPool);

	if (m_blurFragmentShader)
		e2::discard(m_blurFragmentShader);

	if (m_blurPipeline)
		e2::discard(m_blurPipeline);

	if (m_blurSetLayout)
		e2::discard(m_blurSetLayout);

	if (m_blurPipelineLayout)
		e2::discard(m_blurPipelineLayout);

	if (m_outlinePipelineLayout)
		e2::discard(m_outlinePipelineLayout);

	if (m_outlineVertexShader)
		e2::discard(m_outlineVertexShader);

	if (m_outlineFragmentShader)
		e2::discard(m_outlineFragmentShader);

	if (m_outlinePipeline)
		e2::discard(m_outlinePipeline);

	if (m_fogOfWarPipelineLayout)
		e2::discard(m_fogOfWarPipelineLayout);

	if (m_frameData[0].fogOfWarMasks[0])
		e2::discard(m_frameData[0].fogOfWarMasks[0]);
	if (m_frameData[0].fogOfWarMasks[1])
		e2::discard(m_frameData[0].fogOfWarMasks[1]);

	if (m_frameData[1].fogOfWarMasks[0])
		e2::discard(m_frameData[1].fogOfWarMasks[0]);
	if (m_frameData[1].fogOfWarMasks[1])
		e2::discard(m_frameData[1].fogOfWarMasks[1]);


	if (m_frameData[0].fogOfWarTargets[0])
		e2::discard(m_frameData[0].fogOfWarTargets[0]);
	if (m_frameData[0].fogOfWarTargets[1])
		e2::discard(m_frameData[0].fogOfWarTargets[1]);

	if (m_frameData[1].fogOfWarTargets[0])
		e2::discard(m_frameData[1].fogOfWarTargets[0]);
	if (m_frameData[1].fogOfWarTargets[1])
		e2::discard(m_frameData[1].fogOfWarTargets[1]);

	if (m_fogOfWarVertexShader)
		e2::discard(m_fogOfWarVertexShader);

	if (m_fogOfWarFragmentShader)
		e2::discard(m_fogOfWarFragmentShader);

	if (m_fogOfWarPipeline)
		e2::discard(m_fogOfWarPipeline);

	if (m_fogOfWarCommandBuffers[0])
		e2::discard(m_fogOfWarCommandBuffers[0]);

	if (m_fogOfWarCommandBuffers[1])
		e2::discard(m_fogOfWarCommandBuffers[1]);

	if (m_frameData[0].outlineTarget)
		e2::discard(m_frameData[0].outlineTarget);
	if (m_frameData[1].outlineTarget)
		e2::discard(m_frameData[1].outlineTarget);

	if (m_frameData[0].outlineTexture)
		e2::discard(m_frameData[0].outlineTexture);
	if (m_frameData[1].outlineTexture)
		e2::discard(m_frameData[1].outlineTexture);


}

void e2::HexGrid::clearVisibility()
{
	for (int32_t &vis : m_tileVisibility)
		vis = 0;
}

void e2::HexGrid::flagVisible(glm::ivec2 const& v, bool onlyDiscover)
{
	int32_t tileIndex = (int32_t)getTileIndexFromHex(v);

	if(!onlyDiscover)
		m_tileVisibility[tileIndex] = m_tileVisibility[tileIndex]  + 1;
}

void e2::HexGrid::unflagVisible(glm::ivec2 const& v)
{
	auto finder = m_tileIndex.find(v);
	int32_t index = 0;
	if (finder == m_tileIndex.end())
	{
		LogError("attempted to unflag visibility in nondiscovered area, this is likely a bug!");
		return;
	}
	index = (int32_t)finder->second;

	m_tileVisibility[index] = m_tileVisibility[index] - 1;
}

bool e2::HexGrid::isVisible(glm::ivec2 const& v)
{
	auto finder = m_tileIndex.find(v);
	if (finder == m_tileIndex.end())
	{
		return false;
	}
	int32_t index = (int32_t)finder->second;
	return m_tileVisibility[index];
}

void e2::HexGrid::clearOutline()
{
	for (uint8_t i = 0; i < size_t(e2::OutlineLayer::Count); i++)
	{
		m_outlineTiles[i].clear();
	}
	
}

void e2::HexGrid::pushOutline(e2::OutlineLayer layer, glm::ivec2 const& tile)
{
	m_outlineTiles[size_t(layer)].push_back(tile);
}

e2::ITexture* e2::HexGrid::outlineTexture(uint8_t frameIndex)
{
	return m_frameData[frameIndex].outlineTexture;
}

e2::ITexture* e2::HexGrid::minimapTexture(uint8_t frameIndex)
{
	return m_frameData[frameIndex].minimapTexture;
}

void e2::HexGrid::setFog(float newFog)
{
	m_fogProxy->uniformData.set(FogData{ {newFog, 0.0f, 0.0f, 0.0f} });
}

void e2::HexGrid::removeWood(glm::ivec2 const& location)
{
	e2::TileData* tileData = getExistingTileData(location);
	if (!tileData)
		return;

	bool hasForest = (tileData->flags & e2::TileFlags::FeatureForest) != TileFlags::FeatureNone;
	if (!hasForest)
		return;

	tileData->flags = e2::TileFlags(uint16_t(tileData->flags) & (~uint16_t(e2::TileFlags::FeatureForest)));

	if (tileData->forestProxy)
		e2::destroy(tileData->forestProxy);

	tileData->forestProxy = nullptr;

	glm::ivec2 chunkIndex = chunkIndexFromPlanarCoords(Hex(location).planarCoords());
	e2::ChunkState *chunkState = getOrCreateChunk(chunkIndex);
	refreshChunkMeshes(chunkState);
}



void e2::HexGrid::debugDraw()
{
	if (!m_debugDraw)
		return;
	glm::vec3 colorPoked{ 1.0f, 0.0f, 0.0f };
	glm::vec3 colorQueued{ 1.0f, 1.0f, 0.0f };
	glm::vec3 colorStreaming{ 0.0f, 1.0f, 0.0f };
	glm::vec3 colorReady{ 0.0f, 1.0f, 1.0f };

	glm::vec2 _chunkSize = chunkSize();
	glm::vec3 _chunkSize3{ _chunkSize.x, 0.0f, _chunkSize.y };

	glm::vec2 streamStateSize = _chunkSize * 0.125f;
	glm::vec2 visibilitySize = _chunkSize * 0.4895f;

	e2::Renderer* renderer = gameSession()->renderer();

	for (auto& [chunkIndex, chunk] : m_chunkIndex)
	{
		e2::StreamState streamState = chunk->streamState;
		bool inView = chunk->inView;

		glm::vec3 chunkCenter = chunkOffsetFromIndex(chunkIndex) + _chunkSize3 / 2.0f;
		glm::vec2 chunkCenter2{chunkCenter.x, chunkCenter.z};

		glm::vec2 tl = chunkCenter2 - streamStateSize;
		glm::vec2 br = chunkCenter2 + streamStateSize;
		glm::vec2 tr = { br.x, tl.y };
		glm::vec2 bl = { tl.x, br.y };


		glm::vec2 tl2 = chunkCenter2 - visibilitySize;
		glm::vec2 br2 = chunkCenter2 + visibilitySize;
		glm::vec2 tr2 = { br2.x, tl2.y };
		glm::vec2 bl2 = { tl2.x, br2.y };

		glm::vec3 color = streamState == StreamState::Poked ? colorPoked :
			streamState == StreamState::Queued ? colorQueued :
			streamState == StreamState::Streaming ? colorStreaming :
			streamState == StreamState::Ready ? colorReady : colorReady;

		renderer->debugLine(color, tl, tr);
		renderer->debugLine(color, tr, br);
		renderer->debugLine(color, br, bl);
		renderer->debugLine(color, bl, tl);

		glm::vec3 color2{0.05f, 0.05f, 0.05f};
		if (inView)
			color2 = {0.65f, 0.65f, 0.65f};
		renderer->debugLine(color2, tl2, tr2);
		renderer->debugLine(color2, tr2, br2);
		renderer->debugLine(color2, br2, bl2);
		renderer->debugLine(color2, bl2, tl2);
	}

	renderer->debugLine({1.0f, 1.0f, 1.0f}, m_streamingView.corners[0], m_streamingView.corners[1]);
	renderer->debugLine({ 1.0f, 1.0f, 1.0f }, m_streamingView.corners[2], m_streamingView.corners[3]);
	renderer->debugLine({ 1.0f, 1.0f, 1.0f }, m_streamingView.corners[0], m_streamingView.corners[2]);
	renderer->debugLine({ 1.0f, 1.0f, 1.0f }, m_streamingView.corners[1], m_streamingView.corners[3]);

	e2::StackVector<glm::vec2, 4> p = m_streamingViewAabb.points();
	renderer->debugLine({ 0.0f, 0.0f, 1.0f }, p[0], p[1]);
	renderer->debugLine({ 0.0f, 0.0f, 1.0f }, p[1], p[2]);
	renderer->debugLine({ 0.0f, 0.0f, 1.0f }, p[2], p[3]);
	renderer->debugLine({ 0.0f, 0.0f, 1.0f }, p[3], p[0]);

}

void e2::HexGrid::updateViewBounds()
{
	m_minimapViewBounds = m_worldBounds;

	glm::vec2 worldSize = m_minimapViewBounds.max - m_minimapViewBounds.min;
	glm::vec2 worldOffset = m_minimapViewBounds.min;
	glm::vec2 worldCenter = worldOffset + worldSize / 2.0f;

	// rescale and then recenter
	if (worldSize.x < worldSize.y)
		worldSize.x = (worldSize.x / (worldSize.x / worldSize.y)) * (m_minimapSize.x / m_minimapSize.y);
	else
		worldSize.y = (worldSize.y / (worldSize.y / worldSize.x)) * (m_minimapSize.x / m_minimapSize.y);

	//worldCenter = worldCenter + m_minimapViewOffset;
	//worldSize = worldSize * m_minimapViewZoom;

	m_minimapViewBounds.min = worldCenter - worldSize / 2.0f;
	m_minimapViewBounds.max = worldCenter + worldSize / 2.0f;
}

void e2::HexGrid::debugDraw(bool newValue)
{
	m_debugDraw = newValue;
	renderManager()->getShaderModel("e2::FogModel")->active(!newValue);
}

namespace
{
	//uint64_t numChunkLoadTasks = 0;
}

e2::ChunkLoadTask::ChunkLoadTask(e2::HexGrid* grid, glm::ivec2 const& chunkIndex)
	: e2::AsyncTask(grid->game())
	, m_grid(grid)
	, m_chunkIndex(chunkIndex)
{
	//::numChunkLoadTasks++;

	//LogNotice("Num chunk load tasks increased to {}", ::numChunkLoadTasks);
}

e2::ChunkLoadTask::~ChunkLoadTask()
{
	//::numChunkLoadTasks--;
	//LogNotice("Num chunk load tasks decreased to {}", ::numChunkLoadTasks);
}

bool e2::ChunkLoadTask::prepare()
{
	m_fastHex = &m_grid->fastHex();
	m_fastHexHigh = &m_grid->fastHexHigh();

	return true;
}

bool e2::ChunkLoadTask::execute()
{
	//E2_TIME_SCOPE("WorldGen");

	e2::Moment begin = e2::timeNow();

	constexpr uint32_t HexGridChunkResolutionSquared = e2::hexChunkResolution * e2::hexChunkResolution;
	constexpr uint32_t HexGridChunkResolutionHalf = e2::hexChunkResolution / 2;

	glm::vec3 chunkOffset = e2::HexGrid::chunkOffsetFromIndex(m_chunkIndex);

	//e2::DynamicMesh* newChunkMesh = e2::create<e2::DynamicMesh>();
	
	
	HexShaderData shaderData;
	shaderData.chunkOffset = { chunkOffset.x, chunkOffset.z };
	shaderData.grid = m_grid;
	shaderData.cacheOffset = m_chunkIndex * glm::ivec2(e2::hexChunkResolution);
	
	struct _GenTileData
	{
		e2::Hex worldHex;
		//e2::Hex localHex;
		glm::vec3 localOffset;
		//glm::ivec2 localOffsetCoords;
		//glm::ivec2 worldOffsetCoords;
		e2::FastMesh2* mesh;
	};

	thread_local e2::StackVector<_GenTileData, HexGridChunkResolutionSquared> _fastMeshes;
	_fastMeshes.resize(HexGridChunkResolutionSquared);

	uint32_t vertexCapacity = 0;
	uint32_t indexCapacity = 0;

	{
		//E2_TIME_SCOPE("WorldGen.Prepare");
		int32_t i = 0;
		for (int32_t y = -1; y < (int32_t)e2::hexChunkResolution+1; y++)
		{
			for (int32_t x = -1; x < (int32_t)e2::hexChunkResolution+1; x++)
			{
				glm::ivec2 localOffsetCoords(x, y);
				e2::Hex localHex(localOffsetCoords);
				glm::ivec2 worldOffsetCoords = shaderData.cacheOffset + localOffsetCoords;

				e2::TileData newTile = shaderData.grid->calculateTileData(worldOffsetCoords);
				shaderData.tileCache[i++] = newTile;

				if (y < 0 || x < 0 || x >= (int32_t)e2::hexChunkResolution || y >= (int32_t)e2::hexChunkResolution)
					continue;

				_GenTileData newData;
				newData.localOffset = localHex.localCoords();
				newData.worldHex = e2::Hex(worldOffsetCoords);

				if ((newTile.flags & e2::TileFlags::WaterMask) != e2::TileFlags::WaterNone)
					m_hasWaterTile = true;

				bool isMountain = ((newTile.flags & e2::TileFlags::FeatureMountains) != e2::TileFlags::FeatureNone);
				newData.mesh = isMountain ? m_fastHexHigh : m_fastHex;
				//newData.mesh = m_fastHex;

				vertexCapacity += newData.mesh->numVertices;
				indexCapacity += newData.mesh->numIndices;

				_fastMeshes[y * e2::hexChunkResolution + x] = newData;
			}
		}
	}

	e2::FastMesh2 newFastMesh(vertexCapacity, indexCapacity, true);

	{
		//E2_TIME_SCOPE("WorldGen.AddMesh");
		for (_GenTileData& genTile : _fastMeshes)
		{
			shaderData.hex = genTile.worldHex;
			newFastMesh.addMesh(*genTile.mesh, { genTile.localOffset, 1.0f }, ::hexShaderFast2, &shaderData);
		}
	}

	{
		//E2_TIME_SCOPE("WorldGen.CalcVertexTangents");
		newFastMesh.calculateVertexTangents();
	}

	{
		//E2_TIME_SCOPE("WorldGen.Bake");
		m_generatedMesh = newFastMesh.bake(m_grid->hexMaterial());
	}

	//for (int32_t y = 0; y < e2::hexChunkResolution; y++)
	//{
	//	for (int32_t x = 0; x < e2::hexChunkResolution; x++)
	//	{
	//		glm::vec3 tileOffset = e2::Hex(glm::ivec2(x, y)).localCoords();
	//		glm::mat4 transform = glm::translate(glm::identity<glm::mat4>(), tileOffset);
	//		shaderData.hex = e2::Hex(m_chunkIndex * glm::ivec2(e2::hexChunkResolution) + glm::ivec2(x, y));
	//		shaderData.tileData = shaderData.getTileData(shaderData.hex.offsetCoords());
	//		if ((shaderData.tileData.flags & e2::TileFlags::FeatureMountains) != e2::TileFlags::FeatureNone)
	//		{
	//			newChunkMesh->addMeshWithShaderFunction(m_dynaHexHigh, transform, ::hexShader, &shaderData);
	//		}
	//		else
	//		{
	//			newChunkMesh->addMeshWithShaderFunction(m_dynaHex, transform, ::hexShader, &shaderData);
	//		}
	//		if ((shaderData.tileData.flags & e2::TileFlags::WaterMask) != e2::TileFlags::WaterNone)
	//		{
	//			m_hasWaterTile = true;
	//		}
	//	}
	//}
	//newChunkMesh->calculateVertexTangents();
	//m_generatedMesh = newChunkMesh->bake(m_grid->hexMaterial(), e2::VertexAttributeFlags(uint8_t(e2::VertexAttributeFlags::Normal) | uint8_t(e2::VertexAttributeFlags::Color)));
	//e2::destroy(newChunkMesh);


	m_ms = (float)begin.durationSince().milliseconds();

	return true;
}

bool e2::ChunkLoadTask::finalize()
{
	m_grid->endStreamingChunk(m_chunkIndex, m_generatedMesh, m_ms, m_hasWaterTile);
	return true;
}

void e2::HexGrid::popInChunk(e2::ChunkState* state)
{
	state->visibilityState = true;
	m_visibleChunks.insert(state);
	m_hiddenChunks.erase(state);

	glm::vec3 chunkOffset = chunkOffsetFromIndex(state->chunkIndex);

	// finalize
	if (state->mesh && !state->proxy)
	{
		e2::MeshProxyConfiguration proxyConf;
		e2::MeshLodConfiguration lod;
		lod.mesh = state->mesh;
		proxyConf.lods.push(lod);

		state->proxy = e2::create<e2::MeshProxy>(gameSession(), proxyConf);
		state->proxy->modelMatrix = glm::translate(glm::mat4(1.0f), chunkOffset);
		state->proxy->modelMatrixDirty = { true };

		refreshChunkMeshes(state);
	}

	if (!state->waterProxy /** && state->hasWaterTile */)
	{
		e2::MeshProxyConfiguration waterConf;
		e2::MeshLodConfiguration lod;
		lod.mesh = m_waterChunk;
		waterConf.lods.push(lod);

		state->waterProxy = e2::create<e2::MeshProxy>(gameSession(), waterConf);
		state->waterProxy->modelMatrix = glm::translate(glm::mat4(1.0f), chunkOffset + glm::vec3(0.0f, e2::waterLine, 0.0f));
		state->waterProxy->modelMatrixDirty = { true };
	}
	if (!state->fogProxy)
	{
		e2::MeshProxyConfiguration fogConf;
		e2::MeshLodConfiguration lod;
		lod.mesh = m_fogChunk;
		fogConf.lods.push(lod);

		state->fogProxy = e2::create<e2::MeshProxy>(gameSession(), fogConf);
		state->fogProxy->modelMatrix = glm::translate(glm::mat4(1.0f), chunkOffset + glm::vec3(0.0f, -1.0f, 0.0f));
		state->fogProxy->modelMatrixDirty = { true };
	}


}

void e2::HexGrid::popOutChunk(e2::ChunkState* state)
{
	state->visibilityState = false;
	m_visibleChunks.erase(state);
	m_hiddenChunks.insert(state);


	if (state->waterProxy)
	{
		e2::destroy(state->waterProxy);
		state->waterProxy = nullptr;
	}
	if (state->fogProxy)
	{
		e2::destroy(state->fogProxy);
		state->fogProxy = nullptr;
	}
	if (state->proxy)
	{
		e2::destroy(state->proxy);
		state->proxy = nullptr;
	}
	for (e2::MeshProxy* mesh : state->extraMeshes)
	{
		e2::destroy(mesh);
	}
	state->extraMeshes.resize(0);

	for (int32_t y = 0; y < e2::hexChunkResolution; y++)
	{
		for (int32_t x = 0; x < e2::hexChunkResolution; x++)
		{
			glm::vec3 tileOffset = e2::Hex(glm::ivec2(x, y)).localCoords();
			glm::mat4 transform = glm::translate(glm::identity<glm::mat4>(), tileOffset);

			e2::Hex tileHex = e2::Hex(state->chunkIndex * glm::ivec2(e2::hexChunkResolution) + glm::ivec2(x, y));

			e2::TileData* realTileData = getExistingTileData(tileHex.offsetCoords());
			if (realTileData)
			{
				if (realTileData->forestProxy)
					e2::destroy(realTileData->forestProxy);

				realTileData->forestProxy = nullptr;

				if (realTileData->resourceProxy)
					e2::destroy(realTileData->resourceProxy);

				realTileData->resourceProxy = nullptr;
			}
		}
	}
}

void e2::HexGrid::refreshChunkMeshes(e2::ChunkState* state)
{
	for (e2::MeshProxy* mesh : state->extraMeshes)
	{
		e2::destroy(mesh);
	}
	state->extraMeshes.resize(0);


	for (int32_t y = 0; y < e2::hexChunkResolution; y++)
	{
		for (int32_t x = 0; x < e2::hexChunkResolution; x++)
		{
			glm::vec3 tileOffset = e2::Hex(glm::ivec2(x, y)).localCoords();
			glm::mat4 transform = glm::translate(glm::identity<glm::mat4>(), tileOffset);

			glm::ivec2 tileHex = state->chunkIndex * glm::ivec2(e2::hexChunkResolution) + glm::ivec2(x, y);

			// get real data if we have it, otherwise calculate 
			e2::TileData* realTileData = getExistingTileData(tileHex);
			if (realTileData)
			{
				if (realTileData->forestProxy)
					e2::destroy(realTileData->forestProxy);

				realTileData->forestProxy = createForestProxyForTile(realTileData, tileHex);

				if (realTileData->resourceProxy)
					e2::destroy(realTileData->resourceProxy);

				realTileData->resourceProxy = createResourceProxyForTile(realTileData, tileHex);

				if(realTileData->getBiome() == e2::TileFlags::BiomeGrassland && realTileData->isLand() && !realTileData->isMountain())
					state->extraMeshes.push(createGrassProxy(realTileData, tileHex));
			}
			else
			{
				e2::TileData tileData = calculateTileData(tileHex);

				e2::MeshProxy* newForestProxy = createForestProxyForTile(&tileData, tileHex);
				if (newForestProxy)
					state->extraMeshes.push(newForestProxy);

				e2::MeshProxy* newResourceProxy = createResourceProxyForTile(&tileData, tileHex);
				if (newResourceProxy)
					state->extraMeshes.push(newResourceProxy);

				if (tileData.getBiome() == e2::TileFlags::BiomeGrassland && tileData.isLand() && !tileData.isMountain())
					state->extraMeshes.push(createGrassProxy(&tileData, tileHex));
			}
		}
	}

}

void e2::HexGrid::queueStreamingChunk(e2::ChunkState* state)
{
	if (state->streamState != StreamState::Poked)
		return;

	state->streamState = StreamState::Queued;
	m_queuedChunks.insert(state);
}

void e2::HexGrid::startStreamingChunk(e2::ChunkState* state)
{
	if (state->streamState != StreamState::Queued)
		return;

	m_queuedChunks.erase(state);
	m_streamingChunks.insert(state);

	state->streamState = StreamState::Streaming;
	state->task = e2::ChunkLoadTaskPtr::create(this, state->chunkIndex).cast<e2::AsyncTask>();
	asyncManager()->enqueue({ state->task });

}

void e2::HexGrid::endStreamingChunk(glm::ivec2 const& chunkIndex, e2::MeshPtr newMesh, double timeMs, bool hasWaterTile)
{
	// If the chunkstate is no longer valid, throw away the work silently
	auto finder = m_chunkIndex.find(chunkIndex);
	if (finder == m_chunkIndex.end())
	{
		return;
	}
	
	LogNotice("Streamed chunk in {} ms", timeMs);

	// Update highest load time 
	if (timeMs > m_highLoadTime)
		m_highLoadTime = timeMs;

	// Set new stuff on the chunk
	e2::ChunkState* chunk = m_chunkIndex[chunkIndex];
	chunk->mesh = newMesh;
	chunk->task = nullptr;
	chunk->hasWaterTile = hasWaterTile;
	chunk->streamState = StreamState::Ready;

	m_streamingChunks.erase(chunk);
	m_invalidatedChunks.insert(chunk);
}

e2::ChunkState::~ChunkState()
{

}

bool e2::TileData::isPassable(PassableFlags passableFlags)
{
	
	bool canPassLand = (passableFlags & PassableFlags::Land) == PassableFlags::Land;
	bool canPassMountain = (passableFlags & PassableFlags::Mountain) == PassableFlags::Mountain;
	bool canPassWaterShallow = (passableFlags & PassableFlags::WaterShallow) == PassableFlags::WaterShallow;
	bool canPassWaterDeep = (passableFlags & PassableFlags::WaterDeep) == PassableFlags::WaterDeep;

	bool waterLevelLand = (flags & e2::TileFlags::WaterMask) == e2::TileFlags::WaterNone;
	bool waterLevelShallow = (flags & e2::TileFlags::WaterMask) == e2::TileFlags::WaterShallow;
	bool waterLevelDeep = (flags & e2::TileFlags::WaterMask) == e2::TileFlags::WaterDeep;
	bool isMountain = (flags & e2::TileFlags::FeatureMountains) != e2::TileFlags::FeatureNone;

	if (waterLevelLand)
	{
		if (isMountain)
			return canPassMountain;
		else 
			return canPassLand;
	}
	else if (waterLevelShallow)
	{
		return canPassWaterShallow;
	}
	else if (waterLevelDeep)
	{
		return canPassWaterDeep;
	}

	return false;
}

bool e2::TileData::isShallowWater()
{
	return getWater() == e2::TileFlags::WaterShallow;
}

bool e2::TileData::isDeepWater()
{
	return getWater() == e2::TileFlags::WaterDeep;
}

bool e2::TileData::isLand()
{
	return getWater() == e2::TileFlags::WaterNone;
}

bool e2::TileData::isMountain()
{
	return uint16_t(flags & e2::TileFlags::FeatureMountains);
}

bool e2::TileData::hasGold()
{
	return getResource() == TileFlags::ResourceGold;
}

bool e2::TileData::hasOil()
{
	return getResource() == TileFlags::ResourceOil;
}

bool e2::TileData::hasOre()
{
	return getResource() == TileFlags::ResourceOre;
}

bool e2::TileData::hasStone()
{
	return getResource() == TileFlags::ResourceStone;
}

bool e2::TileData::hasUranium()
{
	return getResource() == TileFlags::ResourceUranium;
}

e2::TileFlags e2::TileData::getWater()
{
	return (flags & e2::TileFlags::WaterMask);
}

e2::TileFlags e2::TileData::getFeature()
{
	return (flags & e2::TileFlags::FeatureMask);
}

e2::TileFlags e2::TileData::getBiome()
{
	return (flags & e2::TileFlags::BiomeMask);
}

e2::TileFlags e2::TileData::getResource()
{
	return (flags & e2::TileFlags::ResourceMask);
}

e2::TileFlags e2::TileData::getAbundance()
{
	return (flags & e2::TileFlags::AbundanceMask);
}

e2::TileFlags e2::TileData::getWoodAbundance()
{
	return (flags & e2::TileFlags::WoodAbundanceMask);
}

bool e2::TileData::isForested()
{
	return (flags & TileFlags::FeatureForest) != TileFlags::FeatureNone;
}

bool e2::TileData::hasResource()
{
	return (flags & TileFlags::ResourceMask) != TileFlags::ResourceNone;
}

float e2::TileData::getAbundanceAsFloat()
{
	switch (getAbundance()) 
	{
	case TileFlags::Abundance1:
		return 1.0f;
	case TileFlags::Abundance2:
		return 2.0f;
	case TileFlags::Abundance3:
		return 3.0f;
	case TileFlags::Abundance4:
		return 4.0f;
	}

	return 0.0f;
}

float e2::TileData::getWoodAbundanceAsFloat()
{
	switch (getWoodAbundance())
	{
	case TileFlags::WoodAbundance1:
		return 1.0f;
	case TileFlags::WoodAbundance2:
		return 2.0f;
	case TileFlags::WoodAbundance3:
		return 3.0f;
	case TileFlags::WoodAbundance4:
		return 4.0f;
	}

	return 0.0f;
}

