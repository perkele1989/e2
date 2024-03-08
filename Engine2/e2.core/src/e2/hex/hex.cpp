
#include "e2/hex/hex.hpp"
#include "e2/game/gamesession.hpp"
#include "e2/managers/asyncmanager.hpp"

#include <glm/gtc/noise.hpp>

e2::HexGrid::HexGrid(e2::Context* ctx, e2::GameSession* session)
	: m_engine(ctx->engine())
	, m_session(session)
{
	constexpr size_t prewarmSize = 16'384;
	m_tiles.reserve(prewarmSize);
	m_tileIndex.reserve(prewarmSize);

	e2::ALJDescription aljDesc;
	assetManager()->prescribeALJ(aljDesc, "assets/SM_HexBase.e2a");
	assetManager()->prescribeALJ(aljDesc, "assets/SM_HexBaseHigh.e2a");
	assetManager()->prescribeALJ(aljDesc, "assets/SM_CoordinateSpace.e2a");
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
	m_waterProxy = m_session->getOrCreateDefaultMaterialProxy(m_waterMaterial)->unsafeCast<e2::WaterProxy>();
	m_waterChunk = dynaWater.bake(m_waterMaterial, VertexAttributeFlags::None);


	m_fogMaterial = e2::create<e2::Material>();
	m_fogMaterial->postConstruct(this, {});
	m_fogMaterial->overrideModel(renderManager()->getShaderModel("e2::FogModel"));
	m_fogProxy = m_session->getOrCreateDefaultMaterialProxy(m_fogMaterial)->unsafeCast<e2::FogProxy>();
	m_fogChunk = dynaWater.bake(m_fogMaterial, VertexAttributeFlags::None);

	m_terrainMaterial = e2::create<e2::Material>();
	m_terrainMaterial->postConstruct(this, {});
	m_terrainMaterial->overrideModel(renderManager()->getShaderModel("e2::TerrainModel"));
	m_terrainProxy = m_session->getOrCreateDefaultMaterialProxy(m_terrainMaterial)->unsafeCast<e2::TerrainProxy>();

	initializeFogOfWar();
}

e2::HexGrid::~HexGrid()
{
	clearAllChunks();
	destroyFogOfWar();
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
		
		float hU = sampleHeight(worldPosition - glm::vec2(0.0f, 1.0f) * eps);
		float hD = sampleHeight(worldPosition + glm::vec2(0.0f, 1.0f) * eps);

		return glm::normalize(glm::vec3(hR - hL, -eps2, hD - hU));

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

		//vertex->tangent = glm::rotate(glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, -1.0f)) * vertex->normal;
		


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


void e2::HexGrid::assertChunksWithinRangeVisible(glm::vec2 const& streamCenter, e2::Viewpoints2D const& viewPoints, glm::vec2 const& viewVelocity)
{
	auto renderer = m_session->renderer();
	float viewSpeed = glm::length(viewVelocity);

	m_viewpoints = viewPoints;

	e2::Aabb2D viewpointsAabb = viewPoints.toAabb();

	glm::ivec2 lowerIndex = chunkIndexFromPlanarCoords(viewpointsAabb.min);
	glm::ivec2 upperIndex = chunkIndexFromPlanarCoords(viewpointsAabb.max);
	
	constexpr bool debugRender = false;

	if (debugRender)
	{
		renderer->debugLine(glm::vec3(1.0, 1.0, 0.0), { viewpointsAabb.min.x, viewpointsAabb.min.y }, { viewpointsAabb.max.x, viewpointsAabb.min.y });
		renderer->debugLine(glm::vec3(1.0, 1.0, 0.0), { viewpointsAabb.max.x, viewpointsAabb.min.y }, { viewpointsAabb.max.x, viewpointsAabb.max.y });
		renderer->debugLine(glm::vec3(1.0, 1.0, 0.0), { viewpointsAabb.max.x, viewpointsAabb.max.y }, { viewpointsAabb.min.x, viewpointsAabb.max.y });
		renderer->debugLine(glm::vec3(1.0, 1.0, 0.0), { viewpointsAabb.min.x, viewpointsAabb.max.y }, { viewpointsAabb.min.x, viewpointsAabb.min.y });
	}

	// flag all chunks as invisible this frame
	for (auto [chunkIndex, chunkState] : m_chunkStates)
	{
		chunkState->visible = false;
	}

	glm::vec2 _chunkSize = chunkSize();

	// Setup stream-ahead offsets
	// --
	// ints are offsets to be inclusive for broadphase, they are chunks indices so units are chunk widths and heights
	// floats are margins for whether a chunk is in view or not
	float tresholdMin = 0.01f;
	float tresholdMax = 0.1f;

	int32_t leftOffset = 2;
	float narrowLeftOffset = 1.0f;
	if (viewVelocity.x < 0.0f)
	{
		leftOffset += glm::smoothstep(tresholdMin, tresholdMax, glm::abs(viewVelocity.x)) * 4.0f;
		narrowLeftOffset += glm::smoothstep(tresholdMin, tresholdMax, glm::abs(viewVelocity.x)) * 4.0f;
	}

	int32_t upOffset = 2;
	float narrowUpOffset = 1.0f;
	if (viewVelocity.y < 0.0f)
	{
		upOffset += glm::smoothstep(tresholdMin, tresholdMax, glm::abs(viewVelocity.y)) * 4.0f;
		narrowUpOffset += glm::smoothstep(tresholdMin, tresholdMax, glm::abs(viewVelocity.y)) * 4.0f;
	}

	int32_t rightOffset = 2;
	float narrowRightOffset = 1.0f;
	if (viewVelocity.x > 0.0f)
	{
		rightOffset += glm::smoothstep(tresholdMin, tresholdMax, viewVelocity.x) * 4.0f;
		narrowRightOffset += glm::smoothstep(tresholdMin, tresholdMax, viewVelocity.x) * 4.0f;
	}

	int32_t downOffset = 2;
	float narrowDownOffset = 1.0f;
	if (viewVelocity.y > 0.0f)
	{
		downOffset += glm::smoothstep(tresholdMin, tresholdMax, viewVelocity.y) * 4.0f;
		narrowDownOffset += glm::smoothstep(tresholdMin, tresholdMax, viewVelocity.y) * 4.0f;
	}
	
	m_chunkStreamQueue.clear();
	m_numVisibleChunks = 0;
	// generate visible chunk indices for this frame
	for (int32_t y = lowerIndex.y-1 - upOffset; y <= upperIndex.y + downOffset; y++)
	{
		for (int32_t x = lowerIndex.x-1 - leftOffset; x <= upperIndex.x + rightOffset; x++)
		{
			constexpr uint32_t r = e2::HexGridChunkResolution;
			glm::vec2 chunkBoundsOffset = e2::Hex(glm::ivec2(x * r, y * r)).planarCoords();

			e2::Aabb2D chunkAabb;
			chunkAabb.min = chunkBoundsOffset;
			chunkAabb.max = chunkBoundsOffset + _chunkSize;

			// points ONLY used for debug rendering!
			glm::vec2 chunkPoints[4] = {
				chunkAabb.min,
				{chunkAabb.max.x, chunkAabb.min.y},
				chunkAabb.max,
				{chunkAabb.min.x, chunkAabb.max.y},
			};

			e2::Aabb2D chunkAabbStream = chunkAabb;

			chunkAabbStream.min = chunkAabbStream.min + glm::vec2(narrowLeftOffset, narrowUpOffset) *_chunkSize;
			chunkAabbStream.max = chunkAabbStream.max - glm::vec2(narrowRightOffset, narrowDownOffset) * _chunkSize;

			glm::vec3 debugColor = glm::vec3(1.0f, 0.0f, 1.0f) * 0.1f;

			// check if relevant for streaming or visiblity
			if (viewPoints.test(chunkAabbStream))
			{
				auto finder = m_chunkStates.find({x, y});
				if (finder == m_chunkStates.end())
				{
					debugColor = glm::vec3(1.0f, 1.0f, 0.0f);
					if (viewSpeed < 2.0f)
						insert_sorted(m_chunkStreamQueue, { x, y }, [&streamCenter](const glm::ivec2& a, const glm::ivec2& b) {
						float distanceToA = glm::distance(e2::Hex(a * int32_t(e2::HexGridChunkResolution)).planarCoords(), streamCenter);
						float distanceToB = glm::distance(e2::Hex(b * int32_t(e2::HexGridChunkResolution)).planarCoords(), streamCenter);
						return  distanceToA < distanceToB;
					});
				}
				// state exists, is it relevant for visibility? (narrower check than streaming)
				else if (viewPoints.test(chunkAabb))
				{
					// update last seen
					e2::ChunkState* state = finder->second;
					state->visible = true;
					state->lastVisible = e2::timeNow();

					// if this chunk isnt currently streaming, set it as visible 
					if (!state->task)
					{
						ensureChunkVisible(state);
						m_numVisibleChunks++;
					}
					debugColor = glm::vec3(0.0f, 1.0f, 1.0f);
				}
				else
					debugColor = glm::vec3(1.0f, 0.0f, 1.0f);
			}
			
			if (debugRender)
			{
				renderer->debugLine(debugColor, chunkPoints[0], chunkPoints[1]);
				renderer->debugLine(debugColor, chunkPoints[1], chunkPoints[2]);
				renderer->debugLine(debugColor, chunkPoints[2], chunkPoints[3]);
				renderer->debugLine(debugColor, chunkPoints[3], chunkPoints[0]);
			}
		}
	}

	// send streaming jobs down the pipe
	for (glm::ivec2 const& index: m_chunkStreamQueue)
		prepareChunk(index);

	// Gather all hidden chunks, sort them by lifetime, and nuke excessive
	m_hiddenChunks.clear();
	for (auto [chunkIndex, chunkState] : m_chunkStates)
	{
		if (chunkState->visible)
			continue;
		
		insert_sorted(m_hiddenChunks, chunkState, [](e2::ChunkState* a, e2::ChunkState* b) {
			double ageA = a->lastVisible.durationSince().seconds();
			double ageB = b->lastVisible.durationSince().seconds();

			return ageA < ageB;
		});
	}
	uint32_t i = 0;
	for (e2::ChunkState* state : m_hiddenChunks)
	{
		ensureChunkHidden(state);

		// excessive? NUKE!
		if (i++ >= e2::maxNumExtraChunks)
		{
			//LogNotice("Nuking excessive chunk {}", state->chunkIndex);
			glm::ivec2 index = state->chunkIndex;
			e2::destroy(state);
			m_chunkStates.erase(index);
		}
	}


	// debug render only!
	if (debugRender)
	{
		for (auto stateIt : m_chunkStates)
		{
			auto chunkIndex = stateIt.first;
			auto chunkState = stateIt.second;

			constexpr uint32_t r = e2::HexGridChunkResolution;
			glm::vec2 chunkBoundsOffset = e2::Hex(glm::ivec2(chunkIndex.x * r, chunkIndex.y * r)).planarCoords();
			e2::Aabb2D chunkAabb;
			chunkAabb.min = chunkBoundsOffset;
			chunkAabb.max = chunkBoundsOffset + _chunkSize;



			float smallSize = 2.0f;
			glm::vec2 smallOffsetMin = (_chunkSize / 2.0f) - (smallSize / 2.0f);
			glm::vec2 smallOffsetMax = -(_chunkSize / 2.0f) + (smallSize / 2.0f);

			glm::vec2 chunkPointsSmall[4] = {
				chunkAabb.min + smallOffsetMin,
				{chunkAabb.max.x + smallOffsetMax.x, chunkAabb.min.y + smallOffsetMin.y} ,
				chunkAabb.max + smallOffsetMax,
				{chunkAabb.min.x + smallOffsetMin.x, chunkAabb.max.y + smallOffsetMax.y},
			};

			if (chunkState->mesh)
			{
				renderer->debugLine(glm::vec3(0.0, 1.0, 0.0), chunkPointsSmall[0], chunkPointsSmall[1]);
				renderer->debugLine(glm::vec3(0.0, 1.0, 0.0), chunkPointsSmall[1], chunkPointsSmall[2]);
				renderer->debugLine(glm::vec3(0.0, 1.0, 0.0), chunkPointsSmall[2], chunkPointsSmall[3]);
				renderer->debugLine(glm::vec3(0.0, 1.0, 0.0), chunkPointsSmall[3], chunkPointsSmall[0]);
			}
			else
			{
				renderer->debugLine(glm::vec3(1.0, 0.0, 0.0), chunkPointsSmall[0], chunkPointsSmall[1]);
				renderer->debugLine(glm::vec3(1.0, 0.0, 0.0), chunkPointsSmall[1], chunkPointsSmall[2]);
				renderer->debugLine(glm::vec3(1.0, 0.0, 0.0), chunkPointsSmall[2], chunkPointsSmall[3]);
				renderer->debugLine(glm::vec3(1.0, 0.0, 0.0), chunkPointsSmall[3], chunkPointsSmall[0]);
			}
		}
	}



	
}

float e2::HexGrid::sampleSimplex(glm::vec2 const& position, float scale)
{
	return glm::simplex(position * scale) * 0.5 + 0.5;
}
// @notice this function needs to stay in sync with procgen.fragment.glsl !!
float e2::HexGrid::sampleBaseHeight(glm::vec2 const& position)
{
	float h1p = 0.42;
	float scale1 = 0.058;
	float h1 = glm::pow(sampleSimplex(position, scale1), h1p);

	float semiStart = 0.31;
	float semiSize = 0.47;
	float h2p = 0.013;
	float h2 = glm::smoothstep(semiStart, semiStart + semiSize, sampleSimplex(position, h2p));

	float semiStart2 = 0.65;
	float semiSize2 = 0.1;
	float h3p = (0.75 * 20) / 5000;
	float h3 = 1.0 - glm::smoothstep(semiStart2, semiStart2 + semiSize2, sampleSimplex(position, h3p));

	return h1 * h2 * h3;
}

e2::TileData e2::HexGrid::calculateTileDataForHex(Hex hex)
{
	glm::vec2 planarCoords = hex.planarCoords();

	float h = sampleBaseHeight(planarCoords);

	TileData newTileData;

	// @notice these constants/literals need to stay in sync with procgen.fragment.glsl !!
	if(h > 0.81f)
		newTileData.flags |= TileFlags::BiomeMountain;
	else if (h > 0.39f)
		newTileData.flags |= TileFlags::BiomeGrassland;
	else if (h > 0.03f)
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
	m_tileVisibility.push_back(0);
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

e2::TileData* e2::HexGrid::getTileData(glm::ivec2 const& hex)
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
	// max num of threads to use for chunk streaming
	uint32_t numThreads = std::thread::hardware_concurrency();
	if (numThreads <= 4)
		numThreads = 2;
	else if (numThreads <= 6)
		numThreads = 4;
	else if (numThreads <= 8)
		numThreads = 6;
	else
		numThreads -= 4;

	if (m_numJobsInFlight > numThreads)
		return;

	//LogNotice("Requesting new world chunk at {}", chunkIndex);

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
		//LogError("Hardworking task came home to find himself abandoned. {}", chunkIndex);
		return;
	}

	if (ms > m_highLoadTime)
		m_highLoadTime = ms;

	//LogNotice("New world chunk at {}", chunkIndex);

	e2::ChunkState* chunk = m_chunkStates[chunkIndex];
	chunk->mesh = generatedMesh;
	chunk->task = nullptr;

	if (chunk->visible)
		ensureChunkVisible(chunk);
}

void e2::HexGrid::initializeFogOfWar()
{
	e2::PipelineLayoutCreateInfo layInf{};
	layInf.pushConstantSize = sizeof(e2::FogOfWarConstants);
	m_fogOfWarPipelineLayout = renderContext()->createPipelineLayout(layInf);

	layInf.pushConstantSize = sizeof(e2::OutlineConstants);
	m_outlinePipelineLayout = renderContext()->createPipelineLayout(layInf);

	e2::DescriptorSetLayoutCreateInfo setLayoutInf{};
	setLayoutInf.bindings = {
		{e2::DescriptorBindingType::Texture},
		{e2::DescriptorBindingType::Sampler},
	};
	m_blurSetLayout = renderContext()->createDescriptorSetLayout(setLayoutInf);

	layInf.pushConstantSize = sizeof(e2::BlurConstants);
	layInf.sets.push(m_blurSetLayout);
	m_blurPipelineLayout = renderContext()->createPipelineLayout(layInf);

	m_fogOfWarCommandBuffers[0] = renderManager()->framePool(0)->createBuffer({});
	m_fogOfWarCommandBuffers[1] = renderManager()->framePool(1)->createBuffer({});

	e2::DescriptorPoolCreateInfo poolCreateInfo{};
	poolCreateInfo.maxSets = 2 * e2::maxNumSessions;
	poolCreateInfo.numSamplers = 2 * e2::maxNumSessions;
	poolCreateInfo.numTextures = 2 * e2::maxNumSessions;
	m_blurPool = mainThreadContext()->createDescriptorPool(poolCreateInfo);

	m_blurSet[0] = m_blurPool->createDescriptorSet(m_blurSetLayout);
	m_blurSet[1] = m_blurPool->createDescriptorSet(m_blurSetLayout);

	invalidateFogOfWarShaders();
}

void e2::HexGrid::invalidateFogOfWarRenderTarget(glm::uvec2 const& newResolution)
{
	if (m_outlineTarget)
		e2::discard(m_outlineTarget);

	if (m_outlineTexture)
		e2::discard(m_outlineTexture);

	if (m_fogOfWarMask[0])
		e2::discard(m_fogOfWarMask[0]);

	if (m_fogOfWarMask[1])
		e2::discard(m_fogOfWarMask[1]);

	if (m_fogOfWarTarget[0])
		e2::discard(m_fogOfWarTarget[0]);

	if (m_fogOfWarTarget[1])
		e2::discard(m_fogOfWarTarget[1]);

	e2::TextureCreateInfo texInf{};
	texInf.initialLayout = e2::TextureLayout::ShaderRead;
	texInf.format = TextureFormat::RGBA8;
	texInf.resolution = { glm::vec2(newResolution) / 16.0f, 1 };
	texInf.mips = 1;
	m_fogOfWarMask[0] = renderContext()->createTexture(texInf);
	m_fogOfWarMask[1] = renderContext()->createTexture(texInf);

	e2::RenderTargetCreateInfo renderTargetInfo{};
	renderTargetInfo.areaExtent = glm::vec2(newResolution) / 16.0f;

	e2::RenderAttachment colorAttachment{};
	colorAttachment.target = m_fogOfWarMask[0];
	colorAttachment.clearMethod = ClearMethod::ColorFloat;
	colorAttachment.clearValue.clearColorf32 = { 0.f, 0.f, 0.f, 0.0f };
	colorAttachment.loadOperation = LoadOperation::Clear;
	colorAttachment.storeOperation = StoreOperation::Store;
	renderTargetInfo.colorAttachments.push(colorAttachment);
	m_fogOfWarTarget[0] = renderContext()->createRenderTarget(renderTargetInfo);

	renderTargetInfo.colorAttachments[0].target = m_fogOfWarMask[1];
	m_fogOfWarTarget[1] = renderContext()->createRenderTarget(renderTargetInfo);

	m_fogProxy->visibilityMask.set(m_fogOfWarMask[0]);
	//m_waterProxy->visibilityMask.set(m_fogOfWarMask[0]);
	//m_terrainProxy->visibilityMask.set(m_fogOfWarMask[0]);

	m_blurSet[0]->writeTexture(0, m_fogOfWarMask[0]);
	m_blurSet[1]->writeTexture(0, m_fogOfWarMask[1]);
	m_blurSet[0]->writeSampler(1, renderManager()->clampSampler());
	m_blurSet[1]->writeSampler(1, renderManager()->clampSampler());


	// outline target
	texInf.initialLayout = e2::TextureLayout::ShaderRead;
	texInf.format = TextureFormat::RGBA8;
	texInf.resolution = { newResolution, 1 };
	texInf.mips = 1;
	m_outlineTexture = renderContext()->createTexture(texInf);

	renderTargetInfo.areaExtent = newResolution;
	colorAttachment.target = m_outlineTexture;
	colorAttachment.clearMethod = ClearMethod::ColorFloat;
	colorAttachment.clearValue.clearColorf32 = { 0.f, 0.f, 0.f, 0.0f };
	colorAttachment.loadOperation = LoadOperation::Clear;
	colorAttachment.storeOperation = StoreOperation::Store;
	renderTargetInfo.colorAttachments = { colorAttachment };
	m_outlineTarget = renderContext()->createRenderTarget(renderTargetInfo);
}

void e2::HexGrid::invalidateFogOfWarShaders()
{


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
	m_fogOfWarVertexShader = renderContext()->createShader(shdrInf);

	std::string fragmentSource;
	if (!e2::readFileWithIncludes("shaders/fogofwar.fragment.glsl", fragmentSource))
	{
		LogError("Failed to read shader file.");
	}

	shdrInf.source = fragmentSource.c_str();
	shdrInf.stage = ShaderStage::Fragment;
	m_fogOfWarFragmentShader = renderContext()->createShader(shdrInf);

	e2::PipelineCreateInfo pipeInf{};
	pipeInf.shaders.push(m_fogOfWarVertexShader);
	pipeInf.shaders.push(m_fogOfWarFragmentShader);
	pipeInf.colorFormats = { e2::TextureFormat::RGBA8 };
	pipeInf.layout = m_fogOfWarPipelineLayout;
	m_fogOfWarPipeline = renderContext()->createPipeline(pipeInf);








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
	m_blurFragmentShader = renderContext()->createShader(shdrInf);


	pipeInf = e2::PipelineCreateInfo();
	pipeInf.shaders.push(renderManager()->fullscreenTriangleShader());
	pipeInf.shaders.push(m_blurFragmentShader);
	pipeInf.colorFormats = { e2::TextureFormat::RGBA8 };
	pipeInf.layout = m_blurPipelineLayout;
	m_blurPipeline = renderContext()->createPipeline(pipeInf);





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
	m_outlineVertexShader = renderContext()->createShader(shdrInf);

	std::string outlineFragmentSource;
	if (!e2::readFileWithIncludes("shaders/outline.fragment.glsl", outlineFragmentSource))
	{
		LogError("Failed to read shader file.");
	}

	shdrInf = e2::ShaderCreateInfo();
	shdrInf.source = outlineFragmentSource.c_str();
	shdrInf.stage = ShaderStage::Fragment;
	m_outlineFragmentShader = renderContext()->createShader(shdrInf);


	pipeInf = e2::PipelineCreateInfo();
	pipeInf.shaders.push(m_outlineVertexShader);
	pipeInf.shaders.push(m_outlineFragmentShader);
	pipeInf.colorFormats = { e2::TextureFormat::RGBA8 };
	pipeInf.layout = m_outlinePipelineLayout;
	m_outlinePipeline = renderContext()->createPipeline(pipeInf);

}

void e2::HexGrid::renderFogOfWar()
{
	glm::uvec2 newResolution = m_viewpoints.resolution;
	if (newResolution != m_fogOfWarMaskSize || !m_fogOfWarMask)
	{
		invalidateFogOfWarRenderTarget(newResolution);
		m_fogOfWarMaskSize = newResolution;
	}

	e2::SubmeshSpecification const& hexSpec = m_baseHex->specification(0);

	// outlines first 
	e2::OutlineConstants outlineConstants;
	glm::mat4 vpMatrix = m_viewpoints.view.calculateProjectionMatrix(m_viewpoints.resolution) * m_viewpoints.view.calculateViewMatrix();

	e2::ICommandBuffer* buff = m_fogOfWarCommandBuffers[renderManager()->frameIndex()];
	e2::PipelineSettings defaultSettings;
	defaultSettings.frontFace = e2::FrontFace::CCW;
	buff->beginRecord(true, defaultSettings);
	buff->useAsAttachment(m_outlineTexture);
	buff->beginRender(m_outlineTarget);
	buff->bindPipeline(m_outlinePipeline);
	buff->bindVertexLayout(hexSpec.vertexLayout);
	buff->bindIndexBuffer(hexSpec.indexBuffer);
	for (uint8_t i = 0; i < hexSpec.vertexAttributes.size(); i++)
		buff->bindVertexBuffer(i, hexSpec.vertexAttributes[i]);

	// additive outline
	for (glm::ivec2 const& hexIndex : m_outlineTiles)
	{
		glm::vec3 worldOffset = e2::Hex(hexIndex).localCoords();

		glm::mat4 transform = glm::identity<glm::mat4>();
		transform = glm::translate(transform, worldOffset);
		transform = glm::scale(transform, { 1.15f, 1.15f, 1.15f });


		outlineConstants.mvpMatrix = vpMatrix * transform;
		outlineConstants.color = { 0.5f, 0.00f, 0.00f, 0.5f };

		buff->pushConstants(m_outlinePipelineLayout, 0, sizeof(e2::OutlineConstants), reinterpret_cast<uint8_t*>(&outlineConstants));
		buff->draw(hexSpec.indexCount, 1);
	}
	// subtractive step 
	for (glm::ivec2 const& hexIndex : m_outlineTiles)
	{
		glm::vec3 worldOffset = e2::Hex(hexIndex).localCoords();

		glm::mat4 transform = glm::identity<glm::mat4>();
		transform = glm::translate(transform, worldOffset);

		outlineConstants.mvpMatrix = vpMatrix * transform;
		outlineConstants.color = { 0.5f, 0.04f, 0.03f, 0.0f };

		buff->pushConstants(m_outlinePipelineLayout, 0, sizeof(e2::OutlineConstants), reinterpret_cast<uint8_t*>(&outlineConstants));
		buff->draw(hexSpec.indexCount, 1);
	}
	buff->endRender();
	buff->useAsDefault(m_outlineTexture);

	//fog of war


	e2::FogOfWarConstants fogOfWarConstants;

	buff->useAsAttachment(m_fogOfWarMask[0]);
	buff->beginRender(m_fogOfWarTarget[0]);
	buff->bindPipeline(m_fogOfWarPipeline);

	// Bind vertex states
	buff->bindVertexLayout(hexSpec.vertexLayout);
	buff->bindIndexBuffer(hexSpec.indexBuffer);
	for (uint8_t i = 0; i < hexSpec.vertexAttributes.size(); i++)
		buff->bindVertexBuffer(i, hexSpec.vertexAttributes[i]);

	/*
	for (auto pair : m_chunkStates)
	{
		glm::ivec2 chunkIndex = pair.first;
		e2::ChunkState* chunk = pair.second;

		if (!chunk->visible)
		{
			continue;
		}

		glm::ivec2 chunkTileOffset = chunkIndex * glm::ivec2(e2::HexGridChunkResolution);

		glm::mat4 vpMatrix = m_viewpoints.view.calculateProjectionMatrix(m_viewpoints.resolution) * m_viewpoints.view.calculateViewMatrix();
		// first outlines
		for (int32_t y = 0; y < e2::HexGridChunkResolution; y++)
		{
			for (int32_t x = 0; x < e2::HexGridChunkResolution; x++)
			{
				glm::ivec2 worldIndex = chunkTileOffset + glm::ivec2(x, y);
				auto finder = m_tileIndex.find(worldIndex);
				if (finder == m_tileIndex.end())
					continue;

				e2::Hex currentHex(worldIndex);
				glm::vec3 worldOffset = currentHex.localCoords();

				glm::mat4 transform = glm::identity<glm::mat4>();
				transform = glm::translate(transform, worldOffset);
				transform = glm::scale(transform, { 1.15f, 1.15f, 1.15f });


				fogOfWarConstants.mvpMatrix = vpMatrix * transform;

				fogOfWarConstants.visibility.x = 0.0f;
				fogOfWarConstants.visibility.y = 0.0f;
				fogOfWarConstants.visibility.z = 1.0f;

				buff->pushConstants(m_fogOfWarPipelineLayout, 0, sizeof(e2::FogOfWarConstants), reinterpret_cast<uint8_t*>(&fogOfWarConstants));
				buff->draw(spec.indexCount, 1);
			}
		}
	}*/

	for (auto pair : m_chunkStates)
	{
		glm::ivec2 chunkIndex = pair.first;
		e2::ChunkState* chunk = pair.second;

		if (!chunk->visible)
		{
			continue;
		}

		glm::ivec2 chunkTileOffset = chunkIndex * glm::ivec2(e2::HexGridChunkResolution);

		glm::mat4 vpMatrix = m_viewpoints.view.calculateProjectionMatrix(m_viewpoints.resolution) * m_viewpoints.view.calculateViewMatrix();

		for (int32_t y = 0; y < e2::HexGridChunkResolution; y++)
		{
			for (int32_t x = 0; x < e2::HexGridChunkResolution; x++)
			{
				glm::ivec2 worldIndex = chunkTileOffset + glm::ivec2(x, y);
				auto finder = m_tileIndex.find(worldIndex);
				if (finder == m_tileIndex.end())
					continue;

				e2::Hex currentHex(worldIndex);
				glm::vec3 worldOffset = currentHex.localCoords();

				glm::mat4 transform = glm::identity<glm::mat4>();
				transform = glm::translate(transform, worldOffset);


				fogOfWarConstants.mvpMatrix = vpMatrix * transform;
				fogOfWarConstants.visibility.x = 1.0;
				fogOfWarConstants.visibility.y = m_tileVisibility[finder->second] > 0 ? 1.0 : 0.0;
				fogOfWarConstants.visibility.z = 0.0f;


				buff->pushConstants(m_fogOfWarPipelineLayout, 0, sizeof(e2::FogOfWarConstants), reinterpret_cast<uint8_t*>(&fogOfWarConstants));
				buff->draw(hexSpec.indexCount, 1);
			}
		}


	}

	buff->endRender();
	
	buff->useAsDefault(m_fogOfWarMask[0]);

	buff->setFrontFace(e2::FrontFace::CW);
	BlurConstants bc;
	bc.direction = { 1.0f, 0.0f };
	buff->useAsAttachment(m_fogOfWarMask[1]);
	buff->beginRender(m_fogOfWarTarget[1]);
	buff->bindPipeline(m_blurPipeline);
	buff->nullVertexLayout();
	buff->bindDescriptorSet(m_blurPipelineLayout, 0, m_blurSet[0]);
	buff->pushConstants(m_blurPipelineLayout, 0, sizeof(e2::BlurConstants), reinterpret_cast<uint8_t*>(&bc));
	buff->drawNonIndexed(3, 1);
	buff->endRender();
	buff->useAsDefault(m_fogOfWarMask[1]);

	bc.direction = { 0.0f, 1.0f };
	buff->useAsAttachment(m_fogOfWarMask[0]);
	buff->beginRender(m_fogOfWarTarget[0]);
	buff->bindPipeline(m_blurPipeline);
	buff->nullVertexLayout();
	buff->bindDescriptorSet(m_blurPipelineLayout, 0, m_blurSet[1]);
	buff->pushConstants(m_blurPipelineLayout, 0, sizeof(e2::BlurConstants), reinterpret_cast<uint8_t*>(&bc));
	buff->drawNonIndexed(3, 1);
	buff->endRender();
	buff->useAsDefault(m_fogOfWarMask[0]);


	buff->endRecord();

	renderManager()->queue(buff, nullptr, nullptr);
}

void e2::HexGrid::destroyFogOfWar()
{
	if (m_blurSet[0])
		e2::discard(m_blurSet[0]);

	if (m_blurSet[1])
		e2::discard(m_blurSet[1]);

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

	if (m_fogOfWarPipelineLayout)
		e2::discard(m_fogOfWarPipelineLayout);

	if (m_fogOfWarMask[0])
		e2::discard(m_fogOfWarMask[0]);


	if (m_fogOfWarMask[1])
		e2::discard(m_fogOfWarMask[1]);

	if (m_fogOfWarTarget[0])
		e2::discard(m_fogOfWarTarget[0]);

	if (m_fogOfWarTarget[1])
		e2::discard(m_fogOfWarTarget[1]);

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

}

void e2::HexGrid::clearVisibility()
{
	for (bool vis : m_tileVisibility)
		vis = 0;
}

void e2::HexGrid::flagVisible(glm::ivec2 const& v, bool onlyDiscover)
{
	auto finder = m_tileIndex.find(v);
	int32_t index = 0;
	if (finder == m_tileIndex.end())
	{
		index = discover(e2::Hex(v));

	}
	else
	{
		index = finder->second;
	}

	if(!onlyDiscover)
		m_tileVisibility[index] = m_tileVisibility[index]  + 1;
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
	index = finder->second;

	m_tileVisibility[index] = m_tileVisibility[index] - 1;
}

bool e2::HexGrid::isVisible(glm::ivec2 const& v)
{
	auto finder = m_tileIndex.find(v);
	if (finder == m_tileIndex.end())
	{
		return false;
	}
	int32_t index = finder->second;
	return m_tileVisibility[index];
}

void e2::HexGrid::clearOutline()
{
	m_outlineTiles.clear();
}

void e2::HexGrid::pushOutline(glm::ivec2 const& tile)
{
	m_outlineTiles.push_back(tile);
}

e2::ITexture* e2::HexGrid::outlineTexture()
{
	return m_outlineTexture;
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
	//newChunkMesh->calculateFaceNormals();
	//newChunkMesh->calculateVertexNormals();
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

		state->proxy = e2::create<e2::MeshProxy>(m_session, proxyConf);
		state->proxy->modelMatrix = glm::translate(glm::mat4(1.0f), chunkOffset);
		state->proxy->modelMatrixDirty = { true };
		m_numChunkMeshes++;
	}

	if (!state->waterProxy)
	{
		e2::MeshProxyConfiguration waterConf;
		waterConf.mesh = m_waterChunk;

		state->waterProxy = e2::create<e2::MeshProxy>(m_session, waterConf);
		state->waterProxy->modelMatrix = glm::translate(glm::mat4(1.0f), chunkOffset + glm::vec3(0.0f, 0.1f, 0.0f));
		state->waterProxy->modelMatrixDirty = { true };
	}
	if (!state->fogProxy)
	{
		e2::MeshProxyConfiguration fogConf;
		fogConf.mesh = m_fogChunk;

		state->fogProxy = e2::create<e2::MeshProxy>(m_session, fogConf);
		state->fogProxy->modelMatrix = glm::translate(glm::mat4(1.0f), chunkOffset + glm::vec3(0.0f, -1.0f, 0.0f));
		state->fogProxy->modelMatrixDirty = { true };
	}
}

void e2::HexGrid::ensureChunkHidden(e2::ChunkState* state)
{
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
		m_numChunkMeshes--;
	}
}

e2::ChunkState::~ChunkState()
{

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

e2::TileFlags e2::TileData::getImprovement()
{
	return (flags & e2::TileFlags::ImprovementMask);
}
