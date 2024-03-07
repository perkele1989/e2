
#pragma once 

#include <e2/export.hpp>
#include <e2/utils.hpp>

#include <e2/renderer/meshproxy.hpp>
#include <e2/managers/rendermanager.hpp>
#include <e2/dmesh/dmesh.hpp>
#include <e2/renderer/shadermodels/water.hpp>
#include <e2/renderer/shadermodels/terrain.hpp>

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

	constexpr uint32_t HexGridChunkResolution = 6;

	constexpr uint32_t maxNumExtraChunks = 128;
	constexpr uint32_t maxNumChunkStates = 512;

	constexpr uint32_t maxNumChunkLoadTasks = 256;


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

		float m_ms;
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

		// if not null, then we are loading
		e2::AsyncTaskPtr task;

		// true if visible this frame
		bool visible{};
		e2::Moment lastVisible;

		// only for loaded 
		e2::MeshPtr mesh;

		// only for visible 
		e2::MeshProxy* proxy{};
		e2::MeshProxy* waterProxy{};

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

		glm::vec2 chunkSize();

		glm::ivec2 chunkIndexFromPlanarCoords(glm::vec2 const& planarCoords);

		static glm::vec3 chunkOffsetFromIndex(glm::ivec2 const& index);

		/** Ensures all chunks within the range are visible, given range in planar coords, updates them as visible, nukes any excessive hcunks */
		void assertChunksWithinRangeVisible(glm::vec2 const& streamCenter, e2::Viewpoints2D const& viewPoints, glm::vec2 const& viewVelocity);

		static float sampleSimplex(glm::vec2 const& position, float scale);

		static float sampleBaseHeight(glm::vec2 const& position);

		static e2::TileData calculateTileDataForHex(Hex hex);

		size_t discover(Hex hex);

		/** Retrieves reference to the given tile, creates it if necessary */
		size_t getTileIndexFromHex(Hex hex);

		e2::TileData* getTileData(glm::ivec2 const & hex);

		TileData &getTileFromIndex(size_t index);

		void clearAllChunks();

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

		/** Number of queried chunks */
		uint32_t numChunks();

		/** Number of chunks deemed relevant */
		uint32_t numVisibleChunks();

		uint32_t numJobsInFlight();

		/** Number of chunks which has a mesh attached (some are empty due to being completely submerged)  */
		uint32_t numChunkMeshes();
		double highLoadTime();
		void clearLoadTime();


		void prepareChunk(glm::ivec2 const& chunkIndex);
		void notifyChunkReady(glm::ivec2 const& chunkIndex, e2::MeshPtr generatedMesh, double ms);

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

	protected:



		e2::Engine* m_engine{};
		e2::GameSession* m_session{};

		std::mutex m_dynamicMutex;

		e2::Viewpoints2D m_viewpoints;

		uint32_t m_numVisibleChunks{};
		uint32_t m_numChunkMeshes{};
		uint32_t m_numJobsInFlight{};
		double m_highLoadTime{};

		// temp vectors
		//std::vector<glm::ivec2> m_visibleChunkIndices;
		std::vector<glm::ivec2> m_chunkStreamQueue;
		std::vector<e2::ChunkState*> m_hiddenChunks;
		//

		std::vector<TileData> m_tiles;
		std::vector<int32_t> m_tileVisibility;
		std::unordered_map<Hex, size_t> m_tileIndex;
		
		void ensureChunkVisible(e2::ChunkState* state);
		void ensureChunkHidden(e2::ChunkState* state);

		std::unordered_map<glm::ivec2, e2::ChunkState*> m_chunkStates;

		e2::MeshPtr m_baseHex;
		e2::DynamicMesh m_dynaHex;
		e2::MeshPtr m_baseHexHigh;
		e2::DynamicMesh m_dynaHexHigh;

		e2::MaterialPtr m_terrainMaterial;
		e2::TerrainProxy* m_terrainProxy{};

		e2::MaterialPtr m_waterMaterial;
		e2::WaterProxy* m_waterProxy{};

		e2::MeshPtr m_waterChunk;


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

		// does more than fogofwar, actually renders everything custom this hex grid needs
		e2::Pair<e2::ICommandBuffer*> m_fogOfWarCommandBuffers{ nullptr };

	};

}





#include "hex.generated.hpp"
