
#include "game/builderunit.hpp"
#include "game/game.hpp"
#include <e2/game/gamesession.hpp>
#include <e2/transform.hpp>
#include <e2/renderer/renderer.hpp>

e2::Engineer::Engineer(e2::GameContext* ctx, glm::ivec2 const& tile, uint8_t empire)
	: e2::GameUnit(ctx, tile, empire)

{
	displayName = "Engineer";
	sightRange = 5;
	moveRange = 8;
	movePointsLeft = 8;

	unitType = e2::GameUnitType::Engineer;
	m_mesh = game()->getUnitMesh(unitType);
	buildProxy();

	m_testAnim = game()->dummyAnimation();

	e2::SkinProxyConfiguration conf;
	conf.mesh = m_mesh;
	conf.skeleton = game()->dummySkeleton();
	m_skinProxy = e2::create<e2::SkinProxy>(game()->gameSession(), conf);
	m_proxy->skinProxy = m_skinProxy;

	m_testPose = e2::create<e2::Pose>(game()->dummySkeleton());
	m_proxy->invalidatePipeline();
	
}

e2::Engineer::~Engineer()
{
	e2::destroy(m_testPose);
	e2::destroy(m_skinProxy);
}

void e2::Engineer::updateAnimation(double seconds)
{
	e2::GameUnit::updateAnimation(seconds);

	if(m_play)
		m_testAnimTime += float(seconds) * m_playRate;

	while (m_testAnimTime > m_testAnim->timeSeconds() )
	{
		m_testAnimTime -= m_testAnim->timeSeconds();
	}

	m_testPose->applyBindPose();
	
	if(m_applyAnim)
		m_testPose->applyAnimation(m_testAnim, m_testAnimTime);

	m_testPose->updateSkin();


	glm::vec3 colors[3] = {
	{1.0, 0.0, 0.0},
	{0.0, 1.0, 0.0},
	{0.0, 0.0, 1.0}
	};

	auto renderer = game()->gameSession()->renderer();
	auto skeleton = m_skinProxy->skeletonAsset;

	struct WorkUnit
	{
		WorkUnit(e2::Bone* a, glm::mat4 const& b)
			: bone(a)
			, transform(b)
		{}
		e2::Bone* bone{};
		glm::mat4 transform;
	};

	std::queue<WorkUnit> queue;

	glm::mat4 transform = m_proxy->modelMatrix;

	for (uint32_t rootIndex = 0; rootIndex < skeleton->numRootBones(); rootIndex++)
	{
		e2::Bone* rootBone = skeleton->rootBoneById(rootIndex);
		queue.push(WorkUnit(rootBone, transform));
	}
	uint32_t i = 0;
	while (!queue.empty())
	{
		WorkUnit unit = queue.front();
		queue.pop();

		glm::mat4 newTransform = unit.transform * m_testPose->localBoneTransform(unit.bone->index);// unit.bone->transform;

		glm::vec3 newOrigin = newTransform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

		glm::vec3 newDirForward = newTransform * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
		glm::vec3 tailForward = newOrigin + newDirForward * 0.1f;
		renderer->debugLine(colors[2], newOrigin, tailForward);

		glm::vec3 newDirRight = newTransform * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
		glm::vec3 tailRight = newOrigin + newDirRight * 0.05f;
		renderer->debugLine(colors[0], newOrigin, tailRight);

		glm::vec3 newDirUp = newTransform * glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);
		glm::vec3 tailUp = newOrigin + newDirUp * 0.05f;
		renderer->debugLine(colors[1], newOrigin, tailUp);

		for (e2::Bone* child : unit.bone->children)
			queue.push(WorkUnit(child, newTransform));
	}


	m_skinProxy->applyPose(m_testPose);
}

void e2::Engineer::drawUI(e2::UIContext* ui)
{

	e2::TileData* tileData = game()->hexGrid()->getTileData(tileIndex);
	if (!tileData)
		return;

	auto resStr = [](e2::TileFlags res) -> std::string {
		switch (res)
		{
		case e2::TileFlags::ResourceForest:
			return "Saw Mill";
			break;
		case e2::TileFlags::ResourceStone:
			return "Quarry";
			break;
		case e2::TileFlags::ResourceOre:
			return "Ore Mine";
			break;
		case e2::TileFlags::ResourceGold:
			return "Gold Mine";
			break;
		case e2::TileFlags::ResourceOil:
			return "Oil Well";
			break;
		case e2::TileFlags::ResourceUranium:
			return "Uranium Mine";
			break;
		}
		return "";
	};

	e2::TileFlags resource = tileData->getResource();

	ui->beginStackV("test2");

	ui->gameLabel(std::format("**{}**", displayName), 12, e2::UITextAlign::Middle);

	ui->gameLabel(std::format("Movement points left: {}", movePointsLeft), 11);

	ui->gameLabel(std::format("Animation Time: {:.2f}", m_testAnimTime));
	ui->sliderFloat("playRate", m_playRate, 0.0f, 2.0f);
	ui->checkbox("playAnim", m_play, "Play animation");
	ui->checkbox("applyAnim", m_applyAnim, "Apply animation");


	ui->beginStackH("actions", 32.0f);

	bool hasStructure = game()->structureAtHex(tileIndex);

	if (!tileData->improvedResource && !hasStructure && resource != TileFlags::ResourceNone)
	{
		if (ui->button("build", std::format("Build {}", resStr(resource))))
		{
			game()->spawnStructure<e2::Mine>(e2::Hex(tileIndex), empireId);
			game()->destroyUnit(e2::Hex(tileIndex));
		}
	}

	ui->endStackH();

	ui->endStackV();




	glm::vec3 colors[5] = {
	{1.0, 0.0, 0.0},
	{1.0, 1.0, 0.0},
	{0.0, 1.0, 0.0},
	{0.0, 1.0, 1.0},
	{0.0, 0.0, 1.0},
	};

	auto renderer = game()->gameSession()->renderer();
	auto skeleton = m_skinProxy->skeletonAsset;

	struct WorkUnit
	{
		WorkUnit(e2::Bone* a, glm::mat4 const& b)
			: bone(a)
			, transform(b)
		{}
		e2::Bone* bone{};
		glm::mat4 transform;
	};

	std::queue<WorkUnit> queue;

	glm::mat4 transform = m_proxy->modelMatrix;

	for (uint32_t rootIndex = 0; rootIndex < skeleton->numRootBones(); rootIndex++)
	{
		e2::Bone* rootBone = skeleton->rootBoneById(rootIndex);
		queue.push(WorkUnit(rootBone, transform));
	}
	uint32_t i = 0;

	glm::dmat4 vpMatrix = game()->view().calculateProjectionMatrix(renderer->resolution()) * game()->view().calculateViewMatrix();

	while (!queue.empty())
	{
		WorkUnit unit = queue.front();
		queue.pop();

		glm::mat4 oldTransform = unit.transform;
		glm::vec3 oldOrigin = oldTransform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

		glm::mat4 newTransform = oldTransform * m_testPose->localBoneTransform(unit.bone->index);// unit.bone->transform;
		glm::vec3 newOrigin = newTransform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);



		glm::vec4 viewPos = vpMatrix * glm::dvec4(newOrigin, 1.0);
		viewPos = viewPos / viewPos.z;

		glm::vec2 offset = (glm::vec2(viewPos.x, viewPos.y) * 0.5f + 0.5f) * glm::vec2(renderer->resolution());
		std::string str = std::format("[{}]", unit.bone->name.cstring());
		float wid = ui->calculateTextWidth(FontFace::Monospace, 9, str);

		ui->drawQuad(offset + glm::vec2(-wid / 2.0f, -6.0f), glm::vec2(wid, 12.0f), 0x000000AA);
		ui->drawRasterText(FontFace::Monospace, 9, 0xFFFFFFFF, offset + glm::vec2(-wid / 2.0f, 0.0f), str);

		for (e2::Bone* child : unit.bone->children)
			queue.push(WorkUnit(child, newTransform));
	}


}
