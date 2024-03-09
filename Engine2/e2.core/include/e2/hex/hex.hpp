
#pragma once 

#include <e2/export.hpp>
#include <e2/utils.hpp>

#include <e2/renderer/meshproxy.hpp>
#include <e2/managers/rendermanager.hpp>
#include <e2/dmesh/dmesh.hpp>
#include <e2/renderer/shadermodels/water.hpp>
#include <e2/renderer/shadermodels/terrain.hpp>
#include <e2/renderer/shadermodels/fog.hpp>

#include <vector>
#include <unordered_map>

namespace e2
{

	enum class TileFlags : uint16_t
	{
		None					= 0b0000'0000'0000'0000,

		// Hills is a special little baby, we love it very dearly but it only applies to grasslands, desert, and tundra biomes. it's irrelevant (and 0) for the others
		// if we find specific usecases where we need a bit for any of the other biomes, we can reuse this!
		// @todo this is actually unused 
		HillsMask				= 0b0000'0000'0000'0001,


		BiomeMask				= 0b1110'0000'0000'0000,
		BiomeGrassland			= 0b0000'0000'0000'0000,
		BiomeForest				= 0b0010'0000'0000'0000,
		BiomeDesert				= 0b0100'0000'0000'0000,
		BiomeTundra				= 0b0110'0000'0000'0000,
		BiomeMountain			= 0b1000'0000'0000'0000,
		BiomeShallow			= 0b1010'0000'0000'0000,
		BiomeOcean				= 0b1100'0000'0000'0000,
		BiomeReserved0			= 0b1110'0000'0000'0000,

		ResourceMask			= 0b0001'1100'0000'0000,
		ResourceNone			= 0b0000'0000'0000'0000, // no resource here
		ResourceForest			= 0b0000'0100'0000'0000, // improve to saw mill, provides wood 
		ResourceStone			= 0b0000'1000'0000'0000, // improve to quarry, provides stone 
		ResourceOre				= 0b0000'1100'0000'0000, // improve to ore mine, provides metal
		ResourceGold			= 0b0001'0000'0000'0000, // improve to gold mine, provides gold
		ResourceOil				= 0b0001'0100'0000'0000, // improve to oil derrick, provides oil
		ResourceUranium			= 0b0001'1000'0000'0000, // improve to uranium mine, provides uranium
		ResourceReserved0		= 0b0001'1100'0000'0000,

		ImprovementMask			= 0b0000'0011'1100'0000,
		ImprovementNone			= 0b0000'0000'0000'0000,

		// Resource improvement, what resource is in resource flags
		ImprovementResource		= 0b0000'0000'0100'0000,
		// Barracks, increases max unit count, builds infantry units 
		ImprovementBarracks		= 0b0000'0000'1000'0000,
		// Manufacturing plant, builds vehicle units
		ImprovementFactory		= 0b0000'0000'1100'0000,
		// Forward base, increases build range for other improvements, can be built anywhere in sight range
		// units can be teleported between forward bases limitlessly and instantly
		ImprovementForwardBase	= 0b0000'0001'0000'0000,
		// Airbase, builds air units
		ImprovementAirbase		= 0b0000'0001'0100'0000,
		// Research center, unlocks improvement and unit upgrades, as well as other upgrades such as walls (wood, stone, steel)
		ImprovementResearch		= 0b0000'0001'1000'0000,
		// Pillbox, stealth defensive building, short sight range, short attack range, high damage
		ImprovementPillbox		= 0b0000'0001'1100'0000,
		// Artillery base, offensive building, short sight range, long attack range, high damage
		ImprovementArtillery	= 0b0000'0010'0000'0000,
		// Seaport, builds naval units 
		ImprovementSeaport		= 0b0000'0010'0100'0000,
		// Guard tower, defensive building, long sight range, short attack range, low damage (upgradeable)
		ImprovementGuardTower	= 0b0000'0010'1000'0000,

		// Wall defense, globally upgradeable via research center 
		ImprovementWall			= 0b0000'0010'1100'0000,

		ImprovementReserved6	= 0b0000'0011'0000'0000,
		ImprovementReserved7	= 0b0000'0011'0100'0000,
		ImprovementReserved8	= 0b0000'0011'1000'0000,
		ImprovementReserved9	= 0b0000'0011'1100'0000,
		
		// These are special, and holds a combo of ImprovementResource and Resource*
		ResourceImprovementMask			= ImprovementMask		| ResourceMask, // all resources provide production
		ResourceImprovementSawMill		= ImprovementResource	| ResourceForest, // provides wood 
		ResourceImprovementQuarry		= ImprovementResource	| ResourceStone, // provides stone 
		ResourceImprovementOreMine		= ImprovementResource	| ResourceOre, // provides metal
		ResourceImprovementGoldMine		= ImprovementResource	| ResourceGold, // provides gold
		ResourceImprovementOilDerrick	= ImprovementResource	| ResourceOil, // provides oil
		ResourceImprovementUraniumMine	= ImprovementResource	| ResourceUranium, // provides uranium
		ResourceImprovementReserved0	= ImprovementResource	| ResourceReserved0, // provides reserved0

		// Abundance of the given resources on this tile
		AbundanceMask	= 0b0000'0000'0011'0000,
		Abundance1		= 0b0000'0000'0000'0000,
		Abundance2		= 0b0000'0000'0001'0000,
		Abundance3		= 0b0000'0000'0010'0000,
		Abundance4		= 0b0000'0000'0011'0000,

		Reserved		= 0b0000'0000'0000'1110,
	};

	EnumFlagsDeclaration(TileFlags);

	enum class ImprovementFlags : uint8_t
	{
		None			= 0b0000'0000,
		DefenseMask		= 0b0000'0011,
		Defense0		= 0b0000'0000,
		Defense1		= 0b0000'0001,
		Defense2		= 0b0000'0010,
		Defense3		= 0b0000'0011,

		UpgradesMask	= 0b1111'1100,
		Upgrade0		= 0b0000'0100,
		Upgrade1		= 0b0000'1000,
		Upgrade2		= 0b0001'0000,
		Upgrade3		= 0b0010'0000,
		Upgrade4		= 0b0100'0000,
		Upgrade5		= 0b1000'0000,
	};

	EnumFlagsDeclaration(ImprovementFlags);

	/** */
	struct E2_API TileData
	{
		// flags control biome, resource, improvements
		TileFlags flags{ TileFlags::None }; // 16 bits

		TileFlags getBiome();
		TileFlags getResource();
		TileFlags getAbundance();
		TileFlags getImprovement();

		uint8_t empireId{255}; // 255 means no empire claim this, 254 empire ids that are recycled (max 254 concurrent empires)

		ImprovementFlags improvementFlags{ ImprovementFlags::None };
		e2::ufloat8 improvementHealth{ 1.0f };
	};

	constexpr uint32_t hexChunkResolution = 6;

	constexpr uint32_t maxNumExtraChunks = 256;
	constexpr uint32_t maxNumChunkStates = 512;

	constexpr uint32_t maxNumChunkLoadTasks = 256;
	constexpr uint32_t maxNumTreesPerChunk = hexChunkResolution * hexChunkResolution;

	class HexGrid;

	/** @tags(arena, arenaSize=e2::maxNumChunkLoadTasks)  */
	class E2_API ChunkLoadTask : public e2::AsyncTask
	{
		ObjectDeclaration()
	public:
		ChunkLoadTask(e2::HexGrid* grid, glm::ivec2 const& chunkIndex);
		virtual ~ChunkLoadTask();

		virtual bool prepare() override;
		virtual bool execute() override;
		virtual bool finalize() override;
	
		e2::HexGrid* m_grid{};
		glm::ivec2 m_chunkIndex;

		e2::MeshPtr m_generatedMesh;

		e2::DynamicMesh* m_dynaHex{};
		e2::DynamicMesh* m_dynaHexHigh{};

		e2::StackVector<glm::ivec2, e2::maxNumTreesPerChunk> treeOffsets;

		float m_ms;
	};

	enum class StreamState : uint8_t
	{
		/** Not initialized, nor streamed in */
		Poked,

		/** This chunk is queued for streaming, and waiting patiently */
		Queued,

		/** Mesh currently streaming in */
		Streaming,

		/** Ready to be fully visible */
		Ready
	};

	/** @tags(arena, arenaSize=e2::maxNumChunkStates)  */
	class E2_API ChunkState : public e2::Object
	{
		ObjectDeclaration()
	public:

		ChunkState() = default;
		virtual ~ChunkState();

		// chunk index in offset coordinates 
		glm::ivec2 chunkIndex;

		StreamState streamState{ StreamState::Poked };
		bool visibilityState{ false };
		
		// task, if relevant
		e2::AsyncTaskPtr task;

		// true if visible this frame
		bool inView{};
		e2::Moment lastTimeInView;

		// only for loaded 
		e2::MeshPtr mesh;

		// only for visible 
		e2::MeshProxy* proxy{};
		e2::MeshProxy* waterProxy{};
		e2::MeshProxy* fogProxy{};

		e2::StackVector<glm::ivec2, e2::maxNumTreesPerChunk> forestTileIndices;
		e2::StackVector<e2::MeshProxy*, e2::maxNumTreesPerChunk> forestTileProxies;

	};

	// fog of war 
	struct FogOfWarConstants
	{
		glm::mat4 mvpMatrix;
		glm::vec3 visibility;
	};

	struct OutlineConstants
	{
		glm::mat4 mvpMatrix;
		glm::vec4 color;
	};

	struct BlurConstants
	{
		glm::vec2 direction;
	};

	struct MiniMapConstants
	{
		glm::vec2 viewCornerTL;
		glm::vec2 viewCornerTR;
		glm::vec2 viewCornerBL;
		glm::vec2 viewCornerBR;

		glm::vec2 worldMin;
		glm::vec2 worldMax;

		glm::uvec2 resolution;
	};


	/** 
	 * Procedural hex grid
	 * --
	 * Can grow arbitrarily and without limits
	 * 
	 */
	class E2_API HexGrid : public e2::Context
	{
	public:

		HexGrid(e2::Context* ctx, e2::GameSession* session);
		~HexGrid();

		virtual e2::Engine* engine() override;

		/// Chunks Begin (and World Streaming)

		e2::ChunkState* getOrCreateChunk(glm::ivec2 const& index);
		void nukeChunk(e2::ChunkState* chunk);

		/** queues a chunk for streaming, will do nothing if not unstreamed */
		void queueStreamingChunk(e2::ChunkState* state);

		/** starts streaming a queued chunk, will do nothing if not queued first */
		void startStreamingChunk(e2::ChunkState* state);

		/** finalizes a streaming chunk */
		void endStreamingChunk(glm::ivec2 const& chunkIndex, e2::MeshPtr newMesh, double timeMs, e2::StackVector<glm::ivec2, e2::maxNumTreesPerChunk>* treeIndices);

		/** pops in chunk, no questions asked, self-corrective states and safe to call whenever  */
		void popInChunk(e2::ChunkState* state);

		/** pops out chunk, no questions asked, self-corrective states and safe to call whenever */
		void popOutChunk(e2::ChunkState* state);

		// owning map as well as index from chunk index -> chunk state
		std::unordered_map<glm::ivec2, e2::ChunkState*> m_chunkIndex;

		/** jsut a bunhc of utilikty sets */
		std::unordered_set<e2::ChunkState*> m_queuedChunks;
		std::unordered_set<e2::ChunkState*> m_streamingChunks;
		std::unordered_set<e2::ChunkState*> m_invalidatedChunks;
		std::unordered_set<e2::ChunkState*> m_visibleChunks;
		std::unordered_set<e2::ChunkState*> m_hiddenChunks;
		std::unordered_set<e2::ChunkState*> m_chunksInView;
		std::unordered_set<e2::ChunkState*> m_lookAheadChunks;


		void updateStreaming(glm::vec2 const& streamCenter, e2::Viewpoints2D const& newStreamingView, glm::vec2 const& viewVelocity);



		glm::vec2 m_streamingCenter;
		glm::vec2 m_streamingViewVelocity;
		e2::Viewpoints2D m_streamingView;
		e2::Aabb2D m_streamingViewAabb;

		std::vector<e2::ChunkState*> m_streamingChunksInView;



		/** Clears all chunks from world streaming */
		void clearAllChunks();

		/** Retrieves the world AABB of a chunk, given it's chunk index */
		e2::Aabb2D getChunkAabb(glm::ivec2 const& chunkIndex);

		/** Retrieves the size of a chunk in planar coords */
		glm::vec2 chunkSize();

		/** Retrieves chunk index from world planar coords */
		glm::ivec2 chunkIndexFromPlanarCoords(glm::vec2 const& planarCoords);

		/** Retrieves the chunk offset position in world coords, given a chunk index */
		static glm::vec3 chunkOffsetFromIndex(glm::ivec2 const& index);

		/** Number of chunk states */
		uint32_t numChunks();

		/** Number of chunks deemed relevant */
		uint32_t numVisibleChunks();

		/** Number of currently streaming chunks */
		uint32_t numJobsInFlight();

		/** Puts this chunk index in queue to be streamed in, if it's not already streamed in or streaming */
		//void prepareChunk(glm::ivec2 const& chunkIndex);

		/** Called by streaming thread when it finishes, this simply finalizes a chunk (if it still exists) */
		//void notifyChunkReady(glm::ivec2 const& chunkIndex, e2::MeshPtr generatedMesh, double ms, e2::StackVector<glm::vec4, e2::maxNumTreesPerChunk>* offsets);

		/** Ensures all chunks within the range are visible, given range in planar coords, updates them as visible, nukes any excessive hcunks */
		//void assertChunksWithinRangeVisible(glm::vec2 const& streamCenter, e2::Viewpoints2D const& viewPoints, glm::vec2 const& viewVelocity);

	protected:

		double m_highLoadTime{};

		// temp vectors
		//std::vector<glm::ivec2> m_chunkStreamQueue;
		//std::vector<e2::ChunkState*> m_hiddenChunks;


		// set of discovered chunks and its aabb, for minimap and its fog but can also be used for other stuff 
		std::unordered_set<glm::ivec2> m_discoveredChunks;
		e2::Aabb2D m_discoveredChunksAABB;




		/// Chunks End


		/// Tiles Begin
	public:
		/** Retrieves TileData from the given array index */
		TileData& getTileFromIndex(size_t index);

		/** Discovers the given hex, and returns its new array index (WARNING: This doesnt check if it already exists!) */
		size_t discover(Hex hex);

		/** Retrieves array index to the given tile, discovers it if necessary */
		size_t getTileIndexFromHex(Hex hex);

		/** Retrieves tile data for the given hex, if it exists. Otherwise returns null */
		e2::TileData* getTileData(glm::ivec2 const& hex);

		/// Tiles End

		/// ProcGen Begin

		double highLoadTime();
		void clearLoadTime();

		/** */
		static e2::TileData calculateTileDataForHex(Hex hex);

		static float sampleSimplex(glm::vec2 const& position, float scale);

		static float sampleBaseHeight(glm::vec2 const& position);
		



		


		e2::DynamicMesh& dynamicHex()
		{
			return m_dynaHex;
		}
		
		e2::DynamicMesh &dynamicHexHigh()
		{
			return m_dynaHexHigh;
		}

		e2::MaterialPtr hexMaterial()
		{
			return m_terrainMaterial;
		}


		std::mutex& dynamicMutex()
		{
			return m_dynamicMutex;
		}

		e2::ITexture* fogOfWarMask()
		{
			return m_fogOfWarMask[0];
		}

		// fog of war in this function actually means fog of war, outlines and blur 
		void initializeFogOfWar();
		void invalidateFogOfWarRenderTarget(glm::uvec2 const& newResolution);
		void invalidateFogOfWarShaders();
		void renderFogOfWar();
		void destroyFogOfWar();

		void clearVisibility();
		void flagVisible(glm::ivec2 const& v, bool onlyDiscover = false);
		void unflagVisible(glm::ivec2 const& v);
		bool isVisible(glm::ivec2 const& v);

		void clearOutline();
		void pushOutline(glm::ivec2 const& tile);

		e2::ITexture* outlineTexture();

		e2::ITexture* minimapTexture();

	protected:

		int32_t m_numThreads{4};


		e2::Engine* m_engine{};
		e2::GameSession* m_session{};

		std::mutex m_dynamicMutex;

//		e2::Viewpoints2D m_viewpoints;



		//
		// list of discovered tiles 
		std::vector<TileData> m_tiles;
		std::vector<int32_t> m_tileVisibility;
		std::unordered_map<Hex, size_t> m_tileIndex;



		e2::MeshPtr m_treeMesh[4];

		e2::MeshPtr m_baseHex;
		e2::DynamicMesh m_dynaHex;
		e2::MeshPtr m_baseHexHigh;
		e2::DynamicMesh m_dynaHexHigh;

		e2::MaterialPtr m_terrainMaterial;
		e2::TerrainProxy* m_terrainProxy{};

		e2::MaterialPtr m_waterMaterial;
		e2::WaterProxy* m_waterProxy{};

		e2::MaterialPtr m_fogMaterial;
		e2::FogProxy* m_fogProxy{};

		e2::MeshPtr m_waterChunk;
		e2::MeshPtr m_fogChunk;


		std::vector<glm::ivec2> m_outlineTiles;


		e2::IRenderTarget* m_fogOfWarTarget[2] = {nullptr, nullptr};
		e2::ITexture* m_fogOfWarMask[2] = {nullptr, nullptr};
		glm::uvec2 m_fogOfWarMaskSize{};
		e2::IShader* m_fogOfWarVertexShader{};
		e2::IShader* m_fogOfWarFragmentShader{};
		e2::IPipelineLayout* m_fogOfWarPipelineLayout{};
		e2::IPipeline* m_fogOfWarPipeline{};

		e2::IShader* m_blurFragmentShader{};
		e2::IPipelineLayout* m_blurPipelineLayout{};
		e2::IPipeline* m_blurPipeline{};
		e2::IDescriptorSetLayout* m_blurSetLayout{};
		e2::IDescriptorSet* m_blurSet[2] = {nullptr, nullptr};
		e2::IDescriptorPool* m_blurPool{};

		e2::IRenderTarget* m_outlineTarget { nullptr };
		e2::ITexture* m_outlineTexture { nullptr };
		e2::IShader* m_outlineVertexShader{};
		e2::IShader* m_outlineFragmentShader{};
		e2::IPipelineLayout* m_outlinePipelineLayout{};
		e2::IPipeline* m_outlinePipeline{};



		glm::uvec2 m_minimapSize{};
		e2::IRenderTarget* m_minimapTarget{};
		e2::ITexture* m_minimapTexture{};
		e2::IShader* m_minimapFragmentShader{};
		e2::IPipelineLayout* m_minimapPipelineLayout{};
		e2::IPipeline* m_minimapPipeline{};

	public:
		void updateWorldBounds();

		e2::Aabb2D const& worldBounds()
		{
			return m_worldBounds;
		}

		void minimapZoom(float newZoom)
		{
			m_minimapViewZoom = newZoom;
		}

		float minimapZoom()
		{
			return m_minimapViewZoom;
		}

		glm::vec2 const& minimapOffset()
		{
			return m_minimapViewOffset;
		}

		void minimapOffset(glm::vec2 const& newOffset)
		{
			m_minimapViewOffset = newOffset;
		}

	protected:
		/** the navigatable world bounds, i.e. discoveryaabb + margin */
		e2::Aabb2D m_worldBounds{};
		e2::Aabb2D m_minimapViewBounds{};
		glm::vec2 m_minimapViewOffset{};
		float m_minimapViewZoom{ 1.0f };

		// does more than fogofwar, actually renders everything custom this hex grid needs
		// i.e. fog of war, outlines, minimap, etc. 
		e2::Pair<e2::ICommandBuffer*> m_fogOfWarCommandBuffers{ nullptr };

private:
	void debugDraw();
	};

}





#include "hex.generated.hpp"
