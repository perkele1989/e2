
#pragma once 

#include <e2/export.hpp>
#include <e2/utils.hpp>

#include <e2/renderer/meshproxy.hpp>
#include <e2/managers/rendermanager.hpp>
#include <e2/dmesh/dmesh.hpp>
#include <e2/renderer/shadermodels/water.hpp>
#include <e2/renderer/shadermodels/terrain.hpp>
#include <e2/renderer/shadermodels/custom.hpp>
#include <e2/renderer/shadermodels/fog.hpp> 

#include <e2/assets/sound.hpp>
#include "game/gamecontext.hpp"
#include "game/shared.hpp"

#include <vector>
#include <unordered_map>

namespace e2
{
	// DONT EVER CHANGE THESE vvv
	inline float mtnFreqScale = 0.1f;
	inline float mtnScale = 0.9f;

	inline float treeScale = 0.623f;
	inline float treeSpread = 1.0f;
	inline int32_t treeNum1 = 9;
	inline int32_t treeNum2 = 14;
	inline int32_t treeNum3 = 23;
	// DONT EVER CHANGE THESE ^^^

	class GameUnit;

	//struct MeshTreeLods
	//{
	//	e2::MeshPtr lods[3];
	//};

	struct TileData;



	constexpr float maxTreeFallTime = 4.0f;
	struct UnpackedTreeState
	{
		

		uint32_t meshIndex{};
		e2::MeshProxy* mesh{};
		
		glm::vec2 worldOffset;
		glm::vec2 fallDir;
		float rotation;
		float scale;

		float health{ 1.0f }; // if 0, this tree is harvested, will fall and die
		float fallTime{ e2::maxTreeFallTime }; // if > 0 and health 0.0, currently falling, otherwise fallen and dying
		float killTime{ 1.0f }; // if > 0 and falltime 0.0, currently dying, otherwise dead
		float shake{ 0.0f };
		
		uint32_t totalLumber{};
		uint32_t spawnedLumber{};

	};

	struct UnpackedForestState
	{
		e2::Hex hex;
		std::vector<UnpackedTreeState> trees;
	};

	struct TreeState
	{
		uint32_t meshIndex{};
		glm::vec2 planarOffset{};
		float rotation{};
		float scale{};


		glm::vec2 localOffset(e2::TileData *tile, struct ForestState *forestState);


	};

	struct ForestState
	{
		e2::MeshPtr mesh;
		std::vector<TreeState> trees;
	};




	enum class TileFlags : uint16_t
	{
		None					= 0b0000'0000'0000'0000,

		BiomeMask				= 0b0000'0000'0000'0011,
		BiomeGrassland			= 0b0000'0000'0000'0000,
		BiomeDesert				= 0b0000'0000'0000'0001,
		BiomeTundra				= 0b0000'0000'0000'0010,
		BiomeReserved0			= 0b0000'0000'0000'0011,

		FeatureMask				= 0b0000'0000'0000'1100,
		FeatureNone				= 0b0000'0000'0000'0000,
		FeatureMountains		= 0b0000'0000'0000'0100,
		FeatureForest			= 0b0000'0000'0000'1000,

		WaterMask				= 0b0000'0000'0011'0000,
		WaterNone				= 0b0000'0000'0000'0000,
		WaterShallow			= 0b0000'0000'0001'0000,
		WaterDeep				= 0b0000'0000'0010'0000,
		WaterReserved0			= 0b0000'0000'0011'0000,

		ResourceMask			= 0b0000'0001'1100'0000,
		ResourceNone			= 0b0000'0000'0000'0000,
		ResourceStone			= 0b0000'0000'0100'0000,
		ResourceOre				= 0b0000'0000'1000'0000,
		ResourceGold			= 0b0000'0000'1100'0000,
		ResourceOil				= 0b0000'0001'0000'0000,
		ResourceUranium			= 0b0000'0001'0100'0000,
		ResourceReserved0		= 0b0000'0001'1000'0000,
		ResourceReserved1		= 0b0000'0001'1100'0000,

		// Abundance of the given resources on this tile
		AbundanceMask			= 0b0000'0110'0000'0000,
		Abundance1				= 0b0000'0000'0000'0000,
		Abundance2				= 0b0000'0010'0000'0000,
		Abundance3				= 0b0000'0100'0000'0000,
		Abundance4				= 0b0000'0110'0000'0000,

		// Abundance of forest/wood on this tile
		WoodAbundanceMask		= 0b0001'1000'0000'0000,
		WoodAbundance1			= 0b0000'0000'0000'0000,
		WoodAbundance2			= 0b0000'1000'0000'0000,
		WoodAbundance3			= 0b0001'0000'0000'0000,
		WoodAbundance4			= 0b0001'1000'0000'0000,

		Reserved0				= 0b1110'0000'0000'0000
	};

	EnumFlagsDeclaration(TileFlags);



	/** */
	struct TileData
	{
		bool isPassable(PassableFlags passableFlags);

		bool isShallowWater();
		bool isDeepWater();
		bool isLand();
		bool isMountain();

		bool hasGold();
		bool hasOil();
		bool hasOre();
		bool hasStone();
		bool hasUranium();

		TileFlags getWater();

		TileFlags getFeature();

		TileFlags getBiome();

		TileFlags getResource();

		TileFlags getAbundance();

		TileFlags getWoodAbundance();

		bool isForested();
		bool hasResource();

		float getAbundanceAsFloat();
		float getWoodAbundanceAsFloat();

		// flags control biome, resources, abundance
		TileFlags flags{ TileFlags::None }; // 16 bits

		// optional forest proxy, if this tile is discovered it shows: forest abundance if unbuilt on forest, partly forest if built on forest, nothing if not on forest or if undiscovered (undiscovered tile meshes lives on chunk array)
		int32_t forestIndex{-1};
		e2::MeshProxy* forestProxy{};
		float forestRotation{};


		e2::MeshProxy* resourceProxy{};
		e2::MeshPtr resourceMesh;

	};

	constexpr uint32_t hexChunkResolution = 6;

	constexpr uint32_t maxNumExtraChunks = 1024;
	constexpr uint32_t maxNumChunkStates = 4096;

	constexpr uint32_t maxNumChunkLoadTasks = 4096;
	constexpr uint32_t maxNumMeshesPerChunk = (hexChunkResolution * hexChunkResolution)*3;

	class HexGrid;

	//struct ForestIndex
	//{
	//	glm::ivec2 offsetCoords;
	//	uint32_t meshIndex{};
	//};

	/** @tags(arena, arenaSize=e2::maxNumChunkLoadTasks)  */
	class ChunkLoadTask : public e2::AsyncTask
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

		/*e2::DynamicMesh* m_dynaHex{};
		e2::DynamicMesh* m_dynaHexHigh{};*/
		e2::FastMesh2* m_fastHex{};
		e2::FastMesh2* m_fastHexHigh{};

		e2::StackVector<e2::MeshProxy*, e2::maxNumMeshesPerChunk> forestMeshes;

		bool m_hasWaterTile{};

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
	class ChunkState : public e2::ManagedObject
	{
		ObjectDeclaration()
	public:

		ChunkState() = default;
		virtual ~ChunkState();

		// chunk index in offset coordinates 
		glm::ivec2 chunkIndex;

		bool hasWaterTile{};

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


		e2::StackVector<e2::MeshProxy*, e2::maxNumMeshesPerChunk> extraMeshes;

	};

	// fog of war 
	struct FogOfWarConstants
	{
		glm::mat4 mvpMatrix;
		glm::vec4 visibility;
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


	enum class OutlineLayer : uint8_t
	{
		Movement = 0,
		Attack,

		Count
	};

	constexpr float waterLine = 0.025f;


	struct GrassCutMaskShiftConstants
	{
		glm::vec2 resolution;
		glm::vec2 areaSize;
		glm::vec2 moveOffset;
		float timeDelta;

	};


	struct GrassCutMaskAddConstants
	{
		glm::vec2 resolution;
		glm::vec2 areaSize;
		glm::vec2 areaCenter;
		glm::vec2 position;
		float radius;
	};


	struct GrassCutMaskPushConstants
	{
		glm::vec2 resolution;
		glm::vec2 areaSize;
		glm::vec2 areaCenter;
		glm::vec2 position;
		float radius;
	};

	struct GrassPush
	{
		glm::vec2 position;
		float radius;
	};


	struct GrassCut
	{
		glm::vec2 position;
		float radius;
	};

	constexpr uint64_t maxNumCuts = 16;
	constexpr uint64_t maxNumPush = 512;

	class GrassCutMask : public e2::GameContext, public e2::Context
	{
	public:
		GrassCutMask(e2::GameContext* ctx, uint32_t resolution, float areaSize);
		virtual ~GrassCutMask();

		void recordFrame(e2::ICommandBuffer *buffer, uint8_t frameIndex);

		virtual e2::Engine* engine() override;
		virtual e2::Game* game() override;

		void setCenter(glm::vec2 const& worldPlanarCoords);

		void cut(GrassCut const& cut);
		void push(GrassPush const& push);

		e2::ITexture* getTexture(uint8_t frameIndex);

		inline float getAreaSize()
		{
			return m_areaSize;
		}

		inline uint32_t getResolution()
		{
			return m_resolution;
		}

		inline glm::vec2 const& getCenter()
		{
			return m_center;
		}

	protected:

		e2::Game* m_game{};

		float m_areaSize;
		uint32_t m_resolution;

		glm::vec2 m_lastCenter;
		glm::vec2 m_center;

		e2::ITexture* m_textures[2];
		e2::IRenderTarget* m_targets[2];

		// shift stuff (for moving the view)
		e2::IDescriptorPool* m_shiftPool{};
		e2::IDescriptorSetLayout* m_shiftSetLayout{};
		e2::IDescriptorSet* m_shiftSets[2];
		e2::IShader* m_shiftVertexShader{};
		e2::IShader* m_shiftFragmentShader{};
		e2::IPipelineLayout* m_shiftPipelineLayout{};
		e2::IPipeline* m_shiftPipeline{};

		// add stuff (for adding cuts)
		e2::IShader* m_addVertexShader{};
		e2::IShader* m_addFragmentShader{};
		e2::IPipelineLayout* m_addPipelineLayout{};
		e2::IPipeline* m_addPipeline{};
		
		e2::StackVector<GrassCut, e2::maxNumCuts> m_cuts;

		// push stuff (for adding object weights)
		e2::IShader* m_pushVertexShader{};
		e2::IShader* m_pushFragmentShader{};
		e2::IPipelineLayout* m_pushPipelineLayout{};
		e2::IPipeline* m_pushPipeline{};

		e2::StackVector<GrassPush, e2::maxNumPush> m_pushes;
		 
	};


	/** 
	 * Procedural hex grid
	 * --
	 * Can grow arbitrarily and without limits
	 * 
	 */
	class HexGrid : public e2::GameContext, public e2::Context
	{
	public:

		HexGrid(e2::GameContext* ctx);
		~HexGrid();

		static void prescribeAssets(e2::Context* ctx, e2::ALJDescription& desc);
		

		virtual e2::Engine* engine() override;
		virtual e2::Game* game() override;

		void buildForestStates();
		void buildForestMeshes();

		void saveToBuffer(e2::IStream& toBuffer);
		void loadFromBuffer(e2::IStream& fromBuffer);

		/// Chunks Begin (and World Streaming)

		e2::ChunkState* getOrCreateChunk(glm::ivec2 const& index);
		void nukeChunk(e2::ChunkState* chunk);

		/** queues a chunk for streaming, will do nothing if not unstreamed */
		void queueStreamingChunk(e2::ChunkState* state);

		/** starts streaming a queued chunk, will do nothing if not queued first */
		void startStreamingChunk(e2::ChunkState* state);

		/** finalizes a streaming chunk */
		void endStreamingChunk(glm::ivec2 const& chunkIndex, e2::MeshPtr newMesh, double timeMs, bool hasWaterTile);

		/** pops in chunk, no questions asked, self-corrective states and safe to call whenever  */
		void popInChunk(e2::ChunkState* state);

		/** pops out chunk, no questions asked, self-corrective states and safe to call whenever */
		void popOutChunk(e2::ChunkState* state);

		void refreshChunkMeshes(e2::ChunkState* state);

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

		std::unordered_set<e2::ChunkState*> m_outdatedChunks;


		void forceStreamView(e2::Viewpoints2D const& view);
		std::vector<e2::Viewpoints2D> m_forceStreamQueue;


		void updateStreaming(glm::vec2 const& streamCenter, e2::Viewpoints2D const& newStreamingView, glm::vec2 const& viewVelocity);

		void clearQueue();

		bool m_streamingPaused{};
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

		uint32_t numJobsInQueue();

		/** Puts this chunk index in queue to be streamed in, if it's not already streamed in or streaming */
		//void prepareChunk(glm::ivec2 const& chunkIndex);

		/** Called by streaming thread when it finishes, this simply finalizes a chunk (if it still exists) */
		//void notifyChunkReady(glm::ivec2 const& chunkIndex, e2::MeshPtr generatedMesh, double ms, e2::StackVector<glm::vec4, e2::maxNumTreesPerChunk>* offsets);

		void flagChunkOutdated(glm::ivec2 const& chunkIndex);
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
		size_t discover(glm::ivec2 const& hex);

		/** Retrieves array index to the given tile, discovers it if necessary */
		size_t getTileIndexFromHex(glm::ivec2 const &hex);

		/** Retrieves tile data for the given hex, if it exists. Otherwise returns null */
		e2::TileData* getExistingTileData(glm::ivec2 const& hex);
		e2::TileData calculateTileData(glm::ivec2 const& hex);
		e2::TileData getTileData(glm::ivec2 const& hex);
		/// Tiles End

		/// ProcGen Begin

		double highLoadTime();
		void clearLoadTime();

		/** */

		float calculateBaseHeight(glm::vec2 const& planarCoords);
		void calculateBiome(glm::vec2 const& planarCoords, e2::TileFlags& outFlags);
		void calculateResources(glm::vec2 const& planarCoords, e2::TileFlags& outFlags);
		void calculateFeaturesAndWater(glm::vec2 const& planarCoords, float baseHeight, e2::TileFlags& outFlags);

		e2::MeshPtr getResourceMeshForFlags(e2::TileFlags flags);

		int32_t getForestIndexForFlags(e2::TileFlags flags);
		ForestState* getForestState(int32_t index);

		void damageTree(glm::ivec2 const& hex, uint32_t treeIndex, float dmg);

		e2::UnpackedForestState* getUnpackedForestState(glm::ivec2 const& hex);
		e2::UnpackedForestState* unpackForestState(glm::ivec2 const& hex);

		e2::MeshProxy* createGrassProxy(e2::TileData* tileData, glm::ivec2 const& hex);
		e2::MeshProxy* createResourceProxyForTile(e2::TileData* tileData, glm::ivec2 const& hex);
		e2::MeshProxy* createForestProxyForTile(e2::TileData* tileData, glm::ivec2 const& hex);
		static float sampleSimplex(glm::vec2 const& position);

		e2::FastMesh2& fastHex()
		{
			return m_fastHex;
		}
		
		e2::FastMesh2& fastHexHigh()
		{
			return m_fastHexHigh;
		}

		e2::MaterialPtr hexMaterial()
		{
			return m_terrainMaterial;
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
		void pushOutline(e2::OutlineLayer layer, glm::ivec2 const& tile);

		e2::ITexture* outlineTexture(uint8_t frameIndex);

		e2::ITexture* minimapTexture(uint8_t frameIndex);

		void setFog(float newFog);



		void removeWood(glm::ivec2 const& location);
	protected:

		int32_t m_numThreads{4};


		e2::Game* m_game{};

		GrassCutMask m_cutMask;

	public:
		inline GrassCutMask& grassCutMask()
		{
			return m_cutMask;
		}

		void updateCutMask(glm::vec2 const& newCenter);
	protected:

		//
		// list of discovered tiles 
		std::vector<TileData> m_tiles;
		std::vector<int32_t> m_tileVisibility;
		std::unordered_map<glm::ivec2, uint64_t> m_tileIndex;

		void updateUnpackedForests();

		std::unordered_map<glm::ivec2, e2::UnpackedForestState> m_unpackedForests;

		e2::MeshPtr m_resourceMeshStone;

		e2::ForestState m_forestStates[3];

		e2::MeshPtr m_treeMeshes[4];
		e2::DynamicMesh m_dynamicTreeMeshes[4];
		e2::MaterialPtr m_treeMaterial;
		e2::MaterialProxy* m_treeMaterialProxy;

		e2::MaterialPtr m_grassMaterial;
		e2::CustomProxy* m_grassProxy{};





		e2::DynamicMesh m_grassLeafMesh;
		e2::MeshPtr m_grassMeshes[4];
		e2::DynamicMesh m_dynamicGrassMeshes[4];


		e2::SoundPtr m_treeChopSound;
		e2::SoundPtr m_treeFallSound;
		e2::SoundPtr m_woodDieSound;

		e2::MeshPtr m_baseHex;
		//e2::DynamicMesh m_dynaHex;
		e2::FastMesh2 m_fastHex;
		e2::MeshPtr m_baseHexHigh;
		e2::FastMesh2 m_fastHexHigh;
		//e2::DynamicMesh m_dynaHexHigh;

		e2::MaterialPtr m_terrainMaterial;
		e2::TerrainProxy* m_terrainProxy{};

		e2::MaterialPtr m_waterMaterial;
		e2::WaterProxy* m_waterProxy{};

		e2::MaterialPtr m_fogMaterial;
		e2::FogProxy* m_fogProxy{};

		e2::MeshPtr m_waterChunk;
		e2::MeshPtr m_fogChunk;

		e2::Texture2DPtr m_waterTexture;
		e2::Texture2DPtr m_forestTexture;
		e2::Texture2DPtr m_mountainTexture;


		std::vector<glm::ivec2> m_outlineTiles[size_t(e2::OutlineLayer::Count)];


		struct FrameData
		{
			// two masks and targets per frame ,since we want to use these for blur too 
			e2::IRenderTarget* fogOfWarTargets[2] = { nullptr, nullptr };
			e2::ITexture* fogOfWarMasks[2] = { nullptr, nullptr };
			e2::IDescriptorSet* fogOfWarMaskBlurSets[2] = { nullptr, nullptr };

			e2::IRenderTarget* outlineTarget{  };
			e2::ITexture* outlineTexture{  };

			e2::IRenderTarget* minimapTarget{};
			e2::ITexture* minimapTexture{};
			e2::IDescriptorSet* minimapSet{};

			e2::IRenderTarget* mapUnitsTarget{};
			e2::ITexture* mapUnitsTexture{};

			// two per frame since blur yo 
			e2::IRenderTarget* mapVisTargets[2] = {nullptr, nullptr};
			e2::ITexture* mapVisTextures[2] = {nullptr, nullptr};
			e2::IDescriptorSet* mapVisBlurSets[2] = { nullptr, nullptr };

		};

		FrameData m_frameData[2];

		e2::IShader* m_blurFragmentShader{};
		e2::IPipelineLayout* m_blurPipelineLayout{};
		e2::IPipeline* m_blurPipeline{};
		e2::IDescriptorSetLayout* m_blurSetLayout{};
		e2::IDescriptorPool* m_blurPool{};


		glm::uvec2 m_fogOfWarMaskSize{};
		e2::IShader* m_fogOfWarVertexShader{};
		e2::IShader* m_fogOfWarFragmentShader{};
		e2::IPipelineLayout* m_fogOfWarPipelineLayout{};
		e2::IPipeline* m_fogOfWarPipeline{};


		e2::IShader* m_outlineVertexShader{};
		e2::IShader* m_outlineFragmentShader{};
		e2::IPipelineLayout* m_outlinePipelineLayout{};
		e2::IPipeline* m_outlinePipeline{};


	public:
		glm::uvec2 const& minimapSize()
		{
			return m_minimapSize;
		}
	protected:
		glm::uvec2 m_minimapSize{};

		e2::IShader* m_minimapFragmentShader{};
		e2::IPipelineLayout* m_minimapPipelineLayout{};
		e2::IPipeline* m_minimapPipeline{};

		e2::IDescriptorSetLayout* m_minimapLayout{};
		e2::IDescriptorPool* m_minimapPool{};



	public:
		void initializeWorldBounds(glm::vec2 const& center);

		void updateWorldBounds();

		e2::Aabb2D const& viewBounds()
		{
			return m_minimapViewBounds;
		}

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
	void updateViewBounds();

	bool m_debugDraw{ false };
	public:
		void debugDraw(bool newValue);
	};

}





#include "hex.generated.hpp"
