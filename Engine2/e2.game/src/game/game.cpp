
#include "game/game.hpp"

#include "game/wave.hpp"

#include "e2/e2.hpp"
#include "e2/managers/rendermanager.hpp"
#include "e2/managers/asyncmanager.hpp"
#include "e2/managers/typemanager.hpp"
#include "e2/managers/assetmanager.hpp"
#include "e2/managers/audiomanager.hpp"
#include "e2/managers/uimanager.hpp"
#include "e2/game/gamesession.hpp"
#include "e2/renderer/renderer.hpp"
#include "e2/transform.hpp"



#include <glm/gtx/intersect.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <glm/gtc/noise.hpp>
#include <glm/gtx/spline.hpp>
#include <glm/gtx/easing.hpp>

#include <fmod_errors.h>

#pragma warning(disable : 4996)

#include <filesystem>
#include <Shlobj.h>
#include <ctime>

#include <random>
#include <algorithm>

namespace
{
	std::vector<e2::Hex> tmpHex;
}


e2::Game::Game(e2::Context* ctx)
	: e2::Application(ctx)
{
	m_empires.resize(e2::maxNumEmpires);
	for (uint64_t i = 0; uint64_t(i) < e2::maxNumEmpires; i++)
	{
		m_empires[i] = nullptr;
	}

	m_hitLabels.resize(e2::maxNumHitLabels);
	for (uint64_t i = 0; i < maxNumHitLabels; i++)
	{
		m_hitLabels[i] = HitLabel();
	}

	readAllSaveMetas();

	std::string namesRaw;
	if (!e2::readFile("./data/citynames.txt", namesRaw))
	{
		LogError("failed to read city names");
	}

	std::vector<std::string> namesStr = e2::split(namesRaw, '\n');
	for (std::string str : namesStr)
	{
		str = e2::trim(str);
		m_cityNames.push_back(str);
	}

	auto rd = std::random_device{};
	auto rng = std::default_random_engine{ rd() };
	std::shuffle(m_cityNames.begin(), m_cityNames.end(), rng);
}

e2::Game::~Game()
{
	
}

void e2::Game::saveGame(uint8_t slot)
{
	//if (m_empireTurn != m_localEmpireId)
	//{
	//	LogError("refuse to save game on AI turn");
	//	return;
	//}

	e2::SaveMeta newMeta;
	newMeta.exists = true;
	newMeta.timestamp = std::time(nullptr);
	newMeta.slot = slot;

	e2::FileStream buf(newMeta.fileName(), true);

	buf << int64_t(newMeta.timestamp);

	m_hexGrid->saveToBuffer(buf);

	buf << m_startViewOrigin;
	buf << m_viewOrigin;
	buf << m_turn;

	m_localEmpire->write(buf);
	m_nomadEmpire->write(buf);

	buf << uint64_t(m_entities.size());
	for (e2::GameEntity* entity : m_entities)
	{
		// structure id
		buf << entity->specification->id;

		// structure empire id 
		buf << int64_t(entity->empireId);

		// structure tileIndex 
		buf << entity->tileIndex;

		entity->writeForSave(buf);
	}

	saveSlots[slot] = newMeta;

}

e2::SaveMeta e2::Game::readSaveMeta(uint8_t slot)
{
	e2::SaveMeta meta;
	meta.slot = slot;
	meta.cachedFileName = meta.fileName();

	// header is just int64_t timestamp
	constexpr uint64_t headerSize = 8 ;

	e2::FileStream buf(meta.cachedFileName, false);
	if (!buf.valid())
	{
		meta.cachedDisplayName = meta.displayName();
		saveSlots[slot] = meta;
		return meta;
	}

	meta.exists = true;
	int64_t time{};
	buf >> time;
	meta.timestamp = time;

	meta.cachedDisplayName = meta.displayName();


	saveSlots[slot] = meta;

	

	return meta;

}

void e2::Game::readAllSaveMetas()
{
	for (uint8_t slot = 0; slot < e2::numSaveSlots; slot++)
		readSaveMeta(slot);
}

void e2::Game::loadGame(uint8_t slot)
{
	// header is just uint64_t discoveredTiles + int64_t timestamp
	constexpr uint64_t headerSize = 8 ;

	e2::FileStream buf(saveSlots[slot].fileName(), false);
	if (!buf.valid())
	{
		LogError("save slot missing or corrupted"); 
		return;
	}

	nukeGame();
	setupGame();

	buf.read(sizeof(int64_t));

	m_hexGrid->loadFromBuffer(buf);

	buf >> m_startViewOrigin;
	buf >> m_viewOrigin;
	buf >> m_turn;



	m_localEmpireId = spawnEmpire();
	assert(m_localEmpireId == 0 && "local empire HAS to be 0!");

	m_localEmpire = m_empires[m_localEmpireId];
	//discoverEmpire(m_localEmpireId);
	m_localEmpire->read(buf);

	m_nomadEmpireId = spawnEmpire();
	assert(m_nomadEmpireId == 1 && "nomad empire HAS to be 1!");

	m_nomadEmpire = m_empires[m_nomadEmpireId];
	//discoverEmpire(m_nomadEmpireId);
	m_nomadEmpire->read(buf);

	m_nomadEmpire->ai = e2::create<e2::NomadAI>(this, m_nomadEmpireId);


	uint64_t numEntities{};
	buf >> numEntities;
	for (uint64_t i = 0; i < numEntities; i++)
	{
		e2::Name entityId;
		buf >> entityId;

		int64_t empireId{};
		buf >> empireId;

		glm::ivec2 tileIndex;
		buf >> tileIndex;

		e2::GameEntity* newEntity = spawnEntity(entityId, tileIndex, EmpireId(empireId));
		newEntity->readForSave(buf);
	}

	//uint64_t numDiscoveredEmpires{};
	//buf >> numDiscoveredEmpires;
	//for (uint64_t i = 0; i < numDiscoveredEmpires;i++)
	//{
	//	int64_t empId;
	//	buf >> empId;

	//	if (empId > (std::numeric_limits<EmpireId>::max)())
	//	{
	//		LogError("unsupported datatype in savefile, try updating your game before loading this save!");
	//		continue;
	//	}


	//	discoverEmpire((EmpireId)empId);
	//}

	startGame();
}

void e2::Game::setupGame()
{
	m_hexGrid = e2::create<e2::HexGrid>(this);

	m_startViewOrigin = glm::vec2(528.97f, 587.02f);
	m_viewOrigin = m_startViewOrigin;
	m_hexGrid->initializeWorldBounds(m_viewOrigin);
}

void e2::Game::nukeGame()
{
	e2::ITexture* outlineTextures[2] = {nullptr, nullptr};
	m_session->renderer()->setOutlineTextures(outlineTextures);

	deselectEntity();

	m_entitiesPendingDestroy.clear();


	auto entitiesClone = m_entities;
	for (GameEntity* entity: entitiesClone)
		destroyEntity(entity);

	for (EmpireId i = 0; i < e2::maxNumEmpires-1; i++)
		destroyEmpire(i);

	if (m_hexGrid)
	{
		asyncManager()->waitForPredicate([this]()->bool {
			return m_hexGrid->numJobsInFlight() == 0;
		});
		e2::destroy(m_hexGrid);
		m_hexGrid = nullptr;
	}



	m_globalState = GlobalState::Menu;
	m_inGameMenuState = InGameMenuState::Main;
	m_state = GameState::TurnPreparing;
	m_turn = 0;
	//m_empireTurn = 0;
	m_turnState = TurnState::Unlocked;
	m_timeDelta = 0.0;
	m_localEmpireId = 0;
	m_localEmpire = nullptr;
}

void e2::Game::exitToMenu()
{
	nukeGame();
	setupGame();
}

void e2::Game::finalizeBoot()
{
	// This will fetch and store the loaded assets from all entity specifications 
	e2::EntitySpecification::finalizeSpecifications(this);
	e2::MobSpecification::finalizeSpecifications(this);

	auto am = assetManager();

	m_uiTextureResources = am->get("assets/UI_ResourceIcons.e2a").cast<e2::Texture2D>();

	m_irradianceMap = am->get("assets/hdri/courtyard_irr.e2a").cast<e2::Texture2D>();
	m_radianceMap = am->get("assets/hdri/courtyard_rad.e2a").cast<e2::Texture2D>();

	m_session->renderer()->setEnvironment(m_irradianceMap->handle(), m_radianceMap->handle());

	m_testSound = am->get("assets/sound/city_damage.e2a").cast<e2::Sound>();

	m_uiIconsSheet = am->get("assets/ui/S_UI_Icons.e2a").cast<e2::Spritesheet>();

	m_uiIconsSheet2 = am->get("assets/ui/S_Icons.e2a").cast<e2::Spritesheet>();
	m_uiUnitsSheet = am->get("assets/ui/S_Units.e2a").cast<e2::Spritesheet>();

	uiManager()->registerGlobalSpritesheet("gameUi", m_uiIconsSheet);
	uiManager()->registerGlobalSpritesheet("icons", m_uiIconsSheet2);
	uiManager()->registerGlobalSpritesheet("units", m_uiUnitsSheet);


	// After this line, we may no longer safely fetch and store loaded assets
	am->returnALJ(m_bootTicket);



	setupGame();

#if defined(E2_PROFILER)
	profiler()->start();
#endif
}

void e2::Game::initializeScriptEngine()
{
	try {


		m_scriptModule = chaiscript::ModulePtr(e2::create<chaiscript::Module>());

		/*

		TileData& getTileFromIndex(size_t index);
		size_t discover(Hex hex);
		size_t getTileIndexFromHex(Hex hex);
		e2::TileData* getTileData(glm::ivec2 const& hex);
		*/

		chaiscript::utility::add_class<e2::Name>(*m_scriptModule,
			"Name",
			{
				chaiscript::constructor<e2::Name(std::string const&)>(),
			},
			{
				{chaiscript::fun(&e2::Name::string), "string"},
			}
			);

		chaiscript::utility::add_class<glm::ivec2>(*m_scriptModule,
			"ivec2",
			{
				chaiscript::constructor<glm::ivec2(glm::ivec2 const&)>(),
				chaiscript::constructor<glm::ivec2()>(),
				chaiscript::constructor<glm::ivec2(int32_t, int32_t)>()
			},
			{
				{chaiscript::fun(&glm::ivec2::x), "x"},
				{chaiscript::fun(&glm::ivec2::y), "y"},
				{chaiscript::fun( (glm::ivec2 & (glm::ivec2::*)(const glm::ivec2&)) (&glm::ivec2::operator=) ), "="},
			}
			);
		

		chaiscript::utility::add_class<e2::Hex>(*m_scriptModule,
			"Hex",
			{
				chaiscript::constructor<e2::Hex()>(),
				chaiscript::constructor<e2::Hex(e2::Hex const&)>(),
				chaiscript::constructor<e2::Hex(glm::ivec2 const&)>(),
				chaiscript::constructor<e2::Hex(glm::vec2 const&)>()
			},
			{
				{chaiscript::fun(&e2::Hex::neighbours), "neighbours"},
				{chaiscript::fun(&e2::Hex::planarCoords), "planarCoords"},
				{chaiscript::fun(&e2::Hex::offsetCoords), "offsetCoords"},
				{chaiscript::fun(&e2::Hex::localCoords), "localCoords"},
				{chaiscript::fun(&e2::Hex::x), "x"},
				{chaiscript::fun(&e2::Hex::y), "y"},
				{chaiscript::fun(&e2::Hex::z), "z"},
			}
		);

		m_scriptModule->add(chaiscript::fun<int32_t (*)(e2::Hex const&, e2::Hex const&)>(&e2::Hex::distance), "hexDistance" );

		/*
		e2::StackVector<Hex, 6> 
		*/


		chaiscript::utility::add_class<e2::StackVector<Hex, 6>>(*m_scriptModule,
			"HexNeighbourList",
			{ },
			{
				{chaiscript::fun(&e2::StackVector<Hex, 6>::size), "size"},
				{chaiscript::fun(&e2::StackVector<Hex, 6>::at), "at"},
			}
		);

		chaiscript::utility::add_class<glm::vec2>(*m_scriptModule,
			"vec2",
			{
				chaiscript::constructor<glm::vec2()>(),
				chaiscript::constructor<glm::vec2(float, float)>()
			},
			{
				{chaiscript::fun(&glm::vec2::x), "x"},
				{chaiscript::fun(&glm::vec2::y), "y"},
			}
		);

		chaiscript::utility::add_class<glm::vec3>(*m_scriptModule,
			"vec3",
			{
				chaiscript::constructor<glm::vec3()>(),
				chaiscript::constructor<glm::vec3(float, float, float)>()
			},
			{
				{chaiscript::fun(&glm::vec3::x), "x"},
				{chaiscript::fun(&glm::vec3::y), "y"},
				{chaiscript::fun(&glm::vec3::z), "z"},
			}
			);

		chaiscript::utility::add_class<glm::vec4>(*m_scriptModule,
			"vec4",
			{
				chaiscript::constructor<glm::vec4()>(),
				chaiscript::constructor<glm::vec4(float, float, float, float)>()
			},
			{
				{chaiscript::fun(&glm::vec4::x), "x"},
				{chaiscript::fun(&glm::vec4::y), "y"},
				{chaiscript::fun(&glm::vec4::z), "z"},
				{chaiscript::fun(&glm::vec4::w), "w"},
			}
			);

		chaiscript::utility::add_class<e2::GameContext>(*m_scriptModule,
			"GameContext",
			{ },
		{
			{chaiscript::fun(&GameContext::game), "game"},
			{chaiscript::fun(&GameContext::hexGrid), "hexGrid"},
			{chaiscript::fun(&GameContext::gameSession), "gameSession"},
		}
		);

		chaiscript::utility::add_class<e2::UIColor>(*m_scriptModule,
			"Color",
			{
				chaiscript::constructor<UIColor(glm::vec4 const&)>()
			},
			{
				{chaiscript::fun(&UIColor::toVec4), "toVec4"},
			}
		);

		chaiscript::utility::add_class<e2::TileData>(*m_scriptModule,
			"TileData",
			{ },
		{
			{chaiscript::fun(&e2::TileData::empireId), "empireId"},
			{chaiscript::fun(&e2::TileData::getAbundanceAsFloat), "getAbundanceAsFloat"},
			{chaiscript::fun(&e2::TileData::getWoodAbundanceAsFloat), "getWoodAbundanceAsFloat"},
			{chaiscript::fun(&e2::TileData::isForested), "isForested"},
			{chaiscript::fun(&e2::TileData::hasResource), "hasResource"},
			{chaiscript::fun(&e2::TileData::isPassable), "isPassable"},

			{chaiscript::fun(&e2::TileData::hasGold), "hasGold"},
			{chaiscript::fun(&e2::TileData::hasOil), "hasOil"},
			{chaiscript::fun(&e2::TileData::hasOre), "hasOre"},
			{chaiscript::fun(&e2::TileData::hasStone), "hasStone"},
			{chaiscript::fun(&e2::TileData::hasUranium), "hasUranium"},

			{chaiscript::fun(&e2::TileData::isShallowWater), "isShallowWater"},
			{chaiscript::fun(&e2::TileData::isDeepWater), "isDeepWater"},
			{chaiscript::fun(&e2::TileData::isLand), "isLand"},
			{chaiscript::fun(&e2::TileData::getWater), "getWater"},
			{chaiscript::fun(&e2::TileData::getFeature), "getFeature"},
			{chaiscript::fun(&e2::TileData::getBiome), "getBiome"},
			{chaiscript::fun(&e2::TileData::getResource), "getResource"},
			{chaiscript::fun(&e2::TileData::getAbundance), "getAbundance"},
			{chaiscript::fun(&e2::TileData::getWoodAbundance), "getWoodAbundance"},

		}
		);

		chaiscript::utility::add_class<e2::HexGrid>(*m_scriptModule,
			"HexGrid",
			{ },
		{
			{chaiscript::fun(&HexGrid::viewBounds), "viewBounds"},
			{chaiscript::fun(&HexGrid::worldBounds), "worldBounds"},
			{chaiscript::fun(&HexGrid::isVisible), "isVisible"},
			{chaiscript::fun(&HexGrid::clearOutline), "clearOutline"},
			{chaiscript::fun(&HexGrid::pushOutline), "pushOutline"},
			{chaiscript::fun(&HexGrid::getTileData), "getTileData"},
			{chaiscript::fun(&HexGrid::getExistingTileData), "getExistingTileData"},
			{chaiscript::fun(&HexGrid::getCalculatedTileData), "getCalculatedTileData"},

		}
		);




		chaiscript::utility::add_class<e2::EntitySpecification>(*m_scriptModule,
			"EntitySpecification",
			{ },
		{
			{chaiscript::fun(&e2::EntitySpecification::attackPoints), "attackPoints"},
			{chaiscript::fun(&e2::EntitySpecification::attackStrength), "attackStrength"},
			{chaiscript::fun(&e2::EntitySpecification::buildPoints), "buildPoints"},
			{chaiscript::fun(&e2::EntitySpecification::buildTurns), "buildTurns"},
			{chaiscript::fun(&e2::EntitySpecification::defensiveModifier), "defensiveModifier"},
			{chaiscript::fun(&e2::EntitySpecification::displayName), "displayName"},
			{chaiscript::fun(&e2::EntitySpecification::id), "id"},
			{chaiscript::fun(&e2::EntitySpecification::layerIndex), "layerIndex"},
			{chaiscript::fun(&e2::EntitySpecification::maxHealth), "maxHealth"},
			{chaiscript::fun(&e2::EntitySpecification::meshHeightOffset), "meshHeightOffset"},
			{chaiscript::fun(&e2::EntitySpecification::meshScale), "meshScale"},
			{chaiscript::fun(&e2::EntitySpecification::movePoints), "movePoints"},
			{chaiscript::fun(&e2::EntitySpecification::moveSpeed), "moveSpeed"},
			{chaiscript::fun(&e2::EntitySpecification::moveType), "moveType"},
			{chaiscript::fun(&e2::EntitySpecification::retaliatoryModifier), "retaliatoryModifier"},
			{chaiscript::fun(&e2::EntitySpecification::sightRange), "sightRange"},
			{chaiscript::fun(&e2::EntitySpecification::attackRange), "attackRange"},
		}
		);

		chaiscript::utility::add_class<e2::UnitBuildResult>(*m_scriptModule,
			"UnitBuildResult",
			{ },
		{
			{chaiscript::fun(&e2::UnitBuildResult::buildMessage), "buildMessage"},
			{chaiscript::fun(&e2::UnitBuildResult::didSpawn), "didSpawn"},
			{chaiscript::fun(&e2::UnitBuildResult::spawnLocation), "spawnLocation"}
		}
		);

		chaiscript::utility::add_class<e2::UnitBuildAction>(*m_scriptModule,
			"UnitBuildAction",
			{ },
		{
			{chaiscript::fun(&e2::UnitBuildAction::buildTurnsLeft), "buildTurnsLeft"},
			{chaiscript::fun(&e2::UnitBuildAction::totalBuilt), "totalBuilt"},
			{chaiscript::fun(&e2::UnitBuildAction::turnLastBuilt), "turnLastBuilt"},
			{chaiscript::fun(&e2::UnitBuildAction::tick), "tick"},
		}
		);

		chaiscript::utility::add_class<e2::PathFindingHex>(*m_scriptModule,
			"PathFindingHex",
			{
			},
			{
				{chaiscript::fun(&e2::PathFindingHex::index), "index"},
				{chaiscript::fun(&e2::PathFindingHex::isBegin), "isBegin"},
				{chaiscript::fun(&e2::PathFindingHex::hexHasTarget), "hexHasTarget"},
				{chaiscript::fun(&e2::PathFindingHex::grugTarget), "grugTarget"},
				{chaiscript::fun(&e2::PathFindingHex::stepsFromOrigin), "stepsFromOrigin"},
				{chaiscript::fun(&e2::PathFindingHex::towardsOrigin), "towardsOrigin"},
			}
			);

		chaiscript::utility::add_class<e2::PathFindingAS>(*m_scriptModule,
			"PathFindingAS",
			{
				{ chaiscript::constructor<e2::PathFindingAS(e2::GameEntity*)>() },
				{ chaiscript::constructor<e2::PathFindingAS(e2::Game*, e2::Hex const&, uint64_t, bool, e2::PassableFlags)>() },
				
				// e2::Game* game, e2::Hex const& start, uint64_t range, bool ignoreVisibility = false, e2::PassableFlags passableFlags = PassableFlags::Land
			},
			{
				{chaiscript::fun(&e2::PathFindingAS::find), "find"},
				{ chaiscript::fun(&e2::PathFindingAS::grugTarget), "grugTarget" },
				{ chaiscript::fun(&e2::PathFindingAS::grugCanMove), "grugCanMove" },
			}
		);

		chaiscript::utility::add_class<e2::GameEntity>(*m_scriptModule,
			"Entity",
			{ },
		{

			//{chaiscript::fun(&e2::GameEntity::scriptEqualityPtr), "=="},
			//{chaiscript::fun(&e2::GameEntity::scriptAssignPtr), "="},
			//{chaiscript::fun((e2::GameEntity *& (e2::GameEntity::*)(const e2::GameEntity*&)) (&e2::GameEntity::operator=)), "="},
			{chaiscript::fun(&e2::GameEntity::isBuilding), "isBuilding"},
			{chaiscript::fun(&e2::GameEntity::canBuild), "canBuild"},
			{chaiscript::fun(&e2::GameEntity::build), "build"},
			{chaiscript::fun(&e2::GameEntity::cancelBuild), "cancelBuild"},
			{chaiscript::fun(&e2::GameEntity::buildMessage), "buildMessage"},
			{chaiscript::fun(&e2::GameEntity::currentlyBuilding), "currentlyBuilding"},
			{chaiscript::fun(&e2::GameEntity::scriptState), "scriptState"},
			{chaiscript::fun(&e2::GameEntity::attackPointsLeft), "attackPointsLeft"},
			{chaiscript::fun(&e2::GameEntity::buildPointsLeft), "buildPointsLeft"},
			{chaiscript::fun(&e2::GameEntity::movePointsLeft), "movePointsLeft"},
			{chaiscript::fun(&e2::GameEntity::empireId), "empireId"},
			{chaiscript::fun(&e2::GameEntity::health), "health"},
			{chaiscript::fun(&e2::GameEntity::tileIndex), "tileIndex"},
			{chaiscript::fun(&e2::GameEntity::inView), "inView"},
			{chaiscript::fun(&e2::GameEntity::meshPosition), "meshPosition"},
			{chaiscript::fun(&e2::GameEntity::meshRotation), "meshRotation"},
			{chaiscript::fun(&e2::GameEntity::meshTargetRotation), "meshTargetRotation"},
			{chaiscript::fun(&e2::GameEntity::specification), "specification"},
			{chaiscript::fun(&e2::GameEntity::isLocal), "isLocal"},
			{chaiscript::fun(&e2::GameEntity::createBuildAction), "createBuildAction"},
			{chaiscript::fun(&e2::GameEntity::meshPlanarCoords), "meshPlanarCoords"},
			{chaiscript::fun(&e2::GameEntity::playAction), "playAction"},
			{chaiscript::fun(&e2::GameEntity::stopAction), "stopAction"},
			{chaiscript::fun(&e2::GameEntity::isActionPlaying), "isActionPlaying"},
			{chaiscript::fun(&e2::GameEntity::isAnyActionPlaying), "isAnyActionPlaying"},
			{chaiscript::fun(&e2::GameEntity::setPose), "setPose"},
			{chaiscript::fun(&e2::GameEntity::turnTowards), "turnTowards"},
			{chaiscript::fun(&e2::GameEntity::turnTowardsPlanar), "turnTowardsPlanar"},
			{chaiscript::fun(&e2::GameEntity::grugCanAttack), "grugCanAttack"},
			{chaiscript::fun(&e2::GameEntity::grugCanMove), "grugCanMove"},
			{chaiscript::fun(&e2::GameEntity::playSound), "playSound"},
			{chaiscript::fun(&e2::GameEntity::createMesh), "createMesh"},
			{chaiscript::fun(&e2::GameEntity::destroyMesh), "destroyMesh"},
			{chaiscript::fun(&e2::GameEntity::closestMobWithinRange), "closestMobWithinRange"},
			
		}
		);



		chaiscript::utility::add_class<e2::MobSpecification>(*m_scriptModule,
			"MobSpecification",
			{ },
		{

			{chaiscript::fun(&e2::MobSpecification::attackStrength), "attackStrength"},
			{chaiscript::fun(&e2::MobSpecification::defensiveStrength), "defensiveStrength"},
			{chaiscript::fun(&e2::MobSpecification::displayName), "displayName"},
			{chaiscript::fun(&e2::MobSpecification::id), "id"},
			{chaiscript::fun(&e2::MobSpecification::layerIndex), "layerIndex"},
			{chaiscript::fun(&e2::MobSpecification::maxHealth), "maxHealth"},
			{chaiscript::fun(&e2::MobSpecification::moveSpeed), "moveSpeed"},
			{chaiscript::fun(&e2::MobSpecification::moveType), "moveType"},
			{chaiscript::fun(&e2::MobSpecification::passableFlags), "passableFlags"},
			{chaiscript::fun(&e2::MobSpecification::reward), "reward"},
		}
		);

		chaiscript::utility::add_class<e2::Mob>(*m_scriptModule,
			"Mob",
			{ },
		{
			//{chaiscript::fun(&e2::Mob::scriptEqualityPtr), "=="},
			//{chaiscript::fun(&e2::Mob::scriptAssignPtr), "="},
			{chaiscript::fun(&e2::Mob::health), "health"},
			{chaiscript::fun(&e2::Mob::specification), "specification"},
			{chaiscript::fun(&e2::Mob::meshPosition), "meshPosition"},
		}
		);

		chaiscript::utility::add_class<e2::Game>(*m_scriptModule,
			"Game",
			{ },
			{
				{chaiscript::fun(&Game::killEntity), "killEntity"},
				{chaiscript::fun(&Game::spawnHitLabel), "spawnHitLabel"},
				//{chaiscript::fun(&Game::discoverEmpire), "discoverEmpire"},
				{chaiscript::fun(&Game::turn), "turn"},
				{chaiscript::fun(&Game::timeDelta), "timeDelta"},
				{chaiscript::fun(&Game::worldToPixels), "worldToPixels"},
				{chaiscript::fun(&Game::pauseWorldStreaming), "pauseWorldStreaming"},
				{chaiscript::fun(&Game::resumeWorldStreaming), "resumeWorldStreaming"},
				{chaiscript::fun(&Game::forceStreamLocation), "forceStreamLocation"},
				{chaiscript::fun(&Game::loadGame), "loadGame"},
				{chaiscript::fun(&Game::saveGame), "saveGame"},
				{chaiscript::fun(&Game::exitToMenu), "exitToMenu"},
				{chaiscript::fun(&Game::spawnEmpire), "spawnEmpire"},
				//{chaiscript::fun(&Game::spawnAIEmpire), "spawnAIEmpire"},
				{chaiscript::fun(&Game::localEmpire), "localEmpire"},
				{chaiscript::fun(&Game::nomadEmpire), "nomadEmpire"},
				{chaiscript::fun(&Game::localEmpireId), "localEmpireId"},
				{chaiscript::fun(&Game::nomadEmpireId), "nomadEmpireId"},
				{chaiscript::fun(&Game::empireById), "empireById"},
				{chaiscript::fun(&Game::destroyEmpire), "destroyEmpire"},
				{chaiscript::fun(&Game::harvestWood), "harvestWood"},
				{chaiscript::fun(&Game::getUiSprite), "getUiSprite"},
				{chaiscript::fun(&Game::removeWood), "removeWood"},
				{chaiscript::fun(&Game::view), "view"},
				{chaiscript::fun(&Game::viewPoints), "viewPoints"},
				{chaiscript::fun(&Game::applyDamage), "applyDamage"},
				{chaiscript::fun(&Game::selectEntity), "selectEntity"},
				{chaiscript::fun(&Game::deselectEntity), "deselectEntity"},
				{chaiscript::fun(&Game::moveSelectedEntityTo), "moveSelectedEntityTo"},
				{chaiscript::fun(&Game::beginCustomAction), "beginCustomAction"},
				{chaiscript::fun(&Game::attemptBeginWave), "attemptBeginWave"},
				{chaiscript::fun(&Game::addScreenShake), "addScreenShake"},
				
				{chaiscript::fun(&Game::endCustomAction), "endCustomAction"},
				{chaiscript::fun(&Game::beginTargeting), "beginTargeting"},
				{chaiscript::fun(&Game::endTargeting), "endTargeting"},
				{chaiscript::fun(&Game::spawnEntity), "spawnEntity"},
				{chaiscript::fun(&Game::queueDestroyEntity), "destroyEntity"},
				{chaiscript::fun(&Game::entityAtHex), "entityAtHex"},
				{chaiscript::fun(&Game::getSelectedEntity), "getSelectedEntity"},
				{chaiscript::fun(&Game::getTurnState), "getTurnState"},
				{chaiscript::fun(&Game::grugNumAttackMovePoints), "grugNumAttackMovePoints"},
				{chaiscript::fun(&Game::grugAttackTarget), "grugAttackTarget"},
				{chaiscript::fun(&Game::grugAttackMoveLocation), "grugAttackMoveLocation"},
				{chaiscript::fun(&Game::grugMoveLocation), "grugMoveLocation"},
				{chaiscript::fun(&Game::resolveLocalEntity), "resolveLocalEntity"},
				{chaiscript::fun(&Game::getCityName), "getCityName"},
				/*
						int32_t grugNumAttackMovePoints();
		e2::GameEntity* grugAttackTarget();
		glm::ivec2 grugAttackMoveLocation();
		glm::ivec2 grugMoveLocation();
				*/
			}
		);

		m_scriptModule->add(chaiscript::base_class<GameContext, Game>());

		/*
		chaiscript::utility::add_class<e2::FontFace>(*m_scriptModule,
			"FontFace",
			{
				{e2::FontFace::Serif, "Serif"},
				{e2::FontFace::Sans, "Sans"},
				{e2::FontFace::Monospace, "Monospace"},
			}
		);*/

		/*chaiscript::utility::add_class<e2::UITextAlign>(*m_scriptModule,
			"UITextAlign",
			{
				{e2::UITextAlign::Begin, "Begin"},
				{e2::UITextAlign::End, "End"},
				{e2::UITextAlign::Middle, "Middle"},
			}
		);*/

		chaiscript::utility::add_class<e2::UIContext>(*m_scriptModule,
			"UIContext",
			{ },
			{
				{chaiscript::fun(&UIContext::beginStackV), "beginStackV"},
				{chaiscript::fun(&UIContext::endStackV), "endStackV"},
				{chaiscript::fun(&UIContext::beginStackH), "beginStackH"},
				{chaiscript::fun(&UIContext::endStackH), "endStackH"},
				{chaiscript::fun(&UIContext::beginFlexH), "beginFlexH"},
				{chaiscript::fun(&UIContext::endFlexH), "endFlexH"},
				{chaiscript::fun(&UIContext::beginFlexV), "beginFlexV"},
				{chaiscript::fun(&UIContext::endFlexV), "endFlexV"},
				{chaiscript::fun(&UIContext::flexHSlider), "flexHSlider"},
				{chaiscript::fun(&UIContext::flexVSlider), "flexVSlider"},
				{chaiscript::fun(&UIContext::beginWrap), "beginWrap"},
				{chaiscript::fun(&UIContext::endWrap), "endWrap"},
				{chaiscript::fun(&UIContext::button), "button"},
				{chaiscript::fun(&UIContext::checkbox), "checkbox"},
				{chaiscript::fun(&UIContext::radio), "radio"},
				{chaiscript::fun(&UIContext::inputText), "inputText"},
				{chaiscript::fun(&UIContext::sliderInt), "sliderInt"},
				{chaiscript::fun(&UIContext::sliderFloat), "sliderFloat"},
				{chaiscript::fun(&UIContext::gameLabel), "gameLabel"},
				{chaiscript::fun(&UIContext::gameGridButton), "gameGridButton"},
				{chaiscript::fun(&UIContext::calculateSDFTextWidth), "calculateSDFTextWidth"},
				{chaiscript::fun(&UIContext::calculateTextWidth), "calculateTextWidth"},
				{chaiscript::fun(&UIContext::drawSDFText), "drawSDFText"},
				{chaiscript::fun(&UIContext::drawRasterText), "drawRasterText"},
				{chaiscript::fun(&UIContext::drawQuad), "drawQuad"},
				{chaiscript::fun(&UIContext::drawSprite), "drawSprite"},
				{chaiscript::fun(&UIContext::drawTexturedQuad), "drawTexturedQuad"},
			}
		);

		chaiscript::utility::add_class<e2::MeshProxy>(*m_scriptModule,
			"MeshProxy",
			{ },
			{
				{chaiscript::fun(&MeshProxy::enable), "enable"},
				{chaiscript::fun(&MeshProxy::disable), "disable"},
				{chaiscript::fun(&MeshProxy::enabled), "enabled"},
				{chaiscript::fun(&MeshProxy::setPosition), "setPosition"},
				{chaiscript::fun(&MeshProxy::setRotation), "setRotation"},
				{chaiscript::fun(&MeshProxy::setScale), "setScale"},

			}
			);

		chaiscript::utility::add_class<e2::EntityScriptInterface>(*m_scriptModule,
			"EntityScript",
			{
				chaiscript::constructor<EntityScriptInterface()>()
			},
			{
				{chaiscript::fun(&EntityScriptInterface::hasOnActionTrigger), "hasOnActionTrigger"},
				{chaiscript::fun(&EntityScriptInterface::hasUpdate), "hasUpdate"},
				{chaiscript::fun(&EntityScriptInterface::hasCreateState), "hasCreateState"},
				{chaiscript::fun(&EntityScriptInterface::hasDrawUI), "hasDrawUI"},
				{chaiscript::fun(&EntityScriptInterface::hasCollectExpenditure), "hasCollectExpenditure"},
				{chaiscript::fun(&EntityScriptInterface::hasCollectRevenue), "hasCollectRevenue"},
				{chaiscript::fun(&EntityScriptInterface::hasGrugRelevant), "hasGrugRelevant"},
				{chaiscript::fun(&EntityScriptInterface::hasGrugTick), "hasGrugTick"},
				{chaiscript::fun(&EntityScriptInterface::hasOnBeginMove), "hasOnBeginMove"},
				{chaiscript::fun(&EntityScriptInterface::hasOnEndMove), "hasOnEndMove"},
				{chaiscript::fun(&EntityScriptInterface::hasOnHit), "hasOnHit"},
				{chaiscript::fun(&EntityScriptInterface::hasOnTargetChanged), "hasOnTargetChanged"},
				{chaiscript::fun(&EntityScriptInterface::hasOnTargetClicked), "hasOnTargetClicked"},
				{chaiscript::fun(&EntityScriptInterface::hasOnTurnEnd), "hasOnTurnEnd"},
				{chaiscript::fun(&EntityScriptInterface::hasOnTurnStart), "hasOnTurnStart"},
				{chaiscript::fun(&EntityScriptInterface::hasUpdateAnimation), "hasUpdateAnimation"},
				{chaiscript::fun(&EntityScriptInterface::hasUpdateCustomAction), "hasUpdateCustomAction"},
				{chaiscript::fun(&EntityScriptInterface::hasOnWaveUpdate), "hasOnWaveUpdate"},
				{chaiscript::fun(&EntityScriptInterface::hasOnWaveStart), "hasOnWaveStart"},
				{chaiscript::fun(&EntityScriptInterface::hasOnWaveEnd), "hasOnWaveEnd"},
				{chaiscript::fun(&EntityScriptInterface::hasOnSpawned), "hasOnSpawned"},

				{chaiscript::fun(&EntityScriptInterface::hasOnSelected), "hasOnSelected"},
				{chaiscript::fun(&EntityScriptInterface::hasOnDeselected), "hasOnDeselected"},

				{chaiscript::fun(&EntityScriptInterface::invokeOnActionTrigger), "invokeOnActionTrigger"},
				{chaiscript::fun(&EntityScriptInterface::invokeUpdate), "invokeUpdate"},
				{chaiscript::fun(&EntityScriptInterface::invokeCreateState), "invokeCreateState"},
				{chaiscript::fun(&EntityScriptInterface::invokeDrawUI), "invokeDrawUI"},
				{chaiscript::fun(&EntityScriptInterface::invokeCollectExpenditure), "invokeCollectExpenditure"},
				{chaiscript::fun(&EntityScriptInterface::invokeCollectRevenue), "invokeCollectRevenue"},
				{chaiscript::fun(&EntityScriptInterface::invokeGrugRelevant), "invokeGrugRelevant"},
				{chaiscript::fun(&EntityScriptInterface::invokeGrugTick), "invokeGrugTick"},
				{chaiscript::fun(&EntityScriptInterface::invokeOnBeginMove), "invokeOnBeginMove"},
				{chaiscript::fun(&EntityScriptInterface::invokeOnEndMove), "invokeOnEndMove"},
				{chaiscript::fun(&EntityScriptInterface::invokeOnHit), "invokeOnHit"},
				{chaiscript::fun(&EntityScriptInterface::invokeOnTargetChanged), "invokeOnTargetChanged"},
				{chaiscript::fun(&EntityScriptInterface::invokeOnTargetClicked), "invokeOnTargetClicked"},
				{chaiscript::fun(&EntityScriptInterface::invokeOnTurnEnd), "invokeOnTurnEnd"},
				{chaiscript::fun(&EntityScriptInterface::invokeOnTurnStart), "invokeOnTurnStart"},
				{chaiscript::fun(&EntityScriptInterface::invokeUpdateAnimation), "invokeUpdateAnimation"},
				{chaiscript::fun(&EntityScriptInterface::invokeUpdateCustomAction), "invokeUpdateCustomAction"},
				{chaiscript::fun(&EntityScriptInterface::invokeOnWaveUpdate), "invokeOnWaveUpdate"},
				{chaiscript::fun(&EntityScriptInterface::invokeOnWaveStart), "invokeOnWaveStart"},
				{chaiscript::fun(&EntityScriptInterface::invokeOnWaveEnd), "invokeOnWaveEnd"},
				{chaiscript::fun(&EntityScriptInterface::invokeOnSpawned), "invokeOnSpawned"},

				{chaiscript::fun(&EntityScriptInterface::setOnSelected), "setOnSelected"},
				{chaiscript::fun(&EntityScriptInterface::setOnDeselected), "setOnDeselected"},

				{chaiscript::fun(&EntityScriptInterface::setOnActionTrigger), "setOnActionTrigger"},
				{chaiscript::fun(&EntityScriptInterface::setUpdate), "setUpdate"},
				{chaiscript::fun(&EntityScriptInterface::setCreateState), "setCreateState"},
				{chaiscript::fun(&EntityScriptInterface::setDrawUI), "setDrawUI"},
				{chaiscript::fun(&EntityScriptInterface::setCollectExpenditure), "setCollectExpenditure"},
				{chaiscript::fun(&EntityScriptInterface::setCollectRevenue), "setCollectRevenue"},
				{chaiscript::fun(&EntityScriptInterface::setPlayerRelevant), "setPlayerRelevant"},
				{chaiscript::fun(&EntityScriptInterface::setGrugRelevant), "setGrugRelevant"},
				{chaiscript::fun(&EntityScriptInterface::setGrugTick), "setGrugTick"},
				{chaiscript::fun(&EntityScriptInterface::setOnBeginMove), "setOnBeginMove"},
				{chaiscript::fun(&EntityScriptInterface::setOnEndMove), "setOnEndMove"},
				{chaiscript::fun(&EntityScriptInterface::setOnHit), "setOnHit"},
				{chaiscript::fun(&EntityScriptInterface::setOnTargetChanged), "setOnTargetChanged"},
				{chaiscript::fun(&EntityScriptInterface::setOnTargetClicked), "setOnTargetClicked"},
				{chaiscript::fun(&EntityScriptInterface::setOnTurnEnd), "setOnTurnEnd"},
				{chaiscript::fun(&EntityScriptInterface::setOnTurnStart), "setOnTurnStart"},
				{chaiscript::fun(&EntityScriptInterface::setUpdateAnimation), "setUpdateAnimation"},
				{chaiscript::fun(&EntityScriptInterface::setUpdateCustomAction), "setUpdateCustomAction"},
				{chaiscript::fun(&EntityScriptInterface::setOnWaveUpdate), "setOnWaveUpdate"},
				{chaiscript::fun(&EntityScriptInterface::setOnWaveStart), "setOnWaveStart"},
				{chaiscript::fun(&EntityScriptInterface::setOnWaveEnd), "setOnWaveEnd"},
				{chaiscript::fun(&EntityScriptInterface::setOnSpawned), "setOnSpawned"},
				{chaiscript::fun(&EntityScriptInterface::setOnMobSpawned), "setOnMobSpawned"},
				{chaiscript::fun(&EntityScriptInterface::setOnMobDestroyed), "setOnMobDestroyed"},
				{chaiscript::fun(&EntityScriptInterface::hasOnMobSpawned), "hasOnMobSpawned"},
				{chaiscript::fun(&EntityScriptInterface::hasOnMobDestroyed), "hasOnMobDestroyed"},
				{chaiscript::fun(&EntityScriptInterface::invokeOnMobSpawned), "invokeOnMobSpawned"},
				{chaiscript::fun(&EntityScriptInterface::invokeOnMobDestroyed), "invokeOnMobDestroyed"},

				{chaiscript::fun(&EntityScriptInterface::invokeOnSelected), "invokeOnSelected"},
				{chaiscript::fun(&EntityScriptInterface::invokeOnDeselected), "invokeOnDeselected"},
			}
		);



		chaiscript::utility::add_class<e2::ScriptRef<e2::Mob>>(*m_scriptModule,
			"MobRef",
			{
				chaiscript::constructor<e2::ScriptRef<e2::Mob>()>(),
				chaiscript::constructor<e2::ScriptRef<e2::Mob>(e2::Mob*)>(),
				chaiscript::constructor<e2::ScriptRef<e2::Mob>(e2::ScriptRef<e2::Mob> const &)>(),
			},
			{
				{chaiscript::fun(&e2::ScriptRef<e2::Mob>::operator=), "="},
				{chaiscript::fun(&e2::ScriptRef<e2::Mob>::equals), "=="},
				{chaiscript::fun(&e2::ScriptRef<e2::Mob>::value), "value"},
				{chaiscript::fun(&e2::ScriptRef<e2::Mob>::clear), "clear"},
				{chaiscript::fun(&e2::ScriptRef<e2::Mob>::isNull), "isNull"},

			}
			);


		m_scriptModule->add(chaiscript::type_conversion<e2::Name, std::string>([](const e2::Name& asName) { return asName.cstring(); }));
		m_scriptModule->add(chaiscript::type_conversion<std::string, e2::Name>([](const std::string & asString) { return e2::Name(asString); }));


		m_scriptModule->add(chaiscript::type_conversion<e2::Hex, glm::ivec2>([](const e2::Hex& asHex) { return asHex.offsetCoords(); }));
		m_scriptModule->add(chaiscript::type_conversion<glm::ivec2, e2::Hex>([](const glm::ivec2& asIvec2) { return e2::Hex(asIvec2); }));


		chaiscript::utility::add_class<e2::EntityLayerIndex>(*m_scriptModule,
			"LayerIndex",
			{
				{e2::EntityLayerIndex::Unit, "LI_Unit"},
				{e2::EntityLayerIndex::Structure, "LI_Structure"},
				{e2::EntityLayerIndex::Air, "LI_Air"}
			}
		);

		chaiscript::utility::add_class<e2::EntityMoveType>(*m_scriptModule,
			"MoveType",
			{
				{e2::EntityMoveType::Static, "MT_Static"},
				{e2::EntityMoveType::Linear, "MT_Linear"},
				{e2::EntityMoveType::Smooth, "MT_Smooth"}
			}
		);

		chaiscript::utility::add_class<e2::TileFlags>(*m_scriptModule,
			"TileFlags",
			{
				{e2::TileFlags::None, "TF_None"},

				{e2::TileFlags::BiomeMask, "TF_BiomeMask"},
				{e2::TileFlags::BiomeGrassland, "TF_BiomeGrassland"},
				{e2::TileFlags::BiomeDesert, "TF_BiomeDesert"},
				{e2::TileFlags::BiomeTundra, "TF_BiomeTundra"},

				{e2::TileFlags::FeatureMask, "TF_FeatureMask"},
				{e2::TileFlags::FeatureNone, "TF_FeatureNone"},
				{e2::TileFlags::FeatureMountains, "TF_FeatureMountains"},
				{e2::TileFlags::FeatureForest, "TF_FeatureForest"},

				{e2::TileFlags::WaterMask, "TF_WaterMask"},
				{e2::TileFlags::WaterNone, "TF_WaterNone"},
				{e2::TileFlags::WaterShallow, "TF_WaterShallow"},
				{e2::TileFlags::WaterDeep, "TF_WaterDeep"},

				{e2::TileFlags::ResourceMask, "TF_ResourceMask"},
				{e2::TileFlags::ResourceNone, "TF_ResourceNone"},
				{e2::TileFlags::ResourceStone, "TF_ResourceStone"},
				{e2::TileFlags::ResourceOre, "TF_ResourceOre"},
				{e2::TileFlags::ResourceGold, "TF_ResourceGold"},
				{e2::TileFlags::ResourceOil, "TF_ResourceOil"},
				{e2::TileFlags::ResourceUranium, "TF_ResourceUranium"},

				{e2::TileFlags::AbundanceMask, "TF_AbundanceMask"},
				{e2::TileFlags::Abundance1, "TF_Abundance1"},
				{e2::TileFlags::Abundance2, "TF_Abundance2"},
				{e2::TileFlags::Abundance3, "TF_Abundance3"},
				{e2::TileFlags::Abundance4, "TF_Abundance4"},

				{e2::TileFlags::WoodAbundanceMask, "TF_WoodAbundanceMask"},
				{e2::TileFlags::WoodAbundance1, "TF_WoodAbundance1"},
				{e2::TileFlags::WoodAbundance2, "TF_WoodAbundance2"},
				{e2::TileFlags::WoodAbundance3, "TF_WoodAbundance3"},
				{e2::TileFlags::WoodAbundance4, "TF_WoodAbundance4"},
			}
		);

		chaiscript::utility::add_class<e2::PassableFlags>(*m_scriptModule,
			"PassableFlags",
			{
				{e2::PassableFlags::None, "PF_None"},
				{e2::PassableFlags::Land, "PF_Land"},
				{e2::PassableFlags::WaterShallow, "PF_WaterShallow"},
				{e2::PassableFlags::WaterDeep, "PF_WaterDeep"},
				{e2::PassableFlags::Mountain, "PF_Mountain"},
				{e2::PassableFlags::Air, "PF_Air"},
			}
		);

		chaiscript::utility::add_class<e2::FontFace>(*m_scriptModule,
			"FontFace",
			{
				{e2::FontFace::Sans, "FF_Sans"},
				{e2::FontFace::Serif, "FF_Serif"},
				{e2::FontFace::Monospace, "FF_Monospace"}
			}
		);


		chaiscript::utility::add_class<e2::UITextAlign>(*m_scriptModule,
			"UITextAlign",
			{
				{e2::UITextAlign::Begin, "TA_Begin"},
				{e2::UITextAlign::End, "TA_End"},
				{e2::UITextAlign::Middle, "TA_Middle"}
			}
		);

		chaiscript::utility::add_class<e2::TurnState>(*m_scriptModule,
			"TurnState",
			{
				{e2::TurnState::Unlocked, "TS_Unlocked"},
				//{e2::TurnState::Auto, "TS_Auto"},
				{e2::TurnState::UnitAction_Move, "TS_UnitAction_Move"},
				{e2::TurnState::EntityAction_Generic, "TS_EntityAction_Generic"},
				{e2::TurnState::EntityAction_Target, "TS_EntityAction_Target"},
			}
		);

		m_scriptEngine = e2::create<chaiscript::ChaiScript>();
		m_scriptEngine->add(m_scriptModule);

		

		m_scriptEngine->add_global(chaiscript::var(this), "game");
	}
	catch (std::exception& e)
	{
		LogError("failed to initialize chaiscript: {}", e.what());
	}
}

void e2::Game::destroyScriptEngine()
{
	e2::destroy(m_scriptEngine);
}

void e2::Game::initialize()
{
	m_session = e2::create<e2::GameSession>(this);

	initializeScriptEngine();

	try
	{
		m_scriptEngine->eval_file("data/library.chai");
	}
	catch (chaiscript::exception::eval_error& e)
	{
		LogError("chai: evaluation failed: {}", e.pretty_print());
	}
	catch (chaiscript::exception::bad_boxed_cast& e)
	{
		LogError("chai: casting return-type from script to native failed: {}", e.what());
	}
	catch (std::exception& e)
	{
		LogError("{}", e.what());
	}

	e2::EntitySpecification::initializeSpecifications(this);
	e2::MobSpecification::initializeSpecifications(this);

	m_bootBegin = e2::timeNow();

	auto am = assetManager();

	e2::ALJDescription alj;

	for (size_t i = 0; i < e2::EntitySpecification::specificationCount(); i++)
	{
		e2::EntitySpecification* spec = e2::EntitySpecification::specification(i);

		am->prescribeALJ(alj, spec->meshAssetPath);

		if(spec->skeletonAssetPath.size() > 0)
			am->prescribeALJ(alj, spec->skeletonAssetPath);

		for (auto& [name, action] : spec->actions)
		{
			am->prescribeALJ(alj, action.animationAssetPath);
		}

		for (auto& [name, pose] : spec->poses)
		{
			am->prescribeALJ(alj, pose.animationAssetPath);
		}

		for (auto& [name, asset] : spec->assets)
		{
			am->prescribeALJ(alj, asset.path);
		}
	}

	for (size_t i = 0; i < e2::MobSpecification::specificationCount(); i++)
	{
		e2::MobSpecification* spec = e2::MobSpecification::specification(i);

		am->prescribeALJ(alj, spec->meshAssetPath);

		if (spec->skeletonAssetPath.size() > 0)
			am->prescribeALJ(alj, spec->skeletonAssetPath);

		if (spec->runPoseAssetPath.size() > 0)
			am->prescribeALJ(alj, spec->runPoseAssetPath);


		if (spec->diePoseAssetPath.size() > 0)
			am->prescribeALJ(alj, spec->diePoseAssetPath);
	}

	am->prescribeALJ(alj, "assets/UI_ResourceIcons.e2a");
	am->prescribeALJ(alj, "assets/hdri/courtyard_irr.e2a");
	am->prescribeALJ(alj, "assets/hdri/courtyard_rad.e2a");

	am->prescribeALJ(alj, "assets/sound/city_damage.e2a");

	am->prescribeALJ(alj, "assets/ui/minimap_water.e2a");
	am->prescribeALJ(alj, "assets/ui/minimap_forest.e2a");
	am->prescribeALJ(alj, "assets/ui/minimap_mountain.e2a");

	am->prescribeALJ(alj, "assets/SM_HexBase.e2a");
	am->prescribeALJ(alj, "assets/SM_HexBaseHigh.e2a");

	/*am->prescribeALJ(alj, "assets/environment/trees/SM_PineForest001.e2a");
	am->prescribeALJ(alj, "assets/environment/trees/SM_PineForest002.e2a");
	am->prescribeALJ(alj, "assets/environment/trees/SM_PineForest003.e2a");
	am->prescribeALJ(alj, "assets/environment/trees/SM_PineForest004.e2a");*/

	am->prescribeALJ(alj, "assets/environment/SM_PineForest2_1.e2a");
	am->prescribeALJ(alj, "assets/environment/SM_PineForest2_2.e2a");
	am->prescribeALJ(alj, "assets/environment/SM_PineForest2_3.e2a");

	am->prescribeALJ(alj, "assets/ui/S_UI_Icons.e2a");

	am->prescribeALJ(alj, "assets/ui/S_Icons.e2a");
	am->prescribeALJ(alj, "assets/ui/S_Units.e2a");

	;
	am->prescribeALJ(alj, "assets/environment/resources/SM_ResourceMesh_Stone_LOD0.e2a");

	am->prescribeALJ(alj, "assets/environment/SM_PineTreeNew0.e2a");
	am->prescribeALJ(alj, "assets/environment/SM_PineTreeNew1.e2a");
	am->prescribeALJ(alj, "assets/environment/SM_PineTreeNew2.e2a");
	am->prescribeALJ(alj, "assets/environment/SM_PineTreeNew3.e2a");

	//am->prescribeALJ(alj, "assets/environment/SM_PineTree001_LOD0.e2a");
	//am->prescribeALJ(alj, "assets/environment/SM_PineTree001_LOD1.e2a");
	//am->prescribeALJ(alj, "assets/environment/SM_PineTree001_LOD2.e2a");

	//am->prescribeALJ(alj, "assets/environment/SM_PineTree002_LOD0.e2a");
	//am->prescribeALJ(alj, "assets/environment/SM_PineTree002_LOD1.e2a");
	//am->prescribeALJ(alj, "assets/environment/SM_PineTree002_LOD2.e2a");

	//am->prescribeALJ(alj, "assets/environment/SM_PineTree003_LOD0.e2a");
	//am->prescribeALJ(alj, "assets/environment/SM_PineTree003_LOD1.e2a");
	//am->prescribeALJ(alj, "assets/environment/SM_PineTree003_LOD2.e2a");

	//am->prescribeALJ(alj, "assets/environment/SM_PineTree004_LOD0.e2a");
	//am->prescribeALJ(alj, "assets/environment/SM_PineTree004_LOD1.e2a");
	//am->prescribeALJ(alj, "assets/environment/SM_PineTree004_LOD2.e2a");

	m_bootTicket = am->queueALJ(alj);
	




}

void e2::Game::shutdown()
{
	nukeGame();

	

	e2::EntitySpecification::destroySpecifications();
	e2::MobSpecification::destroySpecifications();

	destroyScriptEngine();

	e2::destroy(m_session);
	uiManager()->unregisterGlobalSpritesheet("gameUi");
	uiManager()->unregisterGlobalSpritesheet("icons");
	uiManager()->unregisterGlobalSpritesheet("units");


	m_irradianceMap = nullptr;
	m_radianceMap = nullptr;
	m_uiTextureResources = nullptr;
	m_uiIconsSheet = nullptr;
	m_uiIconsSheet2 = nullptr;
	m_uiUnitsSheet = nullptr;

}

void e2::Game::update(double seconds)
{

	e2::GameSession* session = gameSession();
	e2::Renderer* renderer = session->renderer();
	e2::UIContext* ui = session->uiContext();

	glm::quat sunRot = glm::identity<glm::quat>();

	sunRot = glm::rotate(sunRot, glm::radians(m_sunAngleA), e2::worldUpf());
	sunRot = glm::rotate(sunRot, glm::radians(m_sunAngleB), e2::worldRightf());

	renderer->setSun(sunRot, { 1.0f, 0.9f, 0.85f }, m_sunStrength);
	renderer->setIbl(m_iblStrength);
	renderer->whitepoint(glm::vec3(m_whitepoint));
	renderer->exposure(m_exposure);

	if (m_globalState == GlobalState::Boot)
	{
		m_session->tick(seconds);

		glm::vec2 winSize = m_session->window()->size();
		float textWidth = m_session->uiContext()->calculateSDFTextWidth(FontFace::Serif, 22.0f, "Loading... ");
		m_session->uiContext()->drawSDFTextCarousel(FontFace::Serif, 22.f, 0xFFFFFFFF, { winSize.x - textWidth - 16.f, winSize.y - 32.0f }, "Loading... ", 8.0f, (float)m_bootBegin.durationSince().seconds() * 4.0f);

#if defined(E2_SHIPPING)
		bool bootPredicate = assetManager()->queryALJ(m_bootTicket).status == ALJStatus::Completed && m_bootBegin.durationSince().seconds() > 4.0f;
#else 
		bool bootPredicate = assetManager()->queryALJ(m_bootTicket).status == ALJStatus::Completed;
#endif

		if (bootPredicate)
		{
			m_globalState = GlobalState::Menu;
			finalizeBoot();
		}
		return;
	}
	 
	auto& kb = ui->keyboardState();
	
	if (kb.pressed(Key::Enter) && kb.state(Key::LeftAlt))
	{
		session->window()->setFullscreen(!session->window()->isFullscreen());
	}

	m_timeDelta = seconds;
	if (m_globalState == GlobalState::Menu)
		updateMenu(seconds);
	else if (m_globalState == GlobalState::Game)
		updateGame(seconds);
	else if (m_globalState == GlobalState::InGameMenu)
		updateInGameMenu(seconds);

	auto view = renderer->view();

	audioManager()->setListenerTransform(glm::inverse(view.calculateViewMatrix()));

	std::unordered_set<e2::GameEntity*> cpy = m_dyingEntities;
	for (e2::GameEntity* entity : cpy)
	{
		if (!entity->isActionPlaying("die"))
		{
			m_dyingEntities.erase(entity);
			game()->queueDestroyEntity(entity);
		}
	}

	for (e2::GameEntity* entity : m_entitiesPendingDestroy)
		destroyEntity(entity);
	m_entitiesPendingDestroy.clear();
	
}

void e2::Game::updateInGameMenu(double seconds)
{
	constexpr float moveSpeed = 10.0f;
	constexpr float viewSpeed = .3f;

	e2::GameSession* session = gameSession();
	e2::Renderer* renderer = session->renderer();
	e2::UIContext* ui = session->uiContext();
	auto& kb = ui->keyboardState();
	auto& mouse = ui->mouseState();

	m_session->renderer()->setEnvironment(m_irradianceMap->handle(), m_radianceMap->handle());


	if (kb.pressed(Key::Escape))
	{
		if (m_inGameMenuState == InGameMenuState::Main)
			m_globalState = GlobalState::Game;
		else
			m_inGameMenuState = InGameMenuState::Main;
	}

	//m_hexGrid->assertChunksWithinRangeVisible(m_viewOrigin, m_viewPoints, m_viewVelocity);
	m_hexGrid->updateStreaming(m_viewOrigin, m_viewPoints, m_viewVelocity);
	m_hexGrid->updateWorldBounds();
	m_hexGrid->renderFogOfWar();
	e2::ITexture* outlineTextures[2] = { m_hexGrid->outlineTexture(0), m_hexGrid->outlineTexture(1) };
	m_session->renderer()->setOutlineTextures(outlineTextures);

	// ticking session renders renderer too, and blits it to the UI, so we need to do it precisely here (after rendering fog of war and before rendering UI)
	m_session->tick(seconds);

	glm::vec2 size(256.0f, 320.0f);
	glm::vec2 offset = (glm::vec2(ui->size()) / 2.0f) - (size / 2.0f);

	ui->pushFixedPanel("ingameMenuContainer", offset, size);

	if (m_inGameMenuState == InGameMenuState::Main)
	{


		ui->beginStackV("ingameMenu");

		if (ui->button("btnCont", "Continue"))
			m_globalState = GlobalState::Game;

		if (ui->button("btnSave", "Save.."))
			m_inGameMenuState = InGameMenuState::Save;

		if (ui->button("btnLoad", "Load.."))
			m_inGameMenuState = InGameMenuState::Load;

		if (ui->button("btnExitMenu", "Exit to Menu"))
			exitToMenu();

		if (ui->button("btnQuitDesktop", "Quit to Desktop"))
		{
			nukeGame();
			engine()->shutdown();
		}

		ui->endStackV();
	}
	else if (m_inGameMenuState == InGameMenuState::Save)
	{
		ui->beginStackV("saveMenu");

		for (uint8_t i = 0; i < e2::numSaveSlots; i++)
		{
			if (ui->button(std::format("save{}", uint32_t(i)), saveSlots[i].displayName()))
				saveGame(i);
		}


		ui->endStackV();
	}
	else if (m_inGameMenuState == InGameMenuState::Load)
	{
		ui->beginStackV("loadMenu");

		for (uint8_t i = 0; i < e2::numSaveSlots; i++)
		{
			if (ui->button(std::format("load{}", uint32_t(i)), saveSlots[i].displayName()))
				loadGame(i);
		}


		ui->endStackV();
	}
	else if (m_inGameMenuState == InGameMenuState::Options)
	{
	}

	ui->popFixedPanel();
}

void e2::Game::updateGame(double seconds)
{
	constexpr float moveSpeed = 10.0f;
	constexpr float viewSpeed = .3f;

	e2::GameSession* session = gameSession();
	e2::Renderer* renderer = session->renderer();
	e2::UIContext* ui = session->uiContext();
	auto& kb = ui->keyboardState();
	auto& mouse = ui->mouseState();

	m_session->renderer()->setEnvironment(m_irradianceMap->handle(), m_radianceMap->handle());



	// update input state
	glm::vec2 resolution = renderer->resolution();
	m_cursor = mouse.relativePosition;
	m_cursorUnit = (m_cursor / resolution);
	m_cursorNdc = m_cursorUnit * 2.0f - 1.0f;
	m_cursorPlane = renderer->view().unprojectWorldPlane(renderer->resolution(), m_cursorNdc, 0.0);
	e2::Hex newHex = e2::Hex(m_cursorPlane);
	m_hexChanged = newHex != m_prevCursorHex;
	m_prevCursorHex = m_cursorHex;
	m_cursorHex = newHex.offsetCoords();

	m_cursorTile = m_hexGrid->getExistingTileData(m_cursorHex);

	constexpr float invisibleChunkLifetime = 3.0f;

	// debug buttons 
	if (kb.keys[int16_t(e2::Key::Num1)].pressed)
	{
		m_altView = !m_altView;
		gameSession()->window()->mouseLock(m_altView);
		if (m_altView)
		{
			m_altViewOrigin =glm::vec3(m_viewOrigin.x, -5.0f, m_viewOrigin.y);
		}

		m_hexGrid->debugDraw(m_altView);
			

	}



	if (kb.keys[int16_t(e2::Key::F1)].pressed)
	{
		m_hexGrid->rebuildForestMeshes();
		m_hexGrid->clearAllChunks();
	}

	if (kb.keys[int16_t(e2::Key::F2)].pressed)
	{
		m_hexGrid->clearLoadTime();
	}

	if (kb.keys[int16_t(e2::Key::F5)].pressed)
	{
		renderManager()->invalidatePipelines();
	}

	if (kb.keys[int16_t(e2::Key::F9)].pressed)
	{
		m_hexGrid->invalidateFogOfWarShaders();
	}

#if defined(E2_PROFILER)
	if (kb.keys[int16_t(e2::Key::F4)].pressed)
	{
		profiler()->stop();
		auto rep =profiler()->report();
		std::stringstream ss;
		ss << std::endl;
		ss << "------------------------------" << std::endl;
		ss << "Profiler report, for " << profiler()->frameCount() << " frames" << std::endl;
		ss << "------------------------------" << std::endl;
		ss << "Groups:" << std::endl;

		for (auto& g : rep.groups)
		{
			ss << std::format("{}: avg frame {:.3f} ms, high frame {:.3f} ms", g.displayName, g.avgTimeInFrame * 1000.0, g.highTimeInFrame * 1000.0) << std::endl;
		}

		for (auto& f : rep.functions)
		{
			ss << std::format("{}: avg {:.3f} ms, high {:.3f} ms | avg frame {:.3f} ms, high frame {:.3f} ms", f.displayName, f.avgTimeInScope * 1000.0, f.highTimeInScope * 1000.0, f.avgTimeInFrame * 1000.0, f.highTimeInFrame * 1000.0) << std::endl;
		}
		LogNotice("{}", ss.str());
		profiler()->start();
	}
#endif


	if (kb.pressed(Key::Escape))
	{
		m_globalState = GlobalState::InGameMenu;
	}

	updateCamera(seconds);

	if (m_hexChanged)
	{
		onNewCursorHex();
	}

	updateGameState();

	updateAnimation(seconds);
	

	//m_cursorHex

	//m_hexGrid->assertChunksWithinRangeVisible(m_viewOrigin, m_viewPoints, m_viewVelocity);
	m_hexGrid->updateStreaming(m_viewOrigin, m_viewPoints, m_viewVelocity);
	m_hexGrid->updateWorldBounds();
	m_hexGrid->renderFogOfWar();

	e2::ITexture* outlineTextures[2] = { m_hexGrid->outlineTexture(0), m_hexGrid->outlineTexture(1) };
	m_session->renderer()->setOutlineTextures(outlineTextures);

	// ticking session renders renderer too, and blits it to the UI, so we need to do it precisely here (after rendering fog of war and before rendering UI)
	m_session->tick(seconds);

	//ui->drawTexturedQuad({}, resolution, 0xFFFFFFFF, m_hexGrid->outlineTexture(renderManager()->frameIndex()));

	if(!kb.state(Key::C))
		drawUI();

}

void e2::Game::updateMenu(double seconds)
{
	static double timer = 0.0;
	static int64_t frames = 0;
	frames++;
	if(frames> 5)
		timer += seconds;

	e2::GameSession* session = gameSession();
	e2::Renderer* renderer = session->renderer();
	e2::UIContext* ui = session->uiContext();
	auto& kb = ui->keyboardState();
	auto& mouse = ui->mouseState();
	auto& leftMouse = mouse.buttons[uint16_t(e2::MouseButton::Left)];

	m_session->renderer()->setEnvironment(m_irradianceMap->handle(), m_radianceMap->handle());

	if (!m_haveBegunStart && kb.pressed(Key::Escape))
		timer = 20.0;

#if !defined(E2_SHIPPING)
	//if (timer < 20.0)
	//	timer = 20.0;
#endif

	m_viewZoom = 0.5f;


	m_view = calculateRenderView(m_viewOrigin);
	m_viewPoints = e2::Viewpoints2D(renderer->resolution(), m_view, 0.0);
	renderer->setView(m_view);


	m_hexGrid->updateStreaming(m_viewOrigin, m_viewPoints, m_viewVelocity);
	m_hexGrid->updateWorldBounds();
	m_hexGrid->renderFogOfWar();
	e2::ITexture* outlineTextures[2] = { m_hexGrid->outlineTexture(0), m_hexGrid->outlineTexture(1) };
	m_session->renderer()->setOutlineTextures(outlineTextures);

	// ticking session renders renderer too, and blits it to the UI, so we need to do it precisely here (after rendering fog of war and before rendering UI)
	m_session->tick(seconds);

	glm::vec2 resolution = renderer->resolution();
	



	if (m_mainMenuState != MainMenuState::Main)
	{
		if (kb.pressed(Key::Escape))
		{
			m_mainMenuState = MainMenuState::Main;
		}
		double width = ui->calculateSDFTextWidth(FontFace::Serif, 42.0, "Reveal & Annihilate");


		double menuHeight = 280.0;
		double menuOffset = resolution.y / 2.0 - menuHeight / 2.0;
		double cursorY = menuOffset;
		double xOffset = resolution.x / 2.0 - width / 2.0;

		if (m_mainMenuState == MainMenuState::Load)
		{
			for (uint8_t sl = 0; sl < e2::numSaveSlots; sl++)
			{
				double slotWidth = ui->calculateSDFTextWidth(FontFace::Serif, 24.0f, saveSlots[sl].cachedDisplayName);

				bool slotHovered = mouse.relativePosition.x > xOffset && mouse.relativePosition.x < xOffset + slotWidth
					&& mouse.relativePosition.y > cursorY && mouse.relativePosition.y < cursorY + 40.0;

				ui->drawSDFText(FontFace::Serif, 24.0f, 0x000000FF, glm::vec2(xOffset, cursorY + 20.0), saveSlots[sl].cachedDisplayName);
				if (saveSlots[sl].exists && slotHovered && leftMouse.clicked)
				{
					loadGame(sl);
				}

				cursorY += 40.0;
			}
		}



		return;
	}



	double globalMenuFade = m_haveBegunStart ? double(1.0 - glm::smoothstep(0.0, 2.0, m_beginStartTime.durationSince().seconds())) : 1.0;

	// tinted block
	double blockTimer1 = glm::smoothstep(12.0, 15.0, double(timer));
	double blockAlpha = 0.9f - blockTimer1 * 0.9f;
	ui->drawQuad({}, resolution, e2::UIColor(102, 88, 66, uint8_t(blockAlpha * 255.0)));

	// black screen
	double blockAlpha2 = 1.0 - glm::smoothstep(8.0, 10.0, double(timer));
	ui->drawQuad({}, resolution, e2::UIColor(0, 0, 0, uint8_t(blockAlpha2 * 255.0)));


	double authorWidth = ui->calculateSDFTextWidth(FontFace::Serif, 36.0f, "Fredrik Haikarainen");
	double authorWidth2 = ui->calculateSDFTextWidth(FontFace::Serif, 24.0f, "A game by");
	double authorAlpha = (double)glm::smoothstep(3.0, 4.0, timer);
	double authorAlpha2 = (double)glm::smoothstep(2.0, 3.0, timer);

	double authorFadeOut = double(1.0 - glm::smoothstep(6.0, 7.0, timer));
	authorAlpha *= authorFadeOut;
	authorAlpha2 *= authorFadeOut;

	e2::UIColor authorColor(255, 255, 255, uint8_t(authorAlpha * 255.0));
	e2::UIColor authorColor2(255, 255, 255, uint8_t(authorAlpha2 * 255.0));
	ui->drawSDFText(FontFace::Serif, 36.0, authorColor, glm::vec2(resolution.x / 2.0 - authorWidth / 2.0, resolution.y / 2.0), "Fredrik Haikarainen");
	ui->drawSDFText(FontFace::Serif, 14.0, authorColor2, glm::vec2(resolution.x / 2.0 - authorWidth2 / 2.0, (resolution.y / 2.0) - 36.0), "A game by");

	double width = ui->calculateSDFTextWidth(FontFace::Serif, 42.0f, "Reveal & Annihilate");

	double menuHeight = 280.0;
	double menuOffset = resolution.y / 2.0 - menuHeight / 2.0;

	double cursorY = menuOffset;
	double xOffset = resolution.x / 2.0 - width / 2.0;


	double blockAlpha3 = (double)glm::smoothstep(10.0, 12.0, timer);

	e2::UIColor textColor(0, 0, 0, uint8_t(blockAlpha3 * 255.0 * globalMenuFade));
	e2::UIColor textColor2(0, 0, 0, uint8_t(blockTimer1 * 170.0 * globalMenuFade));
	ui->drawSDFText(FontFace::Serif, 42.0f, textColor, glm::vec2(xOffset, cursorY), "Reveal & Annihilate");

	static double a = 0.0;
	a += seconds;

	// outwards 
	double sa = glm::simplex(glm::vec2(a*2.0, 0.0));
	// rotation
	double sb = glm::simplex(glm::vec2(a*50.0 + 21.f, 0.0));

	sa = glm::pow(sa, 4.0);
	sb = glm::pow(sb, 2.0);

	if (glm::abs(sa) < 0.2f)
		sa = 0.0;

	if (glm::abs(sb) < 0.5f)
		sb = 0.0;

	glm::vec2 ori(0.0f, float(-sa) * 10.0f);
	glm::vec2 offset1 = e2::rotate2d(ori, float(sb) * 360.0f);

	glm::vec2 ori2(0.0f, float(-sb) * 5.0f);
	glm::vec2 offset2 = e2::rotate2d(ori2, float(sb) * 360.0f);



	ui->drawSDFText(FontFace::Serif, 42.0f, textColor2, glm::vec2(xOffset, cursorY) + offset1, "Reveal & Annihilate");
	ui->drawSDFText(FontFace::Serif, 42.0f, textColor2, glm::vec2(xOffset, cursorY) + offset2, "Reveal & Annihilate");

	cursorY += 64.0;

	double blockAlphaNewGame = glm::smoothstep(15.0, 15.25, timer);
	e2::UIColor textColorNewGame(0, 0, 0, uint8_t(blockAlphaNewGame * 170.0 * globalMenuFade));
	double newGameWidth = ui->calculateSDFTextWidth(FontFace::Serif, 24.0f, "New Game");
	bool newGameHovered = !m_haveBegunStart && timer > 15.25&& mouse.relativePosition.x > xOffset && mouse.relativePosition.x < xOffset + newGameWidth
		&& mouse.relativePosition.y > cursorY + 20.0 && mouse.relativePosition.y < cursorY + 60.0;
	ui->drawSDFText(FontFace::Serif, 24.0f, newGameHovered ? 0x000000FF : textColorNewGame, glm::vec2(xOffset, cursorY + 40.0), "New Game");
	if (!m_haveBegunStart && newGameHovered && leftMouse.clicked)
	{
		beginStartGame();
	}

	double blockAlphaLoadGame = glm::smoothstep(15.25, 15.5, timer);
	e2::UIColor textColorLoadGame(0, 0, 0, uint8_t(blockAlphaLoadGame * 170.0 * globalMenuFade));
	double loadGameWidth = ui->calculateSDFTextWidth(FontFace::Serif, 24.0f, "Load Game");
	bool loadGameHovered = !m_haveBegunStart && timer > 15.5 && mouse.relativePosition.x > xOffset && mouse.relativePosition.x < xOffset + loadGameWidth
		&& mouse.relativePosition.y > cursorY + 60.0 && mouse.relativePosition.y < cursorY + 100.0;
	ui->drawSDFText(FontFace::Serif, 24.0f, loadGameHovered ? 0x000000FF : textColorLoadGame, glm::vec2(xOffset, cursorY + 40.0 * 2), "Load Game");
	if (!m_haveBegunStart && loadGameHovered && leftMouse.clicked)
	{
		m_mainMenuState = MainMenuState::Load;
	}



	double blockAlphaOptions = glm::smoothstep(15.5, 15.75, timer);
	e2::UIColor textColorOptions(0, 0, 0, uint8_t(blockAlphaOptions * 170.0 * globalMenuFade));
	double optionsWidth = ui->calculateSDFTextWidth(FontFace::Serif, 24.0f, "Options");
	bool optionsHovered = !m_haveBegunStart && timer > 15.75 && mouse.relativePosition.x > xOffset && mouse.relativePosition.x < xOffset + optionsWidth
		&& mouse.relativePosition.y > cursorY + 100.0 && mouse.relativePosition.y < cursorY + 140.0;
	ui->drawSDFText(FontFace::Serif, 24.0f, optionsHovered ? 0x000000FF : textColorOptions, glm::vec2(xOffset, cursorY + 40.0 * 3), "Options");
	if (!m_haveBegunStart && optionsHovered && leftMouse.clicked)
	{
		m_mainMenuState = MainMenuState::Options;
	}


	double blockAlphaQuit = glm::smoothstep(15.75, 16.0, timer);
	e2::UIColor textColorQuit(0, 0, 0, uint8_t(blockAlphaQuit * 170.0 * globalMenuFade));
	double quitWidth = ui->calculateSDFTextWidth(FontFace::Serif, 24.0f, "Quit");
	bool quitHovered = !m_haveBegunStart && timer > 16.0 && mouse.relativePosition.x > xOffset && mouse.relativePosition.x < xOffset + optionsWidth
		&& mouse.relativePosition.y > cursorY + 140.0 && mouse.relativePosition.y < cursorY + 180.0;
	ui->drawSDFText(FontFace::Serif, 24.0f, quitHovered ? 0x000000FF : textColorQuit, glm::vec2(xOffset, cursorY + 40.0 * 4), "Quit");
	if (quitHovered && leftMouse.clicked)
	{

		nukeGame();

		engine()->shutdown();
	}

	if (m_haveBegunStart && !m_haveStreamedStart && m_hexGrid->numJobsInFlight() == 0 && m_beginStartTime.durationSince().seconds() > 0.25f)
	{
		m_haveStreamedStart = true;
		m_beginStreamTime = e2::timeNow();

		glm::vec2 startCoords = e2::Hex(m_startLocation).planarCoords();
		m_hexGrid->initializeWorldBounds(startCoords);

		// spawn local empire
		m_localEmpireId = spawnEmpire();
		m_localEmpire = m_empires[m_localEmpireId];
		m_localEmpire->resources.funds.gold = 100.0f;
		//discoverEmpire(m_localEmpireId);

		m_nomadEmpireId = spawnEmpire();
		m_nomadEmpire = m_empires[m_nomadEmpireId];
		//discoverEmpire(m_nomadEmpireId);
		m_nomadEmpire->ai = e2::create<e2::NomadAI>(this, m_nomadEmpireId);

		spawnEntity("city", m_startLocation, m_localEmpireId);
		//spawnCity(m_localEmpireId, m_startLocation);

		//spawnEntity("mobile_mob", m_startLocation, m_localEmpireId);

	}
	if (m_haveStreamedStart)
	{
		double sec = m_beginStreamTime.durationSince().seconds();
		double a = glm::smoothstep(0.0, 2.0, sec);
		m_viewOrigin = glm::mix(m_startViewOrigin, e2::Hex(m_startLocation).planarCoords(), glm::exponentialEaseInOut(a));
	}

	if (m_haveStreamedStart && m_beginStreamTime.durationSince().seconds() > 2.1)
	{
		
		resumeWorldStreaming();
		startGame();
		m_haveBegunStart = false;
		m_haveStreamedStart = false;
	}

}

void e2::Game::pauseWorldStreaming()
{
	m_hexGrid->m_streamingPaused = true;
	m_hexGrid->clearQueue();
}

void e2::Game::resumeWorldStreaming()
{
	m_hexGrid->m_streamingPaused = false;
}

void e2::Game::forceStreamLocation(glm::vec2 const& planarCoords)
{
	glm::vec2 offset = planarCoords - m_viewOrigin;
	e2::Viewpoints2D forceView = m_viewPoints;
	forceView.bottomLeft += offset;
	forceView.bottomRight += offset;
	forceView.topLeft += offset;
	forceView.topRight += offset;
	forceView.view.origin.x += offset.x;
	forceView.view.origin.z += offset.y;
	forceView.calculateDerivatives();

	m_hexGrid->forceStreamView(forceView);
}

void e2::Game::beginStartGame()
{
	m_haveBegunStart = true;
	m_beginStartTime = e2::timeNow();
	do {} while (!findStartLocation(e2::randomIvec2(glm::ivec2(-1024), glm::ivec2(1024)), { 1024, 1024 }, m_startLocation, false));
	pauseWorldStreaming();
	forceStreamLocation(e2::Hex(m_startLocation).planarCoords());
}

bool e2::Game::findStartLocation(glm::ivec2 const& offset, glm::ivec2 const& rangeSize, glm::ivec2 &outLocation, bool forAi)
{
	glm::ivec2 returner;
	int32_t numTries = 0;

	// plop us down somehwere nice 
	std::unordered_set<glm::ivec2> attemptedStartLocations;
	bool foundStartLocation{};
	while (!foundStartLocation)
	{
		glm::ivec2 rangeStart = offset - (rangeSize / 2);
		glm::ivec2 rangeEnd = offset + (rangeSize / 2);
		glm::ivec2 startLocation = e2::randomIvec2(rangeStart, rangeEnd);

		//glm::ivec2 startLocation = e2::randomIvec2({ -512, -512 }, { 512, 512 });
		if (attemptedStartLocations.contains(startLocation))
			continue;

		attemptedStartLocations.insert(startLocation);

		e2::TileData startTile = m_hexGrid->getCalculatedTileData(startLocation);
		if (!startTile.isPassable(e2::PassableFlags::Land))
			continue;

		constexpr bool ignoreVisibility = true;
		auto as = e2::create<e2::PathFindingAS>(this, e2::Hex(startLocation), 64, ignoreVisibility, e2::PassableFlags::Land, false, nullptr);
		uint64_t numWalkableHexes = as->hexIndex.size();
		e2::destroy(as);

		if (numWalkableHexes < 64)
			continue;

		if (forAi)
		{

		}

		returner = startLocation;
		foundStartLocation = true;

		if (numTries++ >= 20)
			break;
	}

	if(foundStartLocation)
		outLocation = returner;

	return foundStartLocation;
}

void e2::Game::startGame()
{
	if (m_globalState != GlobalState::Menu)
		return;
	m_globalState = GlobalState::Game;

	// kick off gameloop
	onStartOfTurn();


}

void e2::Game::addScreenShake(float intensity)
{
	m_shakeIntensity = glm::max(intensity, m_shakeIntensity);
}

glm::vec2 e2::Game::worldToPixels(glm::vec3 const& world)
{
	auto renderer = gameSession()->renderer();
	glm::dvec2 resolution = renderer->resolution();
	glm::dmat4 vpMatrix = game()->view().calculateProjectionMatrix(resolution) * game()->view().calculateViewMatrix();

	glm::vec4 viewPos = vpMatrix * glm::dvec4(glm::dvec3(world), 1.0);
	viewPos = viewPos / viewPos.z;

	glm::vec2 offset = (glm::vec2(viewPos.x, viewPos.y) * 0.5f + 0.5f) * glm::vec2(resolution);
	return offset;

}

e2::ApplicationType e2::Game::type()
{
	return e2::ApplicationType::Game;
}

e2::Game* e2::Game::game()
{
	return this;
}

void e2::Game::updateCamera(double seconds)
{
	updateMainCamera(seconds);
	updateAltCamera(seconds);
}

void e2::Game::updateGameState()
{
	if (!m_empires[0] || !m_localEmpire)
	{
		return;
	}

	if (m_state == GameState::TurnPreparing)
	{
		// do turn prepare things here , and then move on to turn when done 
		bool notDoneYet = false;
		if (notDoneYet)
			return;

		onTurnPreparingEnd();
		m_state = GameState::Turn;
		onStartOfTurn();
	}
	else if (m_state == GameState::Turn)
	{

		if (m_turnState == TurnState::Unlocked)
			updateTurn();
		//else if (m_turnState == TurnState::Auto)
//			updateAuto();
		else if (m_turnState == TurnState::UnitAction_Move)
			updateUnitMove();
		else if (m_turnState == TurnState::EntityAction_Target)
			updateTarget();
		else if (m_turnState == TurnState::EntityAction_Generic)
			updateCustomAction();
		else if (m_turnState == TurnState::WavePreparing)
			updateWavePreparing();
		else if (m_turnState == TurnState::WaveEnding)
			updateWaveEnding();
		else if (m_turnState == TurnState::Wave)
			updateWave();
			
	}
	else if (m_state == GameState::TurnEnding)
	{
		bool notDoneYet = false;
		if (notDoneYet)
			return;

		onTurnEndingEnd();

		// flip through to next empire to let the mhave its turn, if it doesnt exist, keep doing it until we find one or we run out

		//m_empireTurn++;
		//if (m_empireTurn >= e2::maxNumEmpires)
		//{
		//	m_empireTurn = 0;
		//	m_turn++;
		//}
		//else
		//{
		//	while (m_empireTurn < e2::maxNumEmpires)
		//	{
		//		if (!m_empires[m_empireTurn])
		//		{
		//			m_empireTurn++;
		//		}
		//		else break;
		//	}

		//	if (m_empireTurn >= e2::maxNumEmpires)
		//	{
		//		m_empireTurn = 0;
		//		m_turn++;
		//	}
		//}

		m_turn++;

		
		m_state = GameState::TurnPreparing;
		onTurnPreparingBegin();
	}
}

void e2::Game::updateTurn()
{
	// if local is making turn
	//if (m_empireTurn == m_localEmpireId)
		updateTurnLocal();
	//else
		//updateTurnAI();
}

void e2::Game::updateWavePreparing()
{
	for (e2::GameEntity* entity : m_waveEntities)
	{
		entity->onWaveUpdate(m_timeDelta);
	}

	m_waveDeforestTimer += m_timeDelta;
	while (m_waveDeforestTimer > 0.2f)
	{
		m_waveDeforestTimer -= 0.2f;
		if (m_waveDeforestIndex < m_wave->path.size())
		{
			glm::ivec2 currHex = m_wave->path[m_waveDeforestIndex].offsetCoords();
			if(!m_hexGrid->getExistingTileData(currHex))
				m_hexGrid->discover(currHex);

			removeWood(currHex);
			m_waveDeforestIndex++;
		}
	}

	// on predicate do this:
	if (m_waveDeforestIndex >= m_wave->path.size() - 1)
	{
		m_turnState = TurnState::Wave;
		for (e2::GameEntity* entity : m_waveEntities)
		{
			entity->onWaveStart();
		}
	}
}

void e2::Game::updateWave()
{
	if (!m_wave)
	{
		m_turnState = TurnState::Unlocked;
		return;
	}

	m_wave->update(m_timeDelta);

	if (m_wave->completed())
	{

		for (e2::GameEntity* entity : m_waveEntities)
		{
			entity->onWaveEnding();
		}

		m_turnState = TurnState::WaveEnding; 

		return;
	}

	for (e2::GameEntity *entity : m_waveEntities)
	{
		entity->onWaveUpdate(m_timeDelta);
	}
}


void e2::Game::updateWaveEnding()
{
	for (e2::GameEntity* entity : m_waveEntities)
	{
		entity->onWaveUpdate(m_timeDelta);
	}

	// @todo on predicate  do this :
	for (e2::GameEntity* entity : m_waveEntities)
	{
		entity->onWaveEnd();
	}

	e2::destroy(m_wave);
	m_wave = nullptr;
	m_turnState = TurnState::Unlocked;
}

void e2::Game::updateTurnLocal()
{
	constexpr float moveSpeed = 10.0f;
	constexpr float viewSpeed = .3f;

	e2::GameSession* session = gameSession();
	e2::Renderer* renderer = session->renderer();
	e2::UIContext* ui = session->uiContext();
	auto& kb = ui->keyboardState();
	auto& mouse = ui->mouseState();

	auto& leftMouse = mouse.buttons[uint8_t(e2::MouseButton::Left)];
	auto& rightMouse = mouse.buttons[uint8_t(e2::MouseButton::Right)];

	// turn logic here
	if (!m_uiHovered && leftMouse.clicked && leftMouse.dragDistance <= 2.0f)
	{
		if (kb.state(e2::Key::LeftAlt))
		{
			// ugly line of code, no I wont fix it 
			bool onLand = m_cursorTile ? m_cursorTile->getWater() == TileFlags::WaterNone : m_hexGrid->getCalculatedTileData(m_cursorHex).getWater() == TileFlags::WaterNone;
			bool unitSlotTaken = entityAtHex(e2::EntityLayerIndex::Unit, m_cursorHex) != nullptr;

			if (!unitSlotTaken)
			{
				if (onLand)
					spawnEntity("hive", m_cursorHex, m_nomadEmpireId);
				else
					spawnEntity("cb90", m_cursorHex, m_nomadEmpireId);
			}
			return;
		}

		if (kb.state(e2::Key::LeftControl))
		{
			// ugly line of code, no I wont fix it 
			bool onLand = m_cursorTile ? m_cursorTile->getWater() == TileFlags::WaterNone : m_hexGrid->getCalculatedTileData(m_cursorHex).getWater() == TileFlags::WaterNone;
			bool unitSlotTaken = entityAtHex(e2::EntityLayerIndex::Unit, m_cursorHex) != nullptr;

			if (!unitSlotTaken)
			{
				if (onLand)
					spawnEntity("nimble", m_cursorHex, m_localEmpireId);
				else
					spawnEntity("cb90", m_cursorHex, m_nomadEmpireId);
			}
			return;
		}

		if (kb.state(e2::Key::LeftShift))
		{
			// ugly line of code, no I wont fix it 
			bool onLand = m_cursorTile ? m_cursorTile->getWater() == TileFlags::WaterNone : m_hexGrid->getCalculatedTileData(m_cursorHex).getWater() == TileFlags::WaterNone;
			bool unitSlotTaken = entityAtHex(e2::EntityLayerIndex::Unit, m_cursorHex) != nullptr;

			if (!unitSlotTaken)
			{
				if (onLand)
					spawnEntity("grunt", m_cursorHex, m_localEmpireId);
				else
					spawnEntity("cb90", m_cursorHex, m_localEmpireId);
			}
			return;
		}
		e2::GameEntity* unitAtHex = entityAtHex(e2::EntityLayerIndex::Unit, m_cursorHex);
		if (unitAtHex && unitAtHex->empireId != m_localEmpireId)
			unitAtHex = nullptr;
		bool unitSelected = m_selectedEntity && unitAtHex == m_selectedEntity;

		e2::GameEntity* structureAtHex = entityAtHex(e2::EntityLayerIndex::Structure, m_cursorHex);
		if (structureAtHex && structureAtHex->empireId != m_localEmpireId)
			structureAtHex = nullptr;
		bool structureSelected = m_selectedEntity && structureAtHex == m_selectedEntity;

		e2::GameEntity* airUnitAtHex = entityAtHex(e2::EntityLayerIndex::Air, m_cursorHex);
		if (airUnitAtHex && airUnitAtHex->empireId != m_localEmpireId )
			airUnitAtHex = nullptr;
		bool airUnitSelected = m_selectedEntity && airUnitAtHex == m_selectedEntity;

		if (unitAtHex && !unitSelected)
			selectEntity(unitAtHex);
		else if (structureAtHex && !structureSelected)
			selectEntity(structureAtHex);
		else if (!unitAtHex && !structureAtHex && !airUnitAtHex)
			deselectEntity();
	}

	if (!m_uiHovered && rightMouse.clicked && rightMouse.dragDistance <= 2.0f)
	{
		moveSelectedEntityTo(m_cursorHex);
	}
}

//void e2::Game::updateTurnAI()
//{
//	e2::GameEmpire* empire = m_empires[m_empireTurn];
//	if (empire->ai)
//		empire->ai->grugBrainTick(game()->timeDelta());
//}


void e2::Game::endTurn()
{
	if (m_state != GameState::Turn || m_turnState != TurnState::Unlocked)
		return;

	onEndOfTurn();
	m_state = GameState::TurnEnding;
	onTurnEndingBegin();
}

void e2::Game::onTurnPreparingBegin()
{

}

void e2::Game::onTurnPreparingEnd()
{

}

void e2::Game::onStartOfTurn()
{
	// ignore visibility for AI since we dont want to see what theyre up to 
	//if (m_empireTurn == m_localEmpireId)
	//{
	//	while (m_undiscoveredEmpires.size() < 3)
	//		spawnAIEmpire();


		// clear and calculate visibility
		m_hexGrid->clearVisibility();

		for (e2::GameEntity* entity : m_localEmpire->entities)
		{
			entity->spreadVisibility();
		}

	//}

	//m_empires[m_empireTurn]->resources.onNewTurn();
	m_empires[m_localEmpireId]->resources.onNewTurn();

	for (e2::GameEntity* entity : m_empires[m_localEmpireId]->entities)
	{
		entity->onTurnStart();
	}


	//if (m_empireTurn == m_localEmpireId)
	//{
		m_localTurnEntities.clear();
		for (e2::GameEntity* entity : m_localEmpire->entities)
		{
			entity->updateGrugVariables();

			if (entity->playerRelevant())
			{
				m_localTurnEntities.insert(entity);
			}
		}
		
		if (m_localTurnEntities.size() > 0)
			nextLocalEntity();
	//}
	//else if (m_empires[m_empireTurn]->ai)
	//{
	//	m_empires[m_empireTurn]->ai->grugBrainWakeUp();
	//}
}

void e2::Game::onEndOfTurn()
{
	for (e2::GameEntity* entity : m_empires[m_localEmpireId]->entities)
	{
		entity->onTurnEnd();
	}

	if (m_empires[m_localEmpireId]->ai)
		m_empires[m_localEmpireId]->ai->grugBrainGoSleep();

}

void e2::Game::onTurnEndingBegin()
{

}

void e2::Game::onTurnEndingEnd()
{
	deselectEntity();
}

void e2::Game::updateUnitMove()
{
	if (m_selectedEntity->specification->moveType == EntityMoveType::Linear)
	{
		m_unitMoveDelta += float(m_timeDelta) * m_selectedEntity->specification->moveSpeed * (m_ffwMove ? 100.0f : 1.0f);

		while (m_unitMoveDelta > 1.0f)
		{
			m_unitMoveDelta -= 1.0;
			m_unitMoveIndex++;

			if (m_unitMoveIndex >= m_unitMovePath.size() - 1)
			{
				e2::Hex prevHex = m_unitMovePath[m_unitMovePath.size() - 2];
				e2::Hex finalHex = m_unitMovePath[m_unitMovePath.size() - 1];
				// we are done
				if (m_selectedEntity->isLocal())
					m_selectedEntity->rollbackVisibility();

				m_entityLayers[size_t(m_selectedEntity->specification->layerIndex)].entityIndex.erase(m_selectedEntity->tileIndex);
				m_selectedEntity->tileIndex = finalHex.offsetCoords();
				m_entityLayers[size_t(m_selectedEntity->specification->layerIndex)].entityIndex[m_selectedEntity->tileIndex] = m_selectedEntity;

				if (m_selectedEntity->isLocal())
					m_selectedEntity->spreadVisibility();



				float angle = e2::radiansBetween(finalHex.localCoords(), prevHex.localCoords());
				m_selectedEntity->setMeshTransform(finalHex.localCoords(), angle);

				resolveSelectedEntity();

				m_selectedEntity->onEndMove();

				m_turnState = m_moveTurnStateFallback;

				resolveLocalEntity();

				return;
			}
		}

		e2::Hex currHex = m_unitMovePath[m_unitMoveIndex];
		e2::Hex nextHex = m_unitMovePath[m_unitMoveIndex + 1];

		glm::vec3 currHexPos = currHex.localCoords();
		glm::vec3 nextHexPos = nextHex.localCoords();

		glm::vec3 newPos = glm::mix(currHexPos, nextHexPos, m_unitMoveDelta);
		float angle = radiansBetween(nextHexPos, currHexPos);
		m_selectedEntity->setMeshTransform(newPos, angle);
	}
	else if (m_selectedEntity->specification->moveType == e2::EntityMoveType::Smooth)
	{

		m_unitMoveDelta += float(m_timeDelta) * m_selectedEntity->specification->moveSpeed * (m_ffwMove ? 100.0f : 1.0f);

		while (m_unitMoveDelta > 1.0f)
		{
			m_unitMoveDelta -= 1.0;
			m_unitMoveIndex++;

			if (m_unitMoveIndex >= m_unitMovePath.size() - 1)
			{
				e2::Hex prevHex = m_unitMovePath[m_unitMovePath.size() - 2];
				e2::Hex finalHex = m_unitMovePath[m_unitMovePath.size() - 1];
				// we are done
				if (m_selectedEntity->isLocal())
					m_selectedEntity->rollbackVisibility();

				m_entityLayers[size_t(m_selectedEntity->specification->layerIndex)].entityIndex.erase(m_selectedEntity->tileIndex);
				m_selectedEntity->tileIndex = finalHex.offsetCoords();
				m_entityLayers[size_t(m_selectedEntity->specification->layerIndex)].entityIndex[m_selectedEntity->tileIndex] = m_selectedEntity;

				if (m_selectedEntity->isLocal())
					m_selectedEntity->spreadVisibility();

				float angle = e2::radiansBetween(finalHex.localCoords(), prevHex.localCoords());
				m_selectedEntity->setMeshTransform(finalHex.localCoords(), angle);

				resolveSelectedEntity();
				m_selectedEntity->onEndMove();

				m_turnState = m_moveTurnStateFallback;

				resolveLocalEntity();

				return;
			}
		}


		e2::Hex currHex = m_unitMovePath[m_unitMoveIndex];
		e2::Hex nextHex = m_unitMovePath[m_unitMoveIndex + 1];

		glm::vec3 currHexPos = currHex.localCoords();
		glm::vec3 nextHexPos = nextHex.localCoords();


		glm::vec3 prevHexPos = m_unitMoveIndex > 0 ? m_unitMovePath[m_unitMoveIndex - 1].localCoords() : currHexPos - (nextHexPos - currHexPos);

		glm::vec3 nextNextHexPos = m_unitMoveIndex + 2 < m_unitMovePath.size() ? m_unitMovePath[m_unitMoveIndex + 2].localCoords() : nextHexPos + (nextHexPos - currHexPos);


		glm::vec3 newPos = glm::catmullRom(prevHexPos, currHexPos, nextHexPos, nextNextHexPos, m_unitMoveDelta);
		glm::vec3 newPos2 = glm::catmullRom(prevHexPos, currHexPos, nextHexPos, nextNextHexPos, m_unitMoveDelta + 0.01f);


		constexpr bool debugRender = false;
		if (debugRender)
		{
			for (uint32_t k = 0; k < m_unitMovePath.size() - 1; k++)
			{

				e2::Hex currHex = m_unitMovePath[k];
				e2::Hex nextHex = m_unitMovePath[k + 1];

				glm::vec3 currHexPos = currHex.localCoords();
				glm::vec3 nextHexPos = nextHex.localCoords();


				glm::vec3 prevHexPos = k > 0 ? m_unitMovePath[k - 1].localCoords() : currHexPos - (nextHexPos - currHexPos);

				glm::vec3 nextNextHexPos = k + 2 < m_unitMovePath.size() ? m_unitMovePath[k + 2].localCoords() : nextHexPos + (nextHexPos - currHexPos);


				constexpr uint32_t debugResolution = 8;
				for (uint32_t i = 0; i < debugResolution; i++)
				{
					float ii = glm::clamp(float(i) / float(debugResolution), 0.0f, 1.0f);
					float ii2 = glm::clamp(float(i + 1) / float(debugResolution), 0.0f, 1.0f);
					glm::vec3 currPos = glm::catmullRom(prevHexPos, currHexPos, nextHexPos, nextNextHexPos, ii);
					glm::vec3 nextPos = glm::catmullRom(prevHexPos, currHexPos, nextHexPos, nextNextHexPos, ii2);

					gameSession()->renderer()->debugLine(glm::vec3(1.0f, ii, 1.0f), currPos, nextPos);
				}
			}
		}


		float angle = radiansBetween(newPos2, newPos);
		m_selectedEntity->setMeshTransform(newPos, angle);
	}

	if(!m_selectedEntity->isLocal() && hexGrid()->isVisible(m_selectedEntity->tileIndex))
		m_viewOrigin = m_selectedEntity->meshPlanarCoords();
}

void e2::Game::updateTarget()
{
	e2::GameSession* session = gameSession();
	e2::Renderer* renderer = session->renderer();
	e2::UIContext* ui = session->uiContext();
	auto& kb = ui->keyboardState();
	auto& mouse = ui->mouseState();
	auto& leftMouse = mouse.buttons[uint16_t(e2::MouseButton::Left)];

	if (!m_selectedEntity)
		return;

	if (m_hexChanged)
	{
		m_selectedEntity->onTargetChanged(m_cursorHex);
	}

	if (leftMouse.clicked)
	{
		m_selectedEntity->onTargetClicked();
	}
}


e2::GameEntity* e2::Game::spawnEntity(e2::Name entityId, glm::ivec2 const& location, EmpireId empire)
{
	e2::EntitySpecification* spec = EntitySpecification::specificationById(entityId);
	if (!spec)
	{
		LogError("no entity with id {}, returning null", entityId.cstring());
		return nullptr;
	}

	e2::Object* newEntity = spec->type->create();
	if (!newEntity)
	{
		LogError("failed to create instance of type {} for entity with id {}", spec->type->fqn.cstring(), entityId.cstring());
		return nullptr;
	}

	e2::GameEntity* asEntity= newEntity->cast<e2::GameEntity>();
	if (!asEntity)
	{
		e2::destroy(newEntity);
		LogError("created entity is invalid wtf");
		return nullptr;
	}

	// post construct since we created it dynamically
	asEntity->postConstruct(this, spec, location, empire);

	// initialize it (will create mesh proxies etc)
	asEntity->initialize();

	// insert into global entity set 
	m_entities.insert(asEntity);

	if (asEntity->specification->waveRelevant)
	{
		m_waveEntities.insert(asEntity);
	}

	// insert into entity layer index
	m_entityLayers[size_t(asEntity->specification->layerIndex)].entityIndex[asEntity->tileIndex] = asEntity;

	// insert into empire and its fiscal stream 
	m_empires[asEntity->empireId]->entities.insert(asEntity);
	m_empires[asEntity->empireId]->resources.fiscalStreams.insert(asEntity);

	if (spec->layerIndex == EntityLayerIndex::Structure)
	{
		removeWood(asEntity->tileIndex);

		e2::TileData* existingData = m_hexGrid->getExistingTileData(asEntity->tileIndex);
		if (existingData)
		{
			existingData->empireId = empire;
		}
	}

	asEntity->onSpawned();

	return asEntity;
}

void e2::Game::drawUI()
{
	m_uiHovered = false;

	if (!m_altView)
	{
		drawResourceIcons();

		drawHitLabels();


		drawStatusUI();

		drawUnitUI();

		drawMinimapUI();

		drawFinalUI();

		constexpr bool debugShadows = true;
		if(debugShadows)
		{
			glm::vec2 offset{ 300.0f, 40.0f };
			glm::vec2 size{ 120, 220.0f };

			e2::UIContext* ui = gameSession()->uiContext();
			auto& mouse = ui->mouseState();

			bool hovered = !m_viewDragging &&
				(mouse.relativePosition.x > offset.x && mouse.relativePosition.x < offset.x + size.x &&
					mouse.relativePosition.y > offset.y && mouse.relativePosition.y < offset.y + size.y);
			if (hovered)
			{
				m_uiHovered = true;
			}



			ui->pushFixedPanel("envParams", offset, size);
			ui->beginStackV("envParamStack");

			ui->sliderFloat("sunAngleA", m_sunAngleA, -180.0f, 180.0f);
			ui->sliderFloat("sunAngleB", m_sunAngleB, 0.0f, 90.0f);
			//ui->sliderFloat("sunStr", m_sunStrength, 0.0f, 10.0f);
			//ui->sliderFloat("iblStr", m_iblStrength, 0.0f, 10.0f); 

			//ui->sliderFloat("exposure", m_exposure, 0.0f, 20.0f);
			//ui->sliderFloat("whitepoint", m_whitepoint, 0.0f, 20.0f);

			//ui->sliderFloat("mtnFreqScale", e2::mtnFreqScale, 0.001f, 0.1f, "%.6f");
			//ui->sliderFloat("mtnScale", e2::mtnScale, 0.25f, 10.0f, "%.6f");

			ui->sliderFloat("treeScale", e2::treeScale, 0.1f, 2.0f);
			ui->sliderFloat("treeSpread", e2::treeSpread, 0.1f, 2.0f);
			ui->sliderInt("treeNum1", e2::treeNum1, 1, 50);
			ui->sliderInt("treeNum2", e2::treeNum2, 1, 50);
			ui->sliderInt("treeNum3", e2::treeNum3, 1, 50);


			e2::GameSession* session = gameSession();
			e2::Renderer* renderer = session->renderer();

			ui->endStackV();
			ui->popFixedPanel();
		}
	}

	drawDebugUI();
}

void e2::Game::drawResourceIcons()
{
	// @todo move this to the respective structures instead 
	return;

	/*
	e2::GameSession* session = gameSession();
	e2::Renderer* renderer = session->renderer();
	e2::UIContext* ui = session->uiContext();


	glm::dvec2 resolution = renderer->resolution();
	glm::dmat4 vpMatrix = m_view.calculateProjectionMatrix(resolution) * m_view.calculateViewMatrix();



	for (e2::ChunkState* chunk : m_hexGrid->m_chunksInView)
	{
		glm::ivec2 chunkIndex = chunk->chunkIndex;
		glm::ivec2 chunkTileOffset = chunkIndex * glm::ivec2(e2::hexChunkResolution);
		
		for (int32_t y = 0; y < e2::hexChunkResolution; y++)
		{
			for (int32_t x = 0; x < e2::hexChunkResolution; x++)
			{
				glm::ivec2 worldIndex = chunkTileOffset + glm::ivec2(x, y);
				
				e2::TileData* tileData = m_hexGrid->getTileData(worldIndex);
				if (!tileData)
					continue;

				if (!m_hexGrid->isVisible(worldIndex))
					continue;

				e2::TileFlags resource = tileData->getResource();
				e2::TileFlags abundance = tileData->getAbundance();

				glm::vec4 viewPos = vpMatrix* glm::dvec4( glm::dvec3(e2::Hex(worldIndex).localCoords()) + e2::worldUp() * 0.4, 1.0);
				viewPos = viewPos / viewPos.z;

				glm::vec2 offset = (glm::vec2(viewPos.x, viewPos.y) * 0.5f + 0.5f) * glm::vec2(resolution);

				



				auto drawIcon = [abundance, offset, ui, this](glm::vec2 const& uvOffset)	{
					glm::vec2 uvScale = { 1.0f / 7.0f, 1.0f };
					if (abundance == TileFlags::Abundance1)
					{
						glm::vec2 size(32.0f, 32.0f);
						glm::vec2 iconOffset = offset - (size / 2.0f);
						ui->drawTexturedQuad(iconOffset, size, 0xFFFFFFFF, m_uiTextureResources->handle(), uvOffset, uvScale);
					}
					else if (abundance == TileFlags::Abundance2)
					{
						glm::vec2 size(24.0f, 24.0f);
						glm::vec2 iconOffset = offset - (size / 2.0f);
						ui->drawTexturedQuad(iconOffset + glm::vec2(-8.0f, 0.0f), size, 0xFFFFFFFF, m_uiTextureResources->handle(), uvOffset, uvScale);
						ui->drawTexturedQuad(iconOffset + glm::vec2(8.0f, 0.0f), size, 0xFFFFFFFF, m_uiTextureResources->handle(), uvOffset, uvScale);
					}
					else if (abundance == TileFlags::Abundance3)
					{
						glm::vec2 size(24.0f, 24.0f);
						glm::vec2 iconOffset = offset - (size / 2.0f);
						ui->drawTexturedQuad(iconOffset + glm::vec2(-6.0f, 0.0f), size, 0xFFFFFFFF, m_uiTextureResources->handle(), uvOffset, uvScale);
						ui->drawTexturedQuad(iconOffset + glm::vec2(2.0f, -8.0f), size, 0xFFFFFFFF, m_uiTextureResources->handle(), uvOffset, uvScale);
						ui->drawTexturedQuad(iconOffset + glm::vec2(8.0f, 0.0f), size, 0xFFFFFFFF, m_uiTextureResources->handle(), uvOffset, uvScale);
					}
					else if (abundance == TileFlags::Abundance4)
					{
						glm::vec2 size(24.0f, 24.0f);
						glm::vec2 iconOffset = offset - (size / 2.0f);
						ui->drawTexturedQuad(iconOffset + glm::vec2(-8.0f, -8.0f), size, 0xFFFFFFFF, m_uiTextureResources->handle(), uvOffset, uvScale);
						ui->drawTexturedQuad(iconOffset + glm::vec2(8.0f, -8.0f), size, 0xFFFFFFFF, m_uiTextureResources->handle(), uvOffset, uvScale);
						ui->drawTexturedQuad(iconOffset + glm::vec2(-8.0f, 8.0f), size, 0xFFFFFFFF, m_uiTextureResources->handle(), uvOffset, uvScale);
						ui->drawTexturedQuad(iconOffset + glm::vec2(8.0f, 8.0f), size, 0xFFFFFFFF, m_uiTextureResources->handle(), uvOffset, uvScale);
					}
				};

				// gold 
				if(resource == e2::TileFlags::ResourceGold)
					drawIcon({ (1.0f / 7.0f) * 0.0f, 0.0f });
				// wood
				else if (resource == e2::TileFlags::ResourceForest && tileData->improvedResource)
					drawIcon({ (1.0f / 7.0f) * 1.0f, 0.0f });
				// stone 
				else if (resource == e2::TileFlags::ResourceStone && tileData->improvedResource)
					drawIcon({ (1.0f / 7.0f) * 2.0f, 0.0f });
				// metal
				else if (resource == e2::TileFlags::ResourceOre)
					drawIcon({ (1.0f / 7.0f) * 3.0f, 0.0f });
				// oil
				else if (resource == e2::TileFlags::ResourceOil)
					drawIcon({ (1.0f / 7.0f) * 4.0f, 0.0f });
				// uranium
				else if (resource == e2::TileFlags::ResourceUranium)
					drawIcon({ (1.0f / 7.0f) * 5.0f, 0.0f });
			}
		}
	}*/

}

void e2::Game::drawHitLabels()
{
	e2::GameSession* session = gameSession();
	e2::UIContext* ui = session->uiContext();
	for (HitLabel& label : m_hitLabels)
	{
		if (!label.active)
			continue;

		if (label.timeCreated.durationSince().seconds() > 1.0f)
		{
			label.active = false;
			continue;
		}

		label.velocity.y += 20.0f * (float)game()->timeDelta();

		label.offset += label.velocity * (float)game()->timeDelta();

		float fontSize = 14.0f + glm::smoothstep(0.0f, 1.0f, (float)label.timeCreated.durationSince().seconds()) * 10.0f;

		float alpha = (1.0f - glm::smoothstep(0.4f, 1.0f, (float)label.timeCreated.durationSince().seconds())) * 255.f;

		ui->drawSDFText(e2::FontFace::Sans, fontSize, e2::UIColor(255, 255, 255, (uint8_t)alpha), worldToPixels(label.offset), label.text);

	}
}

void e2::Game::drawStatusUI()
{
	e2::GameSession* session = gameSession();
	e2::Renderer* renderer = session->renderer();
	e2::UIContext* ui = session->uiContext();
	auto& kb = ui->keyboardState();
	auto& mouse = ui->mouseState();
	auto& leftMouse = mouse.buttons[uint16_t(e2::MouseButton::Left)];

	e2::IWindow* wnd = session->window();
	glm::vec2 winSize = wnd->size();

	//ui->drawQuadShadow({ -2.0f, -2.0f }, { winSize.x + 4.0f, 30.0f }, 2.0f, 0.9f, 2.0f);
	ui->drawGamePanel({}, { winSize.x, 32.0f }, false, 0.75f);

	float xCursor = 0.0f;
	uint8_t fontSize = 14;

	std::string str;
	float strWidth;

	e2::GameResources& resources = m_empires[m_localEmpireId]->resources;

	float textBaseline = 16.0f;

	// Gold
	e2::Sprite* goldSprite = getIconSprite("gold");
	xCursor += 20.0f;
	ui->drawSprite({ xCursor, 6.0f }, *goldSprite, 0xFFFFFFFF, 20.0f / goldSprite->size.y);
	xCursor += 24.0f;
	xCursor += 8.0f;

	str = std::format("{:}", resources.funds.gold);
	strWidth = ui->calculateTextWidth(e2::FontFace::Sans, fontSize, str);
	ui->drawRasterText(e2::FontFace::Sans, fontSize, 0xFFFFFFFF, { xCursor, textBaseline }, str);
	xCursor += strWidth;

	// Wood
	e2::Sprite* woodSprite = getIconSprite("wood");
	xCursor += 44.0f;
	ui->drawSprite({ xCursor, 6.0f }, *woodSprite, 0xFFFFFFFF, 20.0f / goldSprite->size.y);
	xCursor += 24.0f;
	xCursor += 8.0f;

	str = std::format("{:} *({:+})*", resources.funds.wood, resources.profits.wood);
	strWidth = ui->calculateTextWidth(e2::FontFace::Sans, fontSize, str);
	ui->drawRasterText(e2::FontFace::Sans, fontSize, 0xFFFFFFFF, { xCursor, textBaseline }, str);
	xCursor += strWidth;

	// Iron
	e2::Sprite* oreSprite = getIconSprite("ore");
	xCursor += 44.0f;
	ui->drawSprite({ xCursor, 6.0f }, *oreSprite, 0xFFFFFFFF, 20.0f / goldSprite->size.y);
	xCursor += 24.0f;
	xCursor += 8.0f;

	str = std::format("{:} *({:+})*", resources.funds.iron, resources.profits.iron);
	strWidth = ui->calculateTextWidth(e2::FontFace::Sans, fontSize, str);
	ui->drawRasterText(e2::FontFace::Sans, fontSize, 0xFFFFFFFF, { xCursor, textBaseline }, str);
	xCursor += strWidth;

	// Steel
	e2::Sprite* steelSprite = getIconSprite("beam");
	xCursor += 44.0f;
	ui->drawSprite({ xCursor, 6.0f }, *steelSprite, 0xFFFFFFFF, 20.0f / goldSprite->size.y);
	xCursor += 24.0f;
	xCursor += 8.0f;

	str = std::format("{:}", resources.funds.steel);
	strWidth = ui->calculateTextWidth(e2::FontFace::Sans, fontSize, str);
	ui->drawRasterText(e2::FontFace::Sans, fontSize, 0xFFFFFFFF, { xCursor, textBaseline }, str);
	xCursor += strWidth;


	str = std::format("Turn {}", m_turn);
	strWidth = ui->calculateTextWidth(e2::FontFace::Sans, fontSize, str);
	xCursor = winSize.x - strWidth - 8.0f - 20.0f - 8.0f - 20.0f - 8.0f;
	ui->drawRasterText(e2::FontFace::Sans, fontSize, 0xFFFFFFFF, { xCursor, textBaseline }, str);
	
	e2::Sprite* nextSprite = getUiSprite("endturn");
	xCursor = winSize.x - 20.0f - 8.0f - 20.0f;
	glm::vec2 nextOffset{ xCursor, 6.0f };
	glm::vec2 nextSize{ 20.0f, 20.0f };
	ui->drawSprite(nextOffset, *nextSprite, 0xFFFFFFFF, 20.0f / nextSprite->size.y);

	if (mouse.relativePosition.x > nextOffset.x && mouse.relativePosition.x < nextOffset.x + nextSize.x &&
		mouse.relativePosition.y > nextOffset.y && mouse.relativePosition.y < nextOffset.y + nextSize.y)
	{
		m_uiHovered = true;
		if (leftMouse.pressed)
			endTurn();
	}

	e2::Sprite* menuSprite = getIconSprite("menu");
	xCursor = winSize.x - 20.0f - 8.0f;
	glm::vec2 menuOffset{xCursor, 6.0f};
	glm::vec2 menuSize{20.0f, 20.0f};
	ui->drawSprite(menuOffset, *menuSprite, 0xFFFFFFFF, 20.0f / menuSprite->size.y);

	if (mouse.relativePosition.x > menuOffset.x && mouse.relativePosition.x < menuOffset.x + menuSize.x &&
		mouse.relativePosition.y > menuOffset.y && mouse.relativePosition.y < menuOffset.y + menuSize.y)
	{
		m_uiHovered = true;
		if (leftMouse.pressed)
			m_globalState = GlobalState::InGameMenu;
	}

	/*ui->drawRasterText(e2::FontFace::Serif, 11, 0xFFFFFFFF, { 4.0f + (160.0f * 1.0f), 14.0f }, std::format("Wood: {0:10} ({0:+})", m_resources.funds.wood, m_resources.profits.wood));
	ui->drawRasterText(e2::FontFace::Serif, 11, 0xFFFFFFFF, { 4.0f + (160.0f * 2.0f), 14.0f }, std::format("Stone: {0:10} ({0:+})", m_resources.funds.stone, m_resources.profits.stone));
	ui->drawRasterText(e2::FontFace::Serif, 11, 0xFFFFFFFF, { 4.0f + (160.0f * 3.0f), 14.0f }, std::format("Metal: {0:10} ({0:+})", m_resources.funds.metal, m_resources.profits.metal));
	ui->drawRasterText(e2::FontFace::Serif, 11, 0xFFFFFFFF, { 4.0f + (160.0f * 4.0f), 14.0f }, std::format("Oil: {0:10} ({0:+})", m_resources.funds.oil, m_resources.profits.oil));
	ui->drawRasterText(e2::FontFace::Serif, 11, 0xFFFFFFFF, { 4.0f + (160.0f * 5.0f), 14.0f }, std::format("Uranium: {0:10} ({0:+})", m_resources.funds.uranium, m_resources.profits.uranium));
	ui->drawRasterText(e2::FontFace::Serif, 11, 0xFFFFFFFF, { 4.0f + (160.0f * 6.0f), 14.0f }, std::format("Meteorite: {0:10} ({0:+})", m_resources.funds.meteorite, m_resources.profits.meteorite));*/



}


void e2::Game::drawUnitUI()
{

	e2::GameSession* session = gameSession();
	e2::Renderer* renderer = session->renderer();
	e2::UIContext* ui = session->uiContext();
	auto& kb = ui->keyboardState();
	auto& mouse = ui->mouseState();
	auto& leftMouse = mouse.buttons[uint16_t(e2::MouseButton::Left)];

	e2::IWindow* wnd = session->window();
	glm::vec2 winSize = wnd->size();

	float alpha = glm::mix( 1.0f,  0.75f, m_viewZoom);
	e2::UIColor bgColor = e2::UIColor(glm::vec4{0.0f, 0.0f, 0.0f, alpha});
	e2::UIColor bgColorNomad = e2::UIColor(166, 28, 0, uint8_t(alpha * 255.f));
	e2::UIColor bgColorEnemy = e2::UIColor(7, 82, 163, uint8_t(alpha*255.f));
	for (e2::GameEntity* entity : m_entitiesInView)
	{
		if (!m_hexGrid->isVisible(entity->tileIndex))
		{
			continue;
		}

		bool isPlayer = entity->empireId == m_localEmpireId;
		bool isNomad = entity->empireId == m_nomadEmpireId;
		bool isEnemyCiv = !isPlayer && !isNomad;

		e2::UIColor bg = isPlayer ? bgColor : isNomad ? bgColorNomad : bgColorEnemy;

		bool selected = entity == m_selectedEntity;

		if (!selected)
			bg.a = uint8_t((float)bg.a * 0.8f);


		static const e2::Name badgeBgName = "badge";
		e2::Sprite* badgeBg = getUiSprite(badgeBgName);
		e2::Sprite* badgeSprite;
		
		bool isGlobal = entity->specification->badgeId.string().contains('.');
		if(isGlobal)
			badgeSprite = uiManager()->globalSprite(entity->specification->badgeId);
		else 
			badgeSprite = getUiSprite(entity->specification->badgeId);
		

		
		 
		if (!selected && badgeBg && badgeSprite)
		{
			
			float uiScale = float(winSize.y) / 1080.0f;
			uiScale *= 1.0;

			glm::vec2 badgeSize = glm::vec2( 32.0f * (badgeSprite->size.x / badgeSprite->size.y), 32.0f) * uiScale;

			glm::vec2 badgeShieldSize = glm::vec2(64.0f * (badgeBg->size.x / badgeBg->size.y), 64.0f) * uiScale;

			

			glm::vec2 badgePos = worldToPixels( entity->meshPosition + e2::worldUpf() * 0.75f);
		
			ui->drawSprite(badgePos - badgeShieldSize / 2.0f, *badgeBg, bg, (64.0f/ badgeBg->size.y) * uiScale);

			ui->drawSprite(badgePos - badgeSize / 2.0f, *badgeSprite, 0xFFFFFFFF, (32.0f / badgeSprite->size.y) * uiScale);
		}

	}

	if (!m_selectedEntity || !m_selectedEntity->isLocal())
		return;
	


	float width = 450.0f;
	float height = 180.0f;

	glm::vec2 offset = { winSize.x / 2.0 - width / 2.0, winSize.y - height - 16.0f };

	if (mouse.relativePosition.x > offset.x && mouse.relativePosition.x < offset.x + width &&
		mouse.relativePosition.y > offset.y && mouse.relativePosition.y < offset.y + height)
		m_uiHovered = true;

	ui->drawQuadShadow(offset, {width, height}, 8.0f, 0.9f, 4.0f);
	//uint8_t fontSize = 12;
	//std::string str = m_selectedUnit->displayName;
	//ui->drawRasterText(e2::FontFace::Serif, 14, 0xFFFFFFFF, offset + glm::vec2(8.f, 14.f), str);

	ui->pushFixedPanel("test", offset + glm::vec2(4.0f, 4.0f), glm::vec2(width - 8.0f, height - 8.0f));

	if (m_selectedEntity && m_turnState == TurnState::Unlocked)
	{
		e2::EntitySpecification* spec = m_selectedEntity->specification;

		ui->beginStackV("unitV", glm::vec2(0.0, 0.0));
		ui->beginStackH("headerH", 24.0f);
		ui->gameLabel(std::format("**{}**", spec->displayName));

		
		if (spec->showMovePoints)
		{
			e2::Sprite* moveSprite = uiManager()->globalSprite("gameUi.run");

			ui->sprite(*moveSprite, 0xFFFFFFFF, 0.5f);
			ui->gameLabel(std::to_string(m_selectedEntity->movePointsLeft));
		}

		if (spec->showAttackPoints)
		{
			e2::Sprite* attackSprite = uiManager()->globalSprite("gameUi.attack");

			ui->sprite(*attackSprite, 0xFFFFFFFF, 0.5f);
			ui->gameLabel(std::to_string(m_selectedEntity->attackPointsLeft));
		}

		if (spec->showBuildPoints)
		{
			e2::Sprite* buildSprite = uiManager()->globalSprite("gameUi.engineer");

			ui->sprite(*buildSprite, 0xFFFFFFFF, 0.5f);
			ui->gameLabel(std::to_string(m_selectedEntity->buildPointsLeft));
		}

		ui->endStackH();

		m_selectedEntity->drawUI(ui);

		ui->endStackV();
	}
		



	ui->popFixedPanel();


}

void e2::Game::drawMinimapUI()
{
	e2::GameSession* session = gameSession();
	e2::Renderer* renderer = session->renderer();
	e2::UIContext* ui = session->uiContext();
	auto& kb = ui->keyboardState();
	auto& mouse = ui->mouseState();
	auto& leftMouse = mouse.buttons[uint16_t(e2::MouseButton::Left)];
	e2::IWindow* wnd = session->window();
	glm::vec2 winSize = wnd->size();

	glm::uvec2 miniSize = m_hexGrid->minimapSize();

	float width = (float)miniSize.x;
	float height = (float)miniSize.y;

	glm::vec2 offset = { winSize.x - width - 16.0f , 48.0f };

	bool hovered = !m_viewDragging && 
		(mouse.relativePosition.x > offset.x && mouse.relativePosition.x < offset.x + width &&
		mouse.relativePosition.y > offset.y && mouse.relativePosition.y < offset.y + height);
	if(hovered)
	{
		m_uiHovered = true;
	}


	//ui->drawQuadShadow(offset- glm::vec2(4.0f, 4.0f), {width + 8.0f, height + 8.0f}, 8.0f, 0.9f, 4.0f);
	ui->drawTexturedQuad(offset, { width, height }, 0xFFFFFFFF, m_hexGrid->minimapTexture(renderManager()->frameIndex()));
	ui->drawFrame(offset, { width, height }, e2::gamePanelDarkHalf, 1.0f);
	if (hovered && leftMouse.state)
	{
		e2::Aabb2D viewBounds = m_hexGrid->viewBounds();
		glm::vec2 normalizedMouse = (mouse.relativePosition - offset) / glm::vec2(width, height);
		glm::vec2 worldMouse = viewBounds.min + normalizedMouse * (viewBounds.max - viewBounds.min);
		m_viewOrigin = worldMouse;
	}

	{
		glm::vec2 pos = offset + glm::vec2(0.0f, height + 4.0f);
		glm::vec2 size = { width, 24 };

		static bool grid = false;
		ui->pushFixedPanel("zxccc", pos, size);
		if (ui->checkbox("toggleGrid", grid, "Show Grid"))
		{
			renderer->setDrawGrid(grid);
		}
		ui->popFixedPanel();
	}

	glm::vec2 pos = offset + glm::vec2(0.0f, height + 32.0f);
	glm::vec2 size = { width, 80 };

	bool btnHovered = (mouse.relativePosition.x > pos.x && mouse.relativePosition.x < pos.x + size.x &&
		mouse.relativePosition.y > pos.y && mouse.relativePosition.y < pos.y + size.y);
	ui->drawGamePanel(pos, size, btnHovered, 0.75f);
	if(btnHovered)
		ui->drawFrame(pos, size, e2::gameAccentHalf, 1.0f);
	else 
		ui->drawFrame(pos, size, e2::gamePanelDarkHalf, 1.0f);

	e2::Sprite* droneSprite = getUnitSprite("drone");
	ui->drawSprite(pos + glm::vec2{8.f, 8.f}, * droneSprite, 0xFFFFFFFF, 64.0f / 200.0f);

	ui->drawRasterText(e2::FontFace::Sans, 14, 0xFFFFFFFF, pos + glm::vec2(80.0f, 20.f), "Train Drone", false);

	ui->drawRasterText(e2::FontFace::Sans, 10, 0xFFFFFF7F, pos + glm::vec2(80.0f, 36.f), "Builds districts and turrets.", false);

	e2::Sprite* goldSprite = getIconSprite("gold");
	ui->drawSprite(pos + glm::vec2(80.0f, 40.0f + 10.0f), *goldSprite, 0xFFFFFFFF, 20.0f / goldSprite->size.y);
	float w = ui->calculateTextWidth(FontFace::Sans, 14, "20");
	ui->drawRasterText(FontFace::Sans, 14, 0xFFFFFFFF, pos + glm::vec2(80.0f + 20.0f + 10.0f, 40.0f + 20.0f), "20");

	e2::Sprite* steelSprite = getIconSprite("beam");
	ui->drawSprite(pos + glm::vec2(80.0f + 20.0f + 10.0f + w + 20.0f , 40.0f + 10.0f), *steelSprite, 0xFFFFFFFF, 20.0f / steelSprite->size.y);
	float w2 = ui->calculateTextWidth(FontFace::Sans, 14, "5");
	ui->drawRasterText(FontFace::Sans, 14, 0xFFFFFFFF, pos + glm::vec2(80.0f + 20.0f + 10.0f + w + 20.0f + 20.0f + 10.0f, 40.0f + 20.0f), "5");

	
}

void e2::Game::drawDebugUI()
{
#if defined(E2_PROFILER)
	e2::GameSession* session = gameSession();
	e2::Renderer* renderer = session->renderer();
	e2::UIContext* ui = session->uiContext();
	auto& kb = ui->keyboardState();
	auto& mouse = ui->mouseState();
	auto& leftMouse = mouse.buttons[uint16_t(e2::MouseButton::Left)];

	e2::EngineMetrics& metrics = engine()->metrics();

	float yOffset = 64.0f;
	float xOffset = 12.0f;
	ui->drawQuadShadow({ 0.0f, yOffset - 16.0f }, { 320.0f, 200.0f }, 8.0f, 0.9f, 4.0f);
	ui->drawRasterText(e2::FontFace::Monospace, 14, 0xFFFFFFFF, { xOffset, yOffset }, std::format("^2Avg. {:.1f} ms, potential fps: {:.1f}", metrics.frameTimeMsMean, 1000.0f / metrics.frameTimeMsMean));
	ui->drawRasterText(e2::FontFace::Monospace, 14, 0xFFFFFFFF, { xOffset, yOffset + (18.0f * 1.0f) }, std::format("^3High {:.1f} ms, potential fps: {:.1f}", metrics.frameTimeMsHigh, 1000.0f / metrics.frameTimeMsHigh));
	ui->drawRasterText(e2::FontFace::Monospace, 14, 0xFFFFFFFF, { xOffset, yOffset + (18.0f * 2.0f) }, std::format("^4Real fps: {:.1f}", metrics.realCpuFps));
	ui->drawRasterText(e2::FontFace::Monospace, 14, 0xFFFFFFFF, { xOffset, yOffset + (18.0f * 3.0f) }, std::format("^5Num. chunks: {}", m_hexGrid->numChunks()));
	ui->drawRasterText(e2::FontFace::Monospace, 14, 0xFFFFFFFF, { xOffset, yOffset + (18.0f * 4.0f) }, std::format("^6Visible chunks: {}", m_hexGrid->numVisibleChunks()));
	ui->drawRasterText(e2::FontFace::Monospace, 14, 0xFFFFFFFF, { xOffset, yOffset + (18.0f * 5.0f) }, std::format("^7Entities: {}", m_entities.size()));
	ui->drawRasterText(e2::FontFace::Monospace, 14, 0xFFFFFFFF, { xOffset, yOffset + (18.0f * 6.0f) }, std::format("^8High loadtime: {:.2f}ms", m_hexGrid->highLoadTime()));
	ui->drawRasterText(e2::FontFace::Monospace, 14, 0xFFFFFFFF, { xOffset, yOffset + (18.0f * 7.0f) }, std::format("^9Jobs in flight: {}", m_hexGrid->numJobsInFlight()));
	ui->drawRasterText(e2::FontFace::Monospace, 14, 0xFFFFFFFF, { xOffset, yOffset + (18.0f * 8.0f) }, std::format("^2View Origin: {}", m_viewOrigin));
	ui->drawRasterText(e2::FontFace::Monospace, 14, 0xFFFFFFFF, { xOffset, yOffset + (18.0f * 9.0f) }, std::format("^3View Velocity: {}", m_viewVelocity));



	ui->drawTexturedQuad({ xOffset, yOffset + 18.0f * 11.0f }, { 384.f, 384.f }, 0xFFFFFFFF, renderer->shadowTarget());
#endif
}


void e2::Game::drawFinalUI()
{

	e2::GameSession* session = gameSession();
	e2::Renderer* renderer = session->renderer();
	e2::UIContext* ui = session->uiContext();
	auto& kb = ui->keyboardState();
	auto& mouse = ui->mouseState();
	auto& leftMouse = mouse.buttons[uint16_t(e2::MouseButton::Left)];

	e2::IWindow* wnd = session->window();
	glm::vec2 winSize = wnd->size();

	glm::vec2 btnSize(80.0f, 80.0f);
	float width = 368.0f;
	float height = 144.0f;
	float topA = 32.0f;
	float topB = 24.0f;
	float topHeight = 32.0f + 24.0f;
	float topBottomDelimiter = height - btnSize.y - topHeight;
	// 736 x 160

	glm::vec2 offset = { winSize.x - width - 16.0f, winSize.y - height - 16.0f };
	glm::vec2 cursor = offset;

	if (mouse.relativePosition.x > offset.x && mouse.relativePosition.x < offset.x + width &&
		mouse.relativePosition.y > offset.y && mouse.relativePosition.y < offset.y + height)
		m_uiHovered = true;

	e2::Sprite* citySprite = getUnitSprite("city");

	ui->drawQuad(cursor, { width, topA }, e2::gamePanelLight.withOpacity(0.75f));
	ui->drawQuad(cursor + glm::vec2(0.0f, topA), { width, topB }, e2::gamePanelDark.withOpacity(0.75f));

	float spriteScale = topA / citySprite->size.y;
	ui->drawSprite(cursor + glm::vec2(4.0f, 0.0f), *citySprite, 0xFFFFFFFF, spriteScale);
	ui->drawRasterText(e2::FontFace::Sans, 14, 0xFFFFFFFF, cursor + glm::vec2(4.0f, 0.0f) + glm::vec2(citySprite->size.x * spriteScale, topA / 2.0f), "City of **Mikkeli**");

	ui->drawRasterText(e2::FontFace::Sans, 10, 0xFFFFFFFF, cursor + glm::vec2(4.0f, topA + topB / 2.0f), "2 districts, researching **Diligence** (13 turns)");

	cursor.y += topHeight + topBottomDelimiter;

	glm::vec2 iconSize(44.f, 44.f);
	float iconOffsetY = 6.0f;
	float buttonPadding = 16.0f;
	float infoOpacity = 0.75f;


	auto drawBtn = [&](std::string const& textLabel, e2::Name spriteName) {
		bool hovered = (mouse.relativePosition.x > cursor.x && mouse.relativePosition.x < cursor.x + btnSize.x &&
			mouse.relativePosition.y > cursor.y && mouse.relativePosition.y < cursor.y + btnSize.y);


		e2::Sprite* sprite = getIconSprite(spriteName);
		ui->drawGamePanel(cursor, btnSize, hovered, infoOpacity);
		ui->drawFrame(cursor, btnSize, e2::UIColor(hovered ? e2::gameAccentHalf : e2::gamePanelDark).withOpacity(infoOpacity), 1.0f);
		ui->drawSprite(cursor + glm::vec2(btnSize.x / 2.0f - iconSize.x / 2.0f, iconOffsetY), *sprite, e2::UIColor(0xFFFFFFFF).withOpacity(infoOpacity), iconSize.y / sprite->size.y);
		float tw = ui->calculateTextWidth(FontFace::Sans, 14, textLabel);
		ui->drawRasterText(FontFace::Sans, 14, e2::UIColor(0xFFFFFFFF).withOpacity(infoOpacity), cursor + glm::vec2(btnSize.x / 2.0f - tw / 2.0f, 64.0f), textLabel);
		cursor.x += btnSize.x + buttonPadding;
	};

	drawBtn("*Details*", "info");
	drawBtn("*Research*", "research");
	drawBtn("*Upgrades*", "upgrade");
	drawBtn("*Economy*", "cash");


	/*
	ui->drawQuadShadow(offset, { width, height }, 8.0f, 0.9f, 4.0f);

	ui->pushFixedPanel("test", offset + glm::vec2(4.0f, 4.0f), glm::vec2(width - 8.0f, height - 8.0f));
	ui->beginStackH("test2", 40.0f);

		if (ui->gameGridButton("nextent", "gameUi.nextunit", "Next unit..", m_localTurnEntities.size() > 0))
			nextLocalEntity();
		
		if (ui->gameGridButton("endturn", "gameUi.endturn", "End turn", true))
			endTurn();

	ui->endStackH();
	ui->popFixedPanel();*/
}

void e2::Game::onNewCursorHex()
{
	if (m_selectedEntity && m_selectedEntity->specification->moveType != EntityMoveType::Static)
	{
		m_unitHoverPath = m_unitAS->find(m_cursorHex);
	}
}

void e2::Game::spawnHitLabel(glm::vec3 const& worldLocation, std::string const& text)
{
	m_hitLabels[m_hitLabelIndex++] = e2::HitLabel(worldLocation, text);
	if (m_hitLabelIndex >= e2::maxNumHitLabels)
		m_hitLabelIndex = 0;
}

e2::EmpireId e2::Game::spawnEmpire()
{
	for (e2::EmpireId i = 0; i < m_empires.size(); i++)
	{
		if (!m_empires[i])
		{
			m_empires[i] = e2::create<e2::GameEmpire>(this, i);
			//m_undiscoveredEmpires.insert(m_empires[i]);
			return i;
		}
	}

	LogError("failed to spawn empire, we full rn");
	return 0;
}

void e2::Game::destroyEmpire(EmpireId empireId)
{
	e2::GameEmpire* empire = m_empires[empireId];
	m_empires[empireId] = nullptr;

	if (!empire)
		return;

	//m_aiEmpires.erase(empire);
	//m_discoveredEmpires.erase(empire);
	//m_undiscoveredEmpires.erase(empire);

	if (empire->ai)
		e2::destroy(empire->ai);

	e2::destroy(empire);
}

//void e2::Game::spawnAIEmpire()
//{
//	e2::EmpireId newEmpireId = spawnEmpire();
//	if (newEmpireId == 0)
//		return;
//
//	e2::GameEmpire* newEmpire = m_empires[newEmpireId];
//	m_aiEmpires.insert(newEmpire);
//
//	newEmpire->ai = e2::create<e2::CommanderAI>(this, newEmpireId);
//
//	e2::Aabb2D worldBounds = m_hexGrid->worldBounds();
//	bool foundLocation = false;
//	glm::ivec2 aiStartLocation;
//	do
//	{
//		glm::vec2 random = e2::randomVec2(worldBounds.min, worldBounds.max);
//		float distLeft = glm::abs(worldBounds.min.x - random.x);
//		float distRight = glm::abs(worldBounds.max.x - random.x);
//		float distUp = glm::abs(worldBounds.min.y - random.y);
//		float distDown = glm::abs(worldBounds.max.y - random.y);
//		glm::vec2 offsetDir{};
//
//		if (distLeft < distRight && distLeft < distUp && distLeft < distDown)
//		{
//			random.x = worldBounds.min.x;
//			offsetDir = { -1.0f, 0.0f };
//		}
//		else if (distRight < distLeft && distRight < distUp && distRight < distDown)
//		{
//			random.x = worldBounds.max.x;
//			offsetDir = { 1.0f, 0.0f };
//		}
//		else if (distUp < distRight && distUp < distLeft && distUp < distDown)
//		{
//			random.y = worldBounds.min.y;
//			offsetDir = { 0.0f, -1.0f };
//		}
//		else
//		{
//			random.y = worldBounds.max.y;
//			offsetDir = { 0.0f, 1.0f };
//		}
//
//		glm::vec2 offsetPoint = random + offsetDir * 64.0f;
//
//		foundLocation = findStartLocation(e2::Hex(offsetPoint).offsetCoords(), { 64, 64 }, aiStartLocation, true);
//	} while (!foundLocation);
//
//	spawnEntity("mob", aiStartLocation, newEmpireId);
//}

e2::GameEmpire* e2::Game::empireById(EmpireId id)
{
	return m_empires[id];
}


void e2::Game::resolveLocalEntity()
{
	if (!m_selectedEntity || !m_selectedEntity->isLocal())
		return;

	resolveSelectedEntity();

	if (!m_selectedEntity)
	{
		//nextLocalEntity();
		return;
	}
	
	m_selectedEntity->updateGrugVariables();
	//if (!m_selectedEntity->playerRelevant())
	//{
	//	nextLocalEntity();
	//}
}

void e2::Game::nextLocalEntity()
{
	if (m_localTurnEntities.size() == 0)
		return;

	e2::GameEntity* entity = *m_localTurnEntities.begin();
	if (m_selectedEntity == entity)
	{
		deselectEntity();
		m_localTurnEntities.erase(entity);
	}

	if (m_localTurnEntities.size() == 0)
		return;

	entity = *m_localTurnEntities.begin();
	selectEntity(entity);
	m_viewOrigin = entity->meshPlanarCoords();
}


e2::Name e2::Game::getCityName()
{
	if (m_cityNames.size() == 0)
		return e2::Name("Nomansland");

	e2::Name returner = m_cityNames.back();
	m_cityNames.pop_back();

	return returner;
}

void e2::Game::applyDamage(e2::GameEntity* entity, e2::GameEntity* instigator, float damage)
{
	game()->spawnHitLabel(entity->meshPosition + e2::worldUpf() * 0.5f, std::format("{}", int64_t(damage)));
	entity->onHit(instigator, damage);
}

void e2::Game::resolveSelectedEntity()
{
	if (!m_selectedEntity)
		return;

	if (m_unitAS)
		e2::destroy(m_unitAS);

	m_unitAS = nullptr;

	m_unitAS = e2::create<PathFindingAS>(m_selectedEntity);

	if (!m_selectedEntity->isLocal() || m_selectedEntity->specification->moveType == EntityMoveType::Static)
	{
		m_hexGrid->clearOutline();
	}
	else
	{
		m_hexGrid->clearOutline();
		for (auto& [coords, hexAS] : m_unitAS->hexIndex)
		{
			m_hexGrid->pushOutline(e2::OutlineLayer::Movement, coords);
		}

		for (auto& [entity, pair] : m_unitAS->targetsInMoveRange)
		{
			m_hexGrid->pushOutline(e2::OutlineLayer::Attack, entity->tileIndex);
		}
	}

	if(!m_selectedEntity->isLocal() && m_selectedEntity->inView)
		m_viewOrigin = m_selectedEntity->meshPlanarCoords();

}

void e2::Game::unresolveSelectedEntity()
{

	if (m_unitAS)
		e2::destroy(m_unitAS);
	m_unitAS = nullptr;

	m_hexGrid->clearOutline();
}

e2::GameEntity* e2::Game::getSelectedEntity()
{
	return m_selectedEntity;
}

void e2::Game::selectEntity(e2::GameEntity* entity)
{
	if (!entity || entity == m_selectedEntity)
		return;

	if (!entityRelevantForPlay(entity))
		return;

	deselectEntity();

	m_selectedEntity = entity;

	resolveSelectedEntity();
	m_selectedEntity->specification->scriptInterface.invokeOnSelected(m_selectedEntity);
}

void e2::Game::deselectEntity()
{
	if (!m_selectedEntity)
		return;

	m_selectedEntity->specification->scriptInterface.invokeOnDeselected(m_selectedEntity);

	unresolveSelectedEntity();

	m_selectedEntity = nullptr;
}

void e2::Game::moveSelectedEntityTo(glm::ivec2 const& to)
{
	

	if ((m_state != GameState::Turn && m_turnState != TurnState::Unlocked) || !m_selectedEntity)
		return;

	if (m_selectedEntity->movePointsLeft < 1 || m_selectedEntity->specification->moveType == e2::EntityMoveType::Static)
		return;

	if (to == m_selectedEntity->tileIndex)
		return;

	m_unitMovePath = m_unitAS->find(to);
	if (m_unitMovePath.size() == 0)
		return;
	else if (m_unitMovePath.size() - 1 > m_selectedEntity->movePointsLeft)
		return;


	m_moveTurnStateFallback = m_turnState;

	m_ffwMove = !m_selectedEntity->isLocal() && !m_hexGrid->isVisible(to) && !m_hexGrid->isVisible(m_selectedEntity->tileIndex);

	m_hexGrid->clearOutline();

	m_selectedEntity->movePointsLeft -= int32_t(m_unitMovePath.size()) - 1;

	m_selectedEntity->onBeginMove();

	m_turnState = TurnState::UnitAction_Move;
	m_unitMoveIndex = 0;
	m_unitMoveDelta = 0.0f;
	
}

void e2::Game::beginCustomAction()
{
	if (m_turnState == TurnState::Unlocked)
		m_turnState = TurnState::EntityAction_Generic;
}


void e2::Game::endCustomAction()
{
	if (m_turnState == TurnState::EntityAction_Generic)
	{
		m_turnState = TurnState::Unlocked;
	}

}

void e2::Game::updateCustomAction()
{
	m_selectedEntity->updateCustomAction(m_timeDelta);
}

bool e2::Game::attemptBeginWave(e2::GameEntity* hiveEntity, std::string const& prefabPath)
{
	if (m_wave)
		return false;

	e2::GameEntity* closestEntity = nullptr;
	int32_t tileDist = 40;
	for (e2::GameEntity* entity : m_localEmpire->entities)
	{
		int32_t newDist = e2::Hex::distance(entity->tileIndex, hiveEntity->tileIndex);
		if (newDist > tileDist)
			continue;

		if (entity->specification->layerIndex != e2::EntityLayerIndex::Structure)
			continue;

		if (!entity->specification->waveRelevant)
			continue;

		closestEntity = entity;
	}

	if (!closestEntity)
		return false;
	
	m_wave = e2::create<e2::Wave>(this, hiveEntity, closestEntity);
	if (m_wave->path.size() == 0)
	{
		e2::destroy(m_wave);
		m_wave = nullptr;
		return false;
	}

	deselectEntity();

	m_wave->pushMobs(prefabPath);

	m_waveDeforestIndex = 0;
	m_waveDeforestTimer = 0.0f;

	m_turnState = TurnState::WavePreparing;

	for (e2::GameEntity* entity : m_waveEntities)
	{
		entity->onWavePreparing();
	}

	return true;
}

void e2::Game::beginTargeting()
{
	if (m_turnState == TurnState::Unlocked)
		m_turnState = TurnState::EntityAction_Target;
}

void e2::Game::endTargeting()
{
	if (m_turnState == TurnState::EntityAction_Target)
		m_turnState = TurnState::Unlocked;
}



void e2::Game::updateMainCamera(double seconds)
{
	if (m_shakeIntensity > 0.0f)
		m_shakeIntensity -= (float)seconds;
	else
		m_shakeIntensity = 0.0f;

	constexpr float moveSpeed = 25.0f;
	constexpr float viewSpeed = .3f;

	e2::GameSession* session = gameSession();
	e2::Renderer* renderer = session->renderer();
	e2::UIContext* ui = session->uiContext();
	auto& kb = ui->keyboardState();
	auto& mouse = ui->mouseState();
	auto& leftMouse = mouse.buttons[uint16_t(e2::MouseButton::Left)];

	m_targetViewZoom -= float(mouse.scrollOffset) * 0.1f;
	m_targetViewZoom = glm::clamp(m_targetViewZoom, 0.0f, 1.0f);
	m_viewZoom = glm::mix(m_viewZoom, m_targetViewZoom, glm::clamp(float(seconds)*16.0f, 0.0f, 1.0f));

	

	glm::vec2 oldOrigin = m_viewOrigin;

	// move main camera 
	if (kb.keys[int16_t(e2::Key::Left)].state)
	{
		m_viewOrigin.x -= moveSpeed * float(seconds);
	}
	if (kb.keys[int16_t(e2::Key::Right)].state)
	{
		m_viewOrigin.x += moveSpeed * float(seconds);
	}

	if (kb.keys[int16_t(e2::Key::Up)].state)
	{
		m_viewOrigin.y -= moveSpeed * float(seconds);
	}
	if (kb.keys[int16_t(e2::Key::Down)].state)
	{
		m_viewOrigin.y += moveSpeed * float(seconds);
	}

	if (kb.pressed(e2::Key::K))
	{
		m_viewOrigin = {};
	}

	if (leftMouse.pressed && !m_uiHovered)
	{
		m_dragView = calculateRenderView(m_viewDragOrigin);
		// save where in world we pressed
		m_cursorDragOrigin = m_dragView.unprojectWorldPlane(renderer->resolution(), m_cursorNdc, 0.0);
		m_viewDragOrigin = m_viewOrigin;
		m_viewDragging = true;
	}


	const float dragMultiplier = glm::mix(0.01f, 0.025f, m_viewZoom);
	if (leftMouse.state)
	{
		if (m_viewDragging)
		{
			glm::vec2 newDrag = m_dragView.unprojectWorldPlane(renderer->resolution(), m_cursorNdc, 0.0);
			glm::vec2 dragOffset = newDrag - m_cursorDragOrigin;

			m_viewOrigin = m_viewDragOrigin - dragOffset;
		}
	}
	else
	{
		m_viewDragging = false;
	}

	e2::Aabb2D viewBounds = m_hexGrid->viewBounds();
	m_viewOrigin.x = glm::clamp(m_viewOrigin.x, viewBounds.min.x, viewBounds.max.x);
	m_viewOrigin.y = glm::clamp(m_viewOrigin.y, viewBounds.min.y, viewBounds.max.y);

	m_viewVelocity = m_viewOrigin - oldOrigin;

	m_view = calculateRenderView(m_viewOrigin);
	m_viewPoints = e2::Viewpoints2D(renderer->resolution(), m_view, 0.0);
	if (!m_altView)
	{
		e2::RenderView shakyView = m_view;
		shakyView.origin += e2::randomInUnitSphere() * m_shakeIntensity * 0.5f;
		glm::dquat shakyRight = glm::angleAxis(e2::randomFloat(-1.f, 1.f) * m_shakeIntensity * glm::radians(4.0f), e2::worldRightf());
		glm::dquat shakyUp = glm::angleAxis(e2::randomFloat(-1.f, 1.f) * m_shakeIntensity * glm::radians(4.0f), e2::worldUpf());
		shakyView.orientation = shakyRight * shakyUp * shakyView.orientation;
		renderer->setView(shakyView);
	}
}


e2::RenderView e2::Game::calculateRenderView(glm::vec2 const &viewOrigin)
{
	float viewFov = glm::mix(55.0f, 35.0f, m_viewZoom);
	float viewAngle = glm::mix(35.5f, 50.0f, m_viewZoom);
	float viewDistance = glm::mix(2.5f, 30.0f, m_viewZoom);

	glm::quat orientation = glm::rotate(glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::radians(viewAngle), { 1.0f, 0.0f, 0.0f });

	e2::RenderView newView{};
	newView.fov = viewFov;
	newView.clipPlane = { 0.1f, 100.0f };
	newView.origin = glm::vec3(viewOrigin.x, 0.0f, viewOrigin.y) + (orientation * glm::vec3(e2::worldForward()) * -viewDistance);
	newView.orientation = orientation;

	return newView;
}



void e2::Game::destroyEntity(e2::GameEntity* entity)
{
	if(entity->isLocal())
		entity->rollbackVisibility();

	if (m_selectedEntity && m_selectedEntity == entity)
	{
		if (m_turnState == TurnState::UnitAction_Move)
		{
			LogError("We are moving while being destroyed. We can't handle this! Fix!!");
		}

		m_turnState = TurnState::Unlocked;


		deselectEntity();
	}

	m_localTurnEntities.erase(entity);
	m_entityLayers[size_t(entity->specification->layerIndex)].entityIndex.erase(entity->tileIndex);
	m_waveEntities.erase(entity);
	m_entities.erase(entity);

	if (m_empires[entity->empireId])
	{
		m_empires[entity->empireId]->entities.erase(entity);

		m_empires[entity->empireId]->resources.fiscalStreams.erase(entity);
	}

	e2::destroy(entity);

	resolveSelectedEntity();
}



void e2::Game::queueDestroyEntity(e2::GameEntity* entity)
{
	m_entitiesPendingDestroy.insert(entity);
}


e2::GameEntity* e2::Game::entityAtHex(e2::EntityLayerIndex layerIndex, glm::ivec2 const& hex)
{
	auto finder = m_entityLayers[size_t(layerIndex)].entityIndex.find(hex);
	if (finder == m_entityLayers[size_t(layerIndex)].entityIndex.end())
		return nullptr;

	return finder->second;
}

void e2::Game::killEntity(e2::GameEntity* entity)
{
	if (m_dyingEntities.contains(entity))
	{
		return;
	}

	if (game()->getSelectedEntity() == entity)
	{
		game()->deselectEntity();
	}

	entity->health = 0.0f;
	entity->playAction("die");

	m_dyingEntities.insert(entity);
}

int32_t e2::Game::grugNumAttackMovePoints()
{
	if (!m_selectedEntity || !m_unitAS)
		return 0;

	return m_unitAS->grugTargetMovePoints;
}

e2::GameEntity* e2::Game::grugAttackTarget()
{
	if (!m_selectedEntity || !m_unitAS)
		return nullptr;

	return m_unitAS->grugTarget;
}

glm::ivec2 e2::Game::grugAttackMoveLocation()
{
	if (!m_selectedEntity || !m_unitAS)
		return {};

	return m_unitAS->grugTargetMoveHex.offsetCoords();
}

glm::ivec2 e2::Game::grugMoveLocation()
{
	if (!m_selectedEntity || !m_unitAS)
		return {};

	if (m_unitAS->hexIndex.size() == 0)
		return m_selectedEntity->tileIndex;

	int64_t index = e2::randomInt(0, m_unitAS->hexIndex.size() - 1);
	auto it =m_unitAS->hexIndex.begin();
	for (int64_t i = 0; i < index; i++)
		it++;

	return it->first;
}

bool e2::Game::entityRelevantForPlay(e2::GameEntity* entity)
{
	bool entityInvalid = (!entity || entity->health <= 0.0f || m_dyingEntities.contains(entity) || m_entitiesPendingDestroy.contains(entity));
	return !entityInvalid;
}

e2::Wave* e2::Game::wave()
{
	return m_wave;
}

void e2::Game::onMobSpawned(e2::Mob* mob)
{
	for (e2::GameEntity* entity : m_waveEntities)
	{
		entity->onMobSpawned(mob);
	}
}

void e2::Game::onMobDestroyed(e2::Mob* mob)
{
	for (e2::GameEntity* entity : m_waveEntities)
	{
		entity->onMobDestroyed(mob);
	}
}

e2::Sprite* e2::Game::getUiSprite(e2::Name name)
{
	return m_uiIconsSheet->getSprite(name);
}

e2::Sprite* e2::Game::getIconSprite(e2::Name name)
{
	return m_uiIconsSheet2->getSprite(name);
}

e2::Sprite* e2::Game::getUnitSprite(e2::Name name)
{
	return m_uiUnitsSheet->getSprite(name);
}


void e2::Game::harvestWood(glm::ivec2 const& location, EmpireId empire)
{
	e2::TileData* tileData = m_hexGrid->getExistingTileData(location);
	if (!tileData)
		return;

	bool hasForest = (tileData->flags & e2::TileFlags::FeatureForest) != TileFlags::FeatureNone;
	float woodProfit = tileData->getWoodAbundanceAsFloat() * 14.0f;

	if (!hasForest)
		return;

	m_empires[empire]->resources.funds.wood += woodProfit;
	tileData->flags = e2::TileFlags(uint16_t(tileData->flags) & (~uint16_t(e2::TileFlags::FeatureForest)));

	if (tileData->forestProxy)
		e2::destroy(tileData->forestProxy);

	tileData->forestProxy = nullptr;

}

void e2::Game::removeWood(glm::ivec2 const& location)
{
	e2::TileData* tileData = m_hexGrid->getExistingTileData(location);
	if (!tileData)
		return;

	bool hasForest = (tileData->flags & e2::TileFlags::FeatureForest) != TileFlags::FeatureNone;
	if (!hasForest)
		return;

	tileData->flags = e2::TileFlags(uint16_t(tileData->flags) & (~uint16_t(e2::TileFlags::FeatureForest)));

	if (tileData->forestProxy)
		e2::destroy(tileData->forestProxy);

	tileData->forestProxy = nullptr;

	e2::ChunkState* state = m_hexGrid->getOrCreateChunk(location);
	m_hexGrid->refreshChunkMeshes(state);
}

void e2::Game::removeResource(glm::ivec2 const& location)
{
	e2::TileData* tileData = m_hexGrid->getExistingTileData(location);
	if (!tileData)
		return;

	bool hasResource = (tileData->flags & e2::TileFlags::ResourceMask) != TileFlags::ResourceNone;
	if (!hasResource)
		return;

	tileData->flags = e2::TileFlags(uint16_t(tileData->flags) & (~uint16_t(e2::TileFlags::ResourceMask)));

	if (tileData->resourceProxy)
		e2::destroy(tileData->resourceProxy);

	tileData->resourceProxy = nullptr;
	tileData->resourceMesh = nullptr;
}
//
//void e2::Game::discoverEmpire(EmpireId empireId)
//{
//	// @todo optimize
//	m_discoveredEmpires.insert(m_empires[empireId]);
//	m_undiscoveredEmpires.erase(m_empires[empireId]);
//}

void e2::Game::updateAltCamera(double seconds)
{
	constexpr float moveSpeed = 10.0f;
	constexpr float viewSpeed = .3f;

	e2::GameSession* session = gameSession();
	e2::Renderer* renderer = session->renderer();
	e2::UIContext* ui = session->uiContext();
	auto& kb = ui->keyboardState();
	auto& mouse = ui->mouseState();


	if (m_altView)
	{

		if (mouse.moved)
		{
			m_altViewYaw += mouse.moveDelta.x * viewSpeed;
			m_altViewPitch -= mouse.moveDelta.y * viewSpeed;
			m_altViewPitch = glm::clamp(m_altViewPitch, -89.f, 89.f);
		}

		glm::quat altRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		altRotation = glm::rotate(altRotation, glm::radians(m_altViewYaw), glm::vec3(e2::worldUp()));
		altRotation = glm::rotate(altRotation, glm::radians(m_altViewPitch), glm::vec3(e2::worldRight()));

		if (kb.keys[int16_t(e2::Key::Space)].state)
		{
			m_altViewOrigin -= glm::vec3(e2::worldUp()) * moveSpeed * float(seconds);
		}
		if (kb.keys[int16_t(e2::Key::LeftControl)].state)
		{
			m_altViewOrigin -= -glm::vec3(e2::worldUp()) * moveSpeed * float(seconds);
		}

		if (kb.keys[int16_t(e2::Key::A)].state)
		{
			m_altViewOrigin -= altRotation * -glm::vec3(e2::worldRight()) * moveSpeed * float(seconds);
		}
		if (kb.keys[int16_t(e2::Key::D)].state)
		{
			m_altViewOrigin -= altRotation * glm::vec3(e2::worldRight()) * moveSpeed * float(seconds);
		}

		if (kb.keys[int16_t(e2::Key::W)].state)
		{
			m_altViewOrigin -= altRotation * glm::vec3(e2::worldForward()) * moveSpeed * float(seconds);
		}
		if (kb.keys[int16_t(e2::Key::S)].state)
		{
			m_altViewOrigin -= altRotation * -glm::vec3(e2::worldForward()) * moveSpeed * float(seconds);
		}

		altRotation = glm::rotate(altRotation, glm::radians(180.0f), glm::vec3(e2::worldUp()));

		e2::RenderView newView{};
		newView.fov = 65.0;
		newView.clipPlane = { 1.0, 2000.0 };
		newView.origin = m_altViewOrigin;
		newView.orientation = altRotation;
		renderer->setView(newView);
	}

}


void e2::Game::updateAnimation(double seconds)
{
	// 1 / 30 = 33.333ms = 30 fps 
	constexpr double targetFrameTime = 1.0 / 60.0;
	m_accumulatedAnimationTime += seconds;

	int32_t numTicks = 0;
	while (m_accumulatedAnimationTime > targetFrameTime)
	{
		numTicks++;
		m_accumulatedAnimationTime -= targetFrameTime;
	}

	if (numTicks < 1)
		return;

	e2::Aabb2D viewAabb = m_viewPoints.toAabb();


	m_entitiesInView.clear();
	for (e2::GameEntity* entity : m_entities)
	{
		// update inView, first check aabb to give chance for implicit early exit
		glm::vec2 unitCoords = entity->meshPlanarCoords();
		bool entityRelevant = (m_turnState == e2::TurnState::Wave && entity->specification->waveRelevant) || m_turnState != e2::TurnState::Wave;
		entity->inView = viewAabb.isWithin(unitCoords) && m_viewPoints.isWithin(unitCoords, 1.0f) && entityRelevant;

		if (entity->inView)
		{
			m_entitiesInView.insert(entity);
		}

		// @todo consider using this instead if things break
		//for(int32_t i = 0; i < numTicks; i++)
		//	unit->updateAnimation(targetFrameTime);

		entity->updateAnimation(targetFrameTime * double(numTicks));
	}


}

e2::PathFindingAS::PathFindingAS(e2::GameEntity* entity)
{
	if (!entity)
		return;

	e2::Game* game = entity->game();
	e2::HexGrid* grid = game->hexGrid();

	e2::Hex originHex = e2::Hex(entity->tileIndex);
	origin = e2::create<e2::PathFindingHex>(originHex);
	origin->isBegin = true;

	// find all hexes in attack range from origin hex, add them to targetsInRange and set grugTargets for each hex 
	e2::GameEntity* lowestHealthEntity = nullptr;
	float lowestHealth = std::numeric_limits<float>::max();
	tmpHex.clear();
	e2::Hex::circle(originHex, entity->specification->attackRange, tmpHex);
	for (e2::Hex& attackHex : tmpHex)
	{
		e2::GameEntity* potentialTarget = game->entityAtHex(e2::EntityLayerIndex::Unit, attackHex.offsetCoords());

		if (potentialTarget)
		{
			if (potentialTarget->empireId == entity->empireId)
			{
				continue;
			}

			targetsInRange.insert(potentialTarget);

			if (potentialTarget->health < lowestHealth)
			{
				lowestHealthEntity = potentialTarget;
				lowestHealth = potentialTarget->health;
			}
		}
	}

	if (lowestHealthEntity)
	{
		origin->grugTarget = lowestHealthEntity;

		grugTarget = lowestHealthEntity;
		grugTargetMoveHex = originHex;
		grugTargetMovePoints = 0;
	}


	hexIndex[entity->tileIndex] = origin;


	std::queue<e2::PathFindingHex*> queue;
	std::unordered_set<e2::Hex> processed;

	queue.push(origin);

	while (!queue.empty())
	{
		e2::PathFindingHex* currHexEntry  = queue.front();
		queue.pop();

		for (e2::Hex nextHex : currHexEntry->index.neighbours())
		{
			if (int32_t(currHexEntry->stepsFromOrigin + 1) > entity->movePointsLeft)
				continue;

			if (processed.contains(nextHex))
				continue;

			glm::ivec2 coords = nextHex.offsetCoords();

			if (hexIndex.contains(coords))
				continue;

			// ignore hexes not directly visible, but only if we are lcoal
			if (entity->isLocal() && !grid->isVisible(coords))
				continue;

			// ignore hexes that are occupied by unpassable biome
			e2::TileData tile = grid->getCalculatedTileData(coords);
			if (!tile.isPassable(entity->specification->passableFlags))
				continue;

			// ignore hexes that are occupied by units
			e2::GameEntity* otherUnit = game->entityAtHex(e2::EntityLayerIndex::Unit, coords);
			if (otherUnit)
				continue;

			e2::PathFindingHex* nextHexEntry = e2::create<e2::PathFindingHex>(nextHex);
			nextHexEntry->towardsOrigin = currHexEntry;
			nextHexEntry->stepsFromOrigin = currHexEntry->stepsFromOrigin + 1;



			lowestHealthEntity = nullptr;
			lowestHealth = std::numeric_limits<float>::max();
			tmpHex.clear();
			e2::Hex::circle(nextHex, entity->specification->attackRange, tmpHex);
			for (e2::Hex& attackHex : tmpHex)
			{
				if (attackHex == originHex)
					continue;

				e2::GameEntity* potentialTarget = game->entityAtHex(e2::EntityLayerIndex::Unit, attackHex.offsetCoords());

				if (potentialTarget)
				{

					if (potentialTarget->empireId == game->localEmpireId())
					{
						continue;
					}

					auto finder = targetsInMoveRange.find(potentialTarget);
					if (finder != targetsInMoveRange.end())
					{
						uint32_t currStepsFromOrigin = finder->second.second;
						if (nextHexEntry->stepsFromOrigin < currStepsFromOrigin)
						{
							targetsInMoveRange[potentialTarget] = { nextHex, nextHexEntry->stepsFromOrigin };
						}
					}
					else
					{
						targetsInMoveRange[potentialTarget] = { nextHex, nextHexEntry->stepsFromOrigin };
					}


					if (!grugTarget || grugTargetMovePoints > nextHexEntry->stepsFromOrigin)
					{
						grugTarget = potentialTarget;
						grugTargetMovePoints = nextHexEntry->stepsFromOrigin;
						grugTargetMoveHex = nextHex;
					}

					if (potentialTarget->health < lowestHealth)
					{
						lowestHealthEntity = potentialTarget;
						lowestHealth = potentialTarget->health;
					}
				}
			}

			if (lowestHealthEntity)
			{
				nextHexEntry->grugTarget = lowestHealthEntity;
			}

			hexIndex[coords] = nextHexEntry;
			
			grugCanMove = true;

			queue.push(nextHexEntry);
		}

		processed.insert(currHexEntry->index);
	}
}
//
//e2::PathFindingAS::PathFindingAS(e2::GameEntity* entity, glm::ivec2 const& target)
//{
//	if (!entity)
//		return;
//
//	if (!entity->isLocal())
//	{
//		LogError("passed entity wasn't local; this constructor is only meant for local players!");
//		return;
//	}
//
//	if (e2::Hex::distance(entity->tileIndex, target) > e2::maxAutoMoveRange)
//	{
//		LogError("max range overflow");
//		return;
//	}
//
//	e2::Game* game = entity->game();
//	e2::HexGrid* grid = game->hexGrid();
//
//	e2::Hex originHex = e2::Hex(entity->tileIndex);
//	origin = e2::create<e2::PathFindingHex>(originHex);
//	origin->isBegin = true;
//
//	hexIndex[entity->tileIndex] = origin;
//
//
//	std::queue<e2::PathFindingHex*> queue;
//	std::unordered_set<e2::Hex> processed;
//
//	queue.push(origin);
//
//	while (!queue.empty())
//	{
//		e2::PathFindingHex* currHexEntry = queue.front();
//		queue.pop();
//
//		for (e2::Hex nextHex : currHexEntry->index.neighbours())
//		{
//			
//			if (processed.contains(nextHex))
//				continue;
//
//			glm::ivec2 coords = nextHex.offsetCoords();
//
//			if (hexIndex.contains(coords))
//				continue;
//
//			if (currHexEntry->stepsFromOrigin + 1 > e2::maxAutoMoveRange)
//				continue;
//
//			// check movepoints 
//			if (currHexEntry->stepsFromOrigin + 1 > entity->movePointsLeft)
//			{
//				// we are out of move bounds, do cheeky version 
//				e2::TileData* existingTile = grid->getExistingTileData(coords);
//				if (existingTile && !existingTile->isPassable(entity->specification->passableFlags))
//					continue;
//
//				// ignore hexes that are occupied by units (but only if they are visible)
//				e2::GameEntity* otherUnit = game->entityAtHex(e2::EntityLayerIndex::Unit, coords);
//				if (grid->isVisible(coords) && otherUnit)
//					continue;
//
//				e2::PathFindingHex* nextHexEntry = e2::create<e2::PathFindingHex>(nextHex);
//				nextHexEntry->towardsOrigin = currHexEntry;
//				nextHexEntry->stepsFromOrigin = currHexEntry->stepsFromOrigin + 1;
//				nextHexEntry->instantlyReachable = false;
//				hexIndex[coords] = nextHexEntry;
//				queue.push(nextHexEntry);
//			}
//			else
//			{
//				// we are within move bounds, do proper version 
//				if (!grid->isVisible(coords))
//					continue;
//
//				// ignore hexes that are occupied by unpassable biome
//				e2::TileData tile = grid->getCalculatedTileData(coords);
//				if (!tile.isPassable(entity->specification->passableFlags))
//					continue;
//
//				// ignore hexes that are occupied by units
//				e2::GameEntity* otherUnit = game->entityAtHex(e2::EntityLayerIndex::Unit, coords);
//				if (otherUnit)
//					continue;
//
//				e2::PathFindingHex* nextHexEntry = e2::create<e2::PathFindingHex>(nextHex);
//				nextHexEntry->towardsOrigin = currHexEntry;
//				nextHexEntry->stepsFromOrigin = currHexEntry->stepsFromOrigin + 1;
//				nextHexEntry->instantlyReachable = true;
//				hexIndex[coords] = nextHexEntry;
//				queue.push(nextHexEntry);
//			}
//
//
//		}
//
//		processed.insert(currHexEntry->index);
//	}
//}

e2::PathFindingAS::PathFindingAS(e2::Game* game, e2::Hex const& start, uint64_t range, bool ignoreVisibility, e2::PassableFlags passableFlags, bool onlyWaveRelevant, e2::Hex* stopWhenFound)
{
	e2::HexGrid* grid = game->hexGrid();
	glm::ivec2 tileIndex = start.offsetCoords();

	e2::Hex originHex = start;
	origin = e2::create<e2::PathFindingHex>(originHex);
	origin->isBegin = true;

	hexIndex[tileIndex] = origin;


	std::queue<e2::PathFindingHex*> queue;
	std::unordered_set<e2::Hex> processed;

	queue.push(origin);

	while (!queue.empty())
	{
		e2::PathFindingHex* curr = queue.front();
		queue.pop();

		for (e2::Hex n : curr->index.neighbours())
		{
			if (curr->stepsFromOrigin + 1 > range)
				continue;

			if (processed.contains(n))
				continue;

			glm::ivec2 coords = n.offsetCoords();

			if (hexIndex.contains(coords))
				continue;

			// ignore hexes not directly visible
			if (!ignoreVisibility && !grid->isVisible(coords))
				continue;

			// ignore hexes that are occupied by unpassable biome
			e2::TileData tile = game->hexGrid()->getCalculatedTileData(coords);
			if (!tile.isPassable(passableFlags))
				continue;

			// ignore hexes that are occupied by units
			e2::GameEntity* otherUnit = game->entityAtHex(e2::EntityLayerIndex::Unit, coords);
			if (!onlyWaveRelevant && otherUnit)
				continue;

			if (onlyWaveRelevant && otherUnit && otherUnit->specification->waveRelevant)
				continue;

			e2::PathFindingHex* newHex = e2::create<e2::PathFindingHex>(n);
			newHex->towardsOrigin = curr;
			newHex->stepsFromOrigin = curr->stepsFromOrigin + 1;
			hexIndex[coords] = newHex;


			queue.push(newHex);
		}

		processed.insert(curr->index);

		if (stopWhenFound && *stopWhenFound == curr->index)
			return;
	}
}

e2::PathFindingAS::~PathFindingAS()
{
	std::unordered_set<e2::PathFindingHex*> hexes;
	for (auto [coords, hex] : hexIndex)
		hexes.insert(hex);
	hexIndex.clear(); 

	while (!hexes.empty())
	{
		e2::PathFindingHex* h = *hexes.begin();
		hexes.erase(h);
		e2::destroy(h);
	}
		
}

e2::PathFindingAS::PathFindingAS()
{

}

std::vector<e2::Hex> e2::PathFindingAS::find(e2::Hex const& target)
{

	auto targetFinder = hexIndex.find(target.offsetCoords());
	if (targetFinder == hexIndex.end())
		return {};


	std::vector<e2::Hex> returner;
	e2::PathFindingHex* cursor = targetFinder->second;
	while (cursor)
	{
		returner.push_back(cursor->index);
		if (cursor->isBegin)
			break;
		cursor = cursor->towardsOrigin;
	}

	std::reverse(returner.begin(), returner.end());

	return returner;

}

e2::PathFindingHex::PathFindingHex(e2::Hex const& _index)
	: index(_index)
{

}

e2::PathFindingHex::~PathFindingHex()
{

}

std::string e2::SaveMeta::displayName()
{
	if (!exists)
		return std::format("{}: empty", uint32_t(slot+1));

	char timeString[std::size("hh:mm:ss yyyy-mm-dd")];

	std::strftime(std::data(timeString), std::size(timeString), "%T %F", (std::gmtime)(&timestamp));

	return std::format("{}: {}", uint32_t(slot+1), std::string(timeString));

}

std::string e2::SaveMeta::fileName()
{
	PWSTR saveGameFolder{};
	if (SHGetKnownFolderPath(FOLDERID_SavedGames, KF_FLAG_DEFAULT, nullptr, &saveGameFolder) != S_OK)
	{
		LogError("SHGetKnownFolderPath failed, returning backup folder");
		return std::filesystem::path(std::format("./saves/{}.sav", uint32_t(slot))).string();
	}

	std::wstring wstr = saveGameFolder;
	CoTaskMemFree(saveGameFolder);

	std::string str;
	std::transform(wstr.begin(), wstr.end(), std::back_inserter(str), [](wchar_t c) {
		return (char)c;
	});

	return std::filesystem::path(std::format("{}/reveal_and_annihilate/{}.sav",str, uint32_t(slot))).string();
}

e2::HitLabel::HitLabel(glm::vec3 const& worldOffset, std::string const& inText) : offset(worldOffset)
, text(inText)
{
	active = true;
	timeCreated = e2::timeNow();
	velocity = glm::vec3{ 50.0f, -200.0f, 50.0f } * 0.02f;
}