
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

	glm::vec3 colors[4] = {
	{1.0, 0.0, 0.0},
	{0.0, 1.0, 0.0},
	{0.0, 0.0, 1.0},
	{1.0, 1.0, 1.0},
	};

	auto renderer = game()->gameSession()->renderer();
	auto skeleton = m_skinProxy->skeletonAsset;

	glm::mat4 modelMatrix = m_proxy->modelMatrix;

	for (uint32_t b = 0; b < m_skinProxy->skeletonAsset->numBones(); b++)
	{
		e2::Bone* bone = m_skinProxy->skeletonAsset->boneById(b);
		e2::PoseBone* poseBone = m_testPose->poseBoneById(b);
		e2::Bone* parentBone = bone->parent;
		e2::PoseBone* parentPoseBone = parentBone ? m_testPose->poseBoneById(parentBone->index) : nullptr;


		glm::mat4 boneTransform = modelMatrix * poseBone->cachedGlobalTransform;
		glm::vec3 boneOrigin = boneTransform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		
		glm::vec3 boneRight = boneTransform * glm::vec4(e2::worldRight(), 0.0f);
		glm::vec3 boneUp = boneTransform * glm::vec4(e2::worldUp(), 0.0f);
		glm::vec3 boneForward = boneTransform * glm::vec4(e2::worldForward(), 0.0f);

		
		renderer->debugLine(colors[0], boneOrigin, boneOrigin + boneRight * 0.05f);
		renderer->debugLine(colors[1], boneOrigin, boneOrigin + boneUp * 0.05f);
		renderer->debugLine(colors[2], boneOrigin, boneOrigin + boneForward * 0.05f);


		if (parentBone)
		{
			glm::mat4 parentTransform = modelMatrix * parentPoseBone->cachedGlobalTransform;
			glm::vec3 parentOrigin = parentTransform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
			renderer->debugLine(colors[3], parentOrigin, boneOrigin);
		}

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


	auto renderer = game()->gameSession()->renderer();
	auto skeleton = m_skinProxy->skeletonAsset;


	glm::mat4 modelMatrix = m_proxy->modelMatrix;

	glm::dmat4 vpMatrix = game()->view().calculateProjectionMatrix(renderer->resolution()) * game()->view().calculateViewMatrix();

	for(uint32_t b = 0; b < m_skinProxy->skeletonAsset->numBones(); b++)
	{
		e2::Bone* bone = m_skinProxy->skeletonAsset->boneById(b);
		e2::PoseBone* poseBone = m_testPose->poseBoneById(b);
		e2::Bone* parentBone = bone->parent;
		e2::PoseBone* parentPoseBone = parentBone? m_testPose->poseBoneById(parentBone->index) : nullptr;


		glm::mat4 boneTransform = modelMatrix * poseBone->cachedGlobalTransform;

		glm::vec3 boneOrigin = boneTransform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		glm::vec3 boneDirection = boneTransform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

		glm::vec4 viewPos = vpMatrix * glm::dvec4(boneOrigin, 1.0);
		viewPos = viewPos / viewPos.z;

		glm::vec2 offset = (glm::vec2(viewPos.x, viewPos.y) * 0.5f + 0.5f) * glm::vec2(renderer->resolution());
		std::string str = std::format("[{}]", bone->name.cstring());
		float wid = ui->calculateTextWidth(FontFace::Monospace, 9, str);

		ui->drawQuad(offset + glm::vec2(-wid / 2.0f, -6.0f), glm::vec2(wid, 12.0f), 0x000000AA);
		ui->drawRasterText(FontFace::Monospace, 9, 0xFFFFFFFF, offset + glm::vec2(-wid / 2.0f, 0.0f), str);
	}


}
