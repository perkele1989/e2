
#include "e2/hex/hex.hpp"
#include "e2/game/gamesession.hpp"
#include "e2/managers/asyncmanager.hpp"

#include <glm/gtc/noise.hpp>

e2::HexGrid::HexGrid(e2::Context* ctx)
	: m_engine(ctx->engine())
{
	constexpr size_t prewarmSize = 16'384;
	m_tiles.reserve(prewarmSize);
	m_tileIndex.reserve(prewarmSize);

	e2::ALJDescription aljDesc;
	assetManager()->prescribeALJ(aljDesc, "assets/SM_HexBase.e2a");
	assetManager()->prescribeALJ(aljDesc, "assets/SM_HexBaseHigh.e2a");
	assetManager()->prescribeALJ(aljDesc, "assets/SM_CoordinateSpace.e2a");
	assetManager()->prescribeALJ(aljDesc, "assets/SM_PalmTree001.e2a");
	if (!assetManager()->queueWaitALJ(aljDesc))
	{
		LogError("Failed to load hex base mesh");
		return;
	}

	m_baseHex = assetManager()->get("assets/SM_HexBase.e2a")->cast<e2::Mesh>();
	m_dynaHex = e2::DynamicMesh(m_baseHex, 0, VertexAttributeFlags::Color);

	m_baseHexHigh = assetManager()->get("assets/SM_HexBaseHigh.e2a")->cast<e2::Mesh>();
	m_dynaHexHigh = e2::DynamicMesh(m_baseHexHigh, 0, VertexAttributeFlags::Color);


	glm::vec2 waterOrigin = e2::Hex(glm::ivec2(0)).planarCoords();
	glm::vec2 waterEnd = e2::Hex(glm::ivec2(e2::HexGridChunkResolution)).planarCoords();
	glm::vec2 waterSize = waterEnd - waterOrigin;
	uint32_t waterResolution = 64;
	glm::vec2 waterTileSize = waterSize / float(waterResolution);


	e2::DynamicMesh dynaWater;
	constexpr uint32_t segments = 4;
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
	m_waterMaterial = e2::create<e2::Material>();
	m_waterMaterial->postConstruct(this, {});
	m_waterMaterial->overrideModel(renderManager()->getShaderModel("e2::WaterModel"));
	m_waterChunk = dynaWater.bake(m_waterMaterial, VertexAttributeFlags::None);

	m_terrainMaterial = e2::create<e2::Material>();
	m_terrainMaterial->postConstruct(this, {});
	m_terrainMaterial->overrideModel(renderManager()->getShaderModel("e2::TerrainModel"));
}

e2::HexGrid::~HexGrid()
{
	clearAllChunks();
}

e2::Engine* e2::HexGrid::engine()
{
	return m_engine;
}


glm::vec2 e2::HexGrid::chunkSize()
{
	return e2::Hex(glm::ivec2(e2::HexGridChunkResolution)).planarCoords();
}

glm::ivec2 e2::HexGrid::chunkIndexFromPlanarCoords(glm::vec2 const& planarCoords)
{
	return glm::ivec2(planarCoords / chunkSize());
}

glm::vec3 e2::HexGrid::chunkOffsetFromIndex(glm::ivec2 const& index)
{
	
	return e2::Hex(index * glm::ivec2(e2::HexGridChunkResolution)).localCoords();
}

namespace
{
	struct HexShaderData
	{
		e2::Hex hex;
		glm::vec2 chunkOffset;
	};


	float ridge(float v, float offset)
	{
		v = offset - glm::abs(v);
		return v < 0.5f ? 4.0f * v * v * v : (v - 1.0f) * (2.0f * v - 2.0f) * (2.0f * v - 2.0f) + 1.0f;
	}

	float ridgedMF(glm::vec2 const& position)
	{
		struct {
			uint32_t octaves{ 2 };
			float frequency{ 0.3f };
			float lacunarity{ 2.0f };
			float gain{ 0.5f };
			float ridgeOffset{ 0.95f };
		} cfg;

		float sum = 0.0f;
		float amp = 0.5f;
		float prev = 1.0f;
		float freq = cfg.frequency;
		for (uint32_t i = 0; i < cfg.octaves; i++)
		{
			float h = glm::simplex(position * freq);
			float n = ridge(h, cfg.ridgeOffset);
			sum += n * amp * prev;
			prev = n;
			freq *= cfg.lacunarity;
			amp *= cfg.gain;
		}

		return sum;
	}

	float sampleHeight(glm::vec2 const& worldPosition)
	{
		float outHeight = 0.0f;

		e2::Hex hex(worldPosition);
		glm::vec2 hexCenter = hex.planarCoords();
		

		e2::TileData tile = e2::HexGrid::calculateTileDataForHex(hex);
		bool currIsMountain = (tile.flags & e2::TileFlags::BiomeMask) == e2::TileFlags::BiomeMountain;
		bool currIsShallow = (tile.flags & e2::TileFlags::BiomeMask) == e2::TileFlags::BiomeShallow;
		bool currIsOcean = (tile.flags & e2::TileFlags::BiomeMask) == e2::TileFlags::BiomeOcean;
		bool currIsWater = currIsShallow || currIsOcean;

		float distanceFromVertexToTile = glm::distance(worldPosition, hexCenter);
		float mountainDistanceCoeff = 1.0f - glm::clamp(distanceFromVertexToTile * 1.5f, 0.0f, 1.0f);
		float waterDistanceCoeff = 1.0f - glm::clamp(distanceFromVertexToTile * 1.4f, 0.0f, 1.0f);

		float coeffMountains = 0.0f;
		float coeffWater = 0.0f;

		if (currIsMountain || currIsWater)
		{
			coeffMountains = glm::pow(mountainDistanceCoeff, 0.5f);
			coeffWater = glm::pow(waterDistanceCoeff, 1.0f);

			auto neighbours = hex.neighbours();
			for (uint8_t ni = 0; ni < neighbours.size(); ni++)
			{
				uint8_t nextNi = ni + 1;
				if (nextNi >= neighbours.size())
					nextNi = 0;

				e2::Hex &neighbourHex = neighbours[ni];
				e2::TileData neighbourTile = e2::HexGrid::calculateTileDataForHex(neighbourHex);
				glm::vec2 neighbourWorldPosition = neighbourHex.planarCoords();

				e2::Hex& nextHex = neighbours[nextNi];
				e2::TileData nextTile = e2::HexGrid::calculateTileDataForHex(nextHex);
				glm::vec2 nextWorldPosition = nextHex.planarCoords();

				bool neighbourIsMountain = (neighbourTile.flags & e2::TileFlags::BiomeMask) == e2::TileFlags::BiomeMountain;
				bool neighbourIsWater = (neighbourTile.flags & e2::TileFlags::BiomeMask) == e2::TileFlags::BiomeShallow || (neighbourTile.flags & e2::TileFlags::BiomeMask) == e2::TileFlags::BiomeOcean;

				bool nextIsMountain = (nextTile.flags & e2::TileFlags::BiomeMask) == e2::TileFlags::BiomeMountain;
				bool nextIsWater = (nextTile.flags & e2::TileFlags::BiomeMask) == e2::TileFlags::BiomeShallow || (nextTile.flags & e2::TileFlags::BiomeMask) == e2::TileFlags::BiomeOcean;

				glm::vec2 midpoint = (hexCenter + neighbourWorldPosition + nextWorldPosition) / 3.0f;
				float distanceToMidpoint = glm::distance(worldPosition, midpoint);

				if (neighbourIsMountain && nextIsMountain)
				{
					float x = 1.0f - glm::clamp(distanceToMidpoint * 1.5f, 0.0f, 1.0f);
					coeffMountains += glm::pow(x, 0.5f);
				}

				if (neighbourIsWater && nextIsWater)
				{
					float x = 1.0f - glm::clamp(distanceToMidpoint * 1.0f, 0.0f, 1.0f);
					coeffWater += x;
				}
			}

			if (currIsMountain)
			{
				coeffMountains = glm::clamp(coeffMountains, 0.0f, 1.0f);
				outHeight -= ridgedMF(worldPosition) * coeffMountains;
			}

			coeffWater = glm::clamp(coeffWater, 0.0f, 1.0f);
			if (currIsWater)
			{
				outHeight += coeffWater * 1.0f;
			}
		}

		return outHeight;

	}

	glm::vec3 sampleNormal(glm::vec2 const& worldPosition)
	{
		/*
			vec3 sampleNormal(vec2 position)
	{
		float eps = 0.1;
		float eps2 = eps * 2;
		vec3 off = vec3(1.0, 1.0, 0.0)* eps;
		float hL = sampleHeight(position.xy - off.xz);
		float hR = sampleHeight(position.xy + off.xz);
		float hD = sampleHeight(position.xy - off.zy);
		float hU = sampleHeight(position.xy + off.zy);

		return normalize(vec3(hL - hR, eps2, hD - hU));
	}
		*/

		float eps = 0.1f;
		float eps2 = eps * 2.0f;
		float hL = sampleHeight(worldPosition - glm::vec2(1.0f, 0.0f) * eps);
		float hR = sampleHeight(worldPosition + glm::vec2(1.0f, 0.0f) * eps);
		float hD = sampleHeight(worldPosition - glm::vec2(0.0f, 1.0f) * eps);
		float hU = sampleHeight(worldPosition + glm::vec2(0.0f, 1.0f) * eps);

		return glm::normalize(glm::vec3(hL - hR, -eps2, hD - hU));

	}

	void hexShader(e2::Vertex* vertex, void* shaderData)
	{
		::HexShaderData* data = reinterpret_cast<::HexShaderData*>(shaderData);


		/*
		BiomeGrassland			= 0b0000'0000'0000'0000,
		BiomeForest				= 0b0010'0000'0000'0000,
		BiomeDesert				= 0b0100'0000'0000'0000,
		BiomeTundra				= 0b0110'0000'0000'0000,
		BiomeMountain			= 0b1000'0000'0000'0000,
		BiomeShallow			= 0b1010'0000'0000'0000,
		BiomeOcean				= 0b1100'0000'0000'0000,
		*/


		e2::TileData curr = e2::HexGrid::calculateTileDataForHex(data->hex);

		glm::vec2 localVertexPosition = glm::vec2(vertex->position.x, vertex->position.z);
		glm::vec2 worldVertexPosition = data->chunkOffset + localVertexPosition;

		vertex->uv01 = { worldVertexPosition, 0.0f, 0.0f };
		vertex->position.y = sampleHeight(worldVertexPosition);
		vertex->normal = sampleNormal(worldVertexPosition);


		bool currIsForest = (curr.flags & e2::TileFlags::BiomeMask) == e2::TileFlags::BiomeForest;

		/*
		glm::vec4 waterDry = e2::UIColor(0xD2B99BFF).toVec4();
		glm::vec4 waterWet = e2::UIColor(0xC6A77AFF).toVec4();
		glm::vec4 water = glm::mix(waterWet, waterDry, glm::smoothstep(1.0f, 0.0f,vertex.position.y));
		glm::vec4 grass = e2::UIColor(0x7CA917FF).toVec4();// currIsForest ? e2::UIColor(0x7CA917FF).toVec4() : waterDry;
		glm::vec4 hill = e2::UIColor(0x946644FF).toVec4();
		glm::vec4 hill2 = waterDry;
		glm::vec4 hillBlend = glm::mix(hill, hill2, glm::smoothstep(0.0f, -2.0f, vertex.position.y));
		glm::vec4 alps = e2::UIColor(0xDFE6F7FF).toVec4();
		*/

		// save hex grid thingy
		vertex->color.a = vertex->color.r;

		// fields
		vertex->color.r = glm::smoothstep(0.085f, 0.0f, vertex->position.y);

		// mountains
		vertex->color.g = glm::smoothstep(-0.0f, -0.2f, vertex->position.y);

		vertex->color.b = 0.0f;

		
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


void e2::HexGrid::assertChunksWithinRangeVisible(glm::vec2 const& streamCenter, e2::Viewpoints2D const& viewPoints, bool pauseNewJobs)
{
	e2::Aabb2D viewpointsAabb = viewPoints.toAabb();

	glm::ivec2 lowerIndex = chunkIndexFromPlanarCoords(viewpointsAabb.min);
	glm::ivec2 upperIndex = chunkIndexFromPlanarCoords(viewpointsAabb.max);
	
	gameSession()->renderer()->debugLine(glm::vec3(1.0, 1.0, 0.0), { viewpointsAabb.min.x, viewpointsAabb.min.y }, { viewpointsAabb.max.x, viewpointsAabb.min.y });
	gameSession()->renderer()->debugLine(glm::vec3(1.0, 1.0, 0.0), { viewpointsAabb.max.x, viewpointsAabb.min.y }, { viewpointsAabb.max.x, viewpointsAabb.max.y });
	gameSession()->renderer()->debugLine(glm::vec3(1.0, 1.0, 0.0), { viewpointsAabb.max.x, viewpointsAabb.max.y }, { viewpointsAabb.min.x, viewpointsAabb.max.y });
	gameSession()->renderer()->debugLine(glm::vec3(1.0, 1.0, 0.0), { viewpointsAabb.min.x, viewpointsAabb.max.y }, { viewpointsAabb.min.x, viewpointsAabb.min.y });


	// flag all chunks as invisible this frame
	for (auto [chunkIndex, chunkState] : m_chunkStates)
	{
		chunkState->visible = false;
	}

	m_visibleChunkIndices.clear();
	// generate visible chunk indices for this frame
	for (int32_t y = lowerIndex.y-1; y <= upperIndex.y; y++)
	{
		for (int32_t x = lowerIndex.x-1; x <= upperIndex.x; x++)
		{
			//constexpr float inf = std::numeric_limits<float>::infinity();
			
			constexpr uint32_t r = e2::HexGridChunkResolution;
			e2::Aabb2D chunkAabb;
			chunkAabb.min = e2::Hex(glm::ivec2(x*r, y*r)).planarCoords();
			chunkAabb.max = chunkAabb.min + chunkSize();

			glm::vec2 chunkPoints[4] = {
				chunkAabb.min,
				{chunkAabb.max.x, chunkAabb.min.y},
				chunkAabb.max,
				{chunkAabb.min.x, chunkAabb.max.y},
			};

			if (viewPoints.test(chunkAabb))
			{

				insert_sorted(m_visibleChunkIndices, { x, y }, [&streamCenter](const glm::ivec2& a, const glm::ivec2& b) {
					return glm::distance(e2::Hex(a).planarCoords(), streamCenter) < glm::distance(e2::Hex(b).planarCoords(), streamCenter);
				});

				//m_visibleChunkIndices.push_back({ x, y });
				//gameSession()->renderer()->debugLine(glm::vec3(0.0, 1.0, 1.0), chunkPoints[0], chunkPoints[1]);
				//gameSession()->renderer()->debugLine(glm::vec3(0.0, 1.0, 1.0), chunkPoints[1], chunkPoints[2]);
				//gameSession()->renderer()->debugLine(glm::vec3(0.0, 1.0, 1.0), chunkPoints[2], chunkPoints[3]);
				//gameSession()->renderer()->debugLine(glm::vec3(0.0, 1.0, 1.0), chunkPoints[3], chunkPoints[0]);
				
				//prepareChunk({ x, y });

			}
			else
			{
				//gameSession()->renderer()->debugLine(glm::vec3(1.0, 0.0, 1.0), chunkPoints[0], chunkPoints[1]);
				//gameSession()->renderer()->debugLine(glm::vec3(1.0, 0.0, 1.0), chunkPoints[1], chunkPoints[2]);
				//gameSession()->renderer()->debugLine(glm::vec3(1.0, 0.0, 1.0), chunkPoints[2], chunkPoints[3]);
				//gameSession()->renderer()->debugLine(glm::vec3(1.0, 0.0, 1.0), chunkPoints[3], chunkPoints[0]);
			}
		}
	}

	m_numVisibleChunks = m_visibleChunkIndices.size();

	// sort visible chunks by distance, to make the mstream from stream center
	std::sort(m_visibleChunkIndices.begin(), m_visibleChunkIndices.end(), [&](const glm::ivec2& a, const glm::ivec2& b) {
		return glm::distance(e2::Hex(a).planarCoords(), streamCenter)  < glm::distance(e2::Hex(b).planarCoords(), streamCenter);
	});

	// ensure visible chunks are visible
	for (glm::ivec2& offsetIndex : m_visibleChunkIndices)
	{
		// find the chunk, if it doesnt exist, then start streaming it
		auto finder = m_chunkStates.find(offsetIndex);
		if (finder == m_chunkStates.end())
		{
			if(m_numJobsInFlight < e2::maxNumJobsInFlight && !pauseNewJobs)
				prepareChunk(offsetIndex);
			continue;
		}

		// update last seen
		e2::ChunkState* state = finder->second;
		state->visible = true;
		state->lastVisible = e2::timeNow();

		// if this chunk is currently streaming in, we skip early
		if (state->task)
		{
			continue;
		}

		ensureChunkVisible(state);
	}

	m_sortedChunks.clear();
	// gather all invisible chunks and sort them by lifetime
	for (auto [chunkIndex, chunkState] : m_chunkStates)
	{
		if (chunkState->visible)
			continue;
		
		insert_sorted(m_sortedChunks, chunkState, [](e2::ChunkState* a, e2::ChunkState* b) {
			double ageA = a->lastVisible.durationSince().seconds();
			double ageB = b->lastVisible.durationSince().seconds();

			return ageA < ageB;
		});
	}

	// then ensure extra chunks are invisible, and nuke the excessive chunks
	uint32_t i = 0;
	for (e2::ChunkState* state : m_sortedChunks)
	{
		ensureChunkHidden(state);

		// excessive? NUKE!
		if (i++ >= e2::maxNumExtraChunks)
		{
			LogNotice("Nuking excessive chunk {}", state->chunkIndex);
			glm::ivec2 index = state->chunkIndex;
			e2::destroy(state);
			m_chunkStates.erase(index);
		}
	}

	
}

e2::TileData e2::HexGrid::calculateTileDataForHex(Hex hex)
{
	glm::vec2 planarCoords = hex.planarCoords();

	float simplex = pow(glm::simplex(planarCoords*0.05f), 0.5f);

	TileData newTileData;

	if(simplex > 0.75f)
		newTileData.flags |= TileFlags::BiomeMountain;
	else if (simplex > 0.5f)
		newTileData.flags |= TileFlags::BiomeGrassland;
	else if (simplex > 0.25f)
		newTileData.flags |= TileFlags::BiomeShallow;
	else 
		newTileData.flags |= TileFlags::BiomeOcean;

	newTileData.flags |= TileFlags::ResourceGold;
	newTileData.flags |= TileFlags::Abundance3;

	return newTileData;
}

size_t e2::HexGrid::discover(Hex hex)
{
	m_tiles.push_back(calculateTileDataForHex(hex));
	m_tileIndex[hex] = m_tiles.size() - 1;

	return m_tiles.size() - 1;
}

size_t e2::HexGrid::getTileIndexFromHex(Hex hex)
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
	for (auto [chunkIndex, chunkState] : m_chunkStates)
	{
		ensureChunkHidden(chunkState);

		e2::destroy(chunkState);
	}

	m_chunkStates.clear();
}

uint32_t e2::HexGrid::numChunks()
{
	return m_chunkStates.size();
}

uint32_t e2::HexGrid::numVisibleChunks()
{
	return m_numVisibleChunks;
}

uint32_t e2::HexGrid::numJobsInFlight()
{
	return m_numJobsInFlight;
}

uint32_t e2::HexGrid::numChunkMeshes()
{
	return m_numChunkMeshes;
}

double e2::HexGrid::highLoadTime()
{
	return m_highLoadTime;
}

void e2::HexGrid::clearLoadTime()
{
	m_highLoadTime = 0.0;
}

void e2::HexGrid::prepareChunk(glm::ivec2 const& chunkIndex)
{
	LogNotice("Requesting new world chunk at {}", chunkIndex);

	m_numJobsInFlight++;
	e2::ChunkState* newState = e2::create<e2::ChunkState>();

	newState->chunkIndex = chunkIndex;
	newState->task = e2::ChunkLoadTaskPtr::create(this, chunkIndex).cast<e2::AsyncTask>();
	newState->lastVisible = e2::timeNow();
	newState->visible = true;

	m_chunkStates[chunkIndex] = newState;

	asyncManager()->enqueue({ newState->task });
}

void e2::HexGrid::notifyChunkReady(glm::ivec2 const& chunkIndex, e2::MeshPtr generatedMesh, double ms)
{
	m_numJobsInFlight--;
	// no longer loading for whatever reason
	auto finder = m_chunkStates.find(chunkIndex);
	if (finder == m_chunkStates.end())
	{
		LogError("Hardworking task came home to find himself abandoned. {}", chunkIndex);
		return;
	}

	if (ms > m_highLoadTime)
		m_highLoadTime = ms;

	LogNotice("New world chunk at {}", chunkIndex);

	e2::ChunkState* chunk = m_chunkStates[chunkIndex];
	chunk->mesh = generatedMesh;
	chunk->task = nullptr;

	if (chunk->visible)
		ensureChunkVisible(chunk);
}

namespace
{
	//uint64_t numChunkLoadTasks = 0;
}

e2::ChunkLoadTask::ChunkLoadTask(e2::HexGrid* grid, glm::ivec2 const& chunkIndex)
	: e2::AsyncTask(grid)
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
	m_dynaHex = &m_grid->dynamicHex();
	m_dynaHexHigh = &m_grid->dynamicHexHigh();

	return true;
}

bool e2::ChunkLoadTask::execute()
{
	e2::Moment begin = e2::timeNow();

	constexpr uint32_t HexGridChunkResolutionSquared = e2::HexGridChunkResolution * e2::HexGridChunkResolution;
	constexpr uint32_t HexGridChunkResolutionHalf = e2::HexGridChunkResolution / 2;

	glm::vec3 chunkOffset = e2::HexGrid::chunkOffsetFromIndex(m_chunkIndex);

	/*
	e2::DynamicMesh dynaHex;
	e2::DynamicMesh dynaHexHigh;
	{
		std::scoped_lock lock(m_grid->dynamicMutex());
		dynaHex = m_grid->dynamicHex();
		dynaHexHigh = m_grid->dynamicHexHigh();
	}*/



	e2::DynamicMesh* newChunkMesh = e2::create<e2::DynamicMesh>();
	//newChunkMesh->reserve(m_dynaHexHigh->numVertices() * HexGridChunkResolutionSquared, m_dynaHexHigh->numTriangles() * HexGridChunkResolutionSquared);

	HexShaderData shaderData;
	shaderData.chunkOffset = { chunkOffset.x, chunkOffset.z };


	for (int32_t y = 0; y < e2::HexGridChunkResolution; y++)
	{
		for (int32_t x = 0; x < e2::HexGridChunkResolution; x++)
		{
			glm::vec3 tileOffset = e2::Hex(glm::ivec2(x, y)).localCoords();
			glm::mat4 transform = glm::translate(glm::identity<glm::mat4>(), tileOffset);

			shaderData.hex = e2::Hex(m_chunkIndex * glm::ivec2(e2::HexGridChunkResolution) + glm::ivec2(x, y));
			e2::TileData tileData = e2::HexGrid::calculateTileDataForHex(shaderData.hex);

			if ((tileData.flags & e2::TileFlags::BiomeMask) == e2::TileFlags::BiomeMountain)
			{
				newChunkMesh->addMeshWithShaderFunction(m_dynaHexHigh, transform, ::hexShader, &shaderData);
			}
			else
			{
				newChunkMesh->addMeshWithShaderFunction(m_dynaHex, transform, ::hexShader, &shaderData);
			}

		}
	}
	newChunkMesh->cutSlack();
	newChunkMesh->calculateVertexTangents();
	m_generatedMesh = newChunkMesh->bake(m_grid->hexMaterial(), e2::VertexAttributeFlags(uint8_t(e2::VertexAttributeFlags::Normal) | uint8_t(e2::VertexAttributeFlags::Color)));
	e2::destroy(newChunkMesh);


	m_ms = begin.durationSince().milliseconds();

	return true;
}

bool e2::ChunkLoadTask::finalize()
{
	m_grid->notifyChunkReady(m_chunkIndex, m_generatedMesh, m_ms);
	return true;
}

void e2::HexGrid::ensureChunkVisible(e2::ChunkState* state)
{
	if (state->task)
		return;

	glm::vec3 chunkOffset = chunkOffsetFromIndex(state->chunkIndex);

	// finalize
	if (state->mesh && !state->proxy)
	{
		e2::MeshProxyConfiguration proxyConf;
		proxyConf.mesh = state->mesh;

		state->proxy = e2::create<e2::MeshProxy>(gameSession(), proxyConf);
		state->proxy->modelMatrix = glm::translate(glm::mat4(1.0f), chunkOffset);
		state->proxy->modelMatrixDirty = { true };
		m_numChunkMeshes++;
	}

	if (!state->waterProxy)
	{
		e2::MeshProxyConfiguration waterConf;
		waterConf.mesh = m_waterChunk;

		state->waterProxy = e2::create<e2::MeshProxy>(gameSession(), waterConf);
		state->waterProxy->modelMatrix = glm::translate(glm::mat4(1.0f), chunkOffset + glm::vec3(0.0f, 0.1f, 0.0f));
		state->waterProxy->modelMatrixDirty = { true };
	}
}

void e2::HexGrid::ensureChunkHidden(e2::ChunkState* state)
{
	if (state->waterProxy)
	{
		e2::destroy(state->waterProxy);
		state->waterProxy = nullptr;
	}
	if (state->proxy)
	{
		e2::destroy(state->proxy);
		state->proxy = nullptr;
		m_numChunkMeshes--;
	}
}

e2::ChunkState::~ChunkState()
{

}
