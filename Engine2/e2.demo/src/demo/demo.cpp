
#include "demo/demo.hpp"

#include "e2/e2.hpp"
#include "e2/managers/rendermanager.hpp"
#include "e2/managers/typemanager.hpp"
#include "e2/managers/assetmanager.hpp"
#include "e2/assets/mesh.hpp"
#include "e2/game/gamesession.hpp"
#include "e2/transform.hpp"
#include "e2/dmesh/dmesh.hpp"


#include "init.inl"

e2::Demo::Demo(e2::Context* ctx)
	: e2::Application(ctx)
{

}

e2::Demo::~Demo()
{

}

void e2::Demo::initialize()
{
	e2::ALJDescription alj;
	assetManager()->prescribeALJ(alj, "assets/demo/SM_DemoFloor.e2a");
	assetManager()->prescribeALJ(alj, "assets/hdri/courtyard_irr.e2a");
	assetManager()->prescribeALJ(alj, "assets/hdri/courtyard_rad.e2a");
	assetManager()->prescribeALJ(alj, "assets/hdri/sunset_irr.e2a");
	assetManager()->prescribeALJ(alj, "assets/hdri/sunset_rad.e2a");
	assetManager()->prescribeALJ(alj, "assets/kloofendal_irr.e2a");
	assetManager()->prescribeALJ(alj, "assets/kloofendal_rad.e2a");
	assetManager()->prescribeALJ(alj, "assets/kloofensky_irr.e2a");
	assetManager()->prescribeALJ(alj, "assets/kloofensky_rad.e2a");
	assetManager()->prescribeALJ(alj, "assets/towers/nimble/SM_Nimble_LOD0.e2a");
	assetManager()->prescribeALJ(alj, "assets/towers/nimble/SK_Nimble_LOD0.e2a");
	assetManager()->prescribeALJ(alj, "assets/towers/nimble/SK_NimbleIdle.e2a");
	
	assetManager()->queueWaitALJ(alj);
	e2::MeshPtr demoFloorMesh = assetManager()->get("assets/demo/SM_DemoFloor.e2a").cast<e2::Mesh>();

	nimbleMeshAsset = assetManager()->get("assets/towers/nimble/SM_Nimble_LOD0.e2a").cast<e2::Mesh>();
	nimbleSkeletonAsset = assetManager()->get("assets/towers/nimble/SK_Nimble_LOD0.e2a").cast<e2::Skeleton>();
	nimbleAnimationAsset = assetManager()->get("assets/towers/nimble/SK_NimbleIdle.e2a").cast<e2::Animation>();

	EnvMap map;
	
	map.irradiance = assetManager()->get("assets/hdri/sunset_irr.e2a").cast<e2::Texture2D>();
	map.radiance = assetManager()->get("assets/hdri/sunset_rad.e2a").cast<e2::Texture2D>();
	envMaps.push(map);

	map.irradiance = assetManager()->get("assets/hdri/courtyard_irr.e2a").cast<e2::Texture2D>();
	map.radiance = assetManager()->get("assets/hdri/courtyard_rad.e2a").cast<e2::Texture2D>();
	envMaps.push(map);

	map.irradiance = assetManager()->get("assets/kloofendal_irr.e2a").cast<e2::Texture2D>();
	map.radiance = assetManager()->get("assets/kloofendal_rad.e2a").cast<e2::Texture2D>();
	envMaps.push(map);

	map.irradiance = assetManager()->get("assets/kloofensky_irr.e2a").cast<e2::Texture2D>();
	map.radiance = assetManager()->get("assets/kloofensky_rad.e2a").cast<e2::Texture2D>();
	envMaps.push(map);


	m_session = e2::create<e2::GameSession>(this);

	e2::MeshProxyConfiguration cfg;
	cfg.lods.push({ 0.0f, demoFloorMesh });
	floorMesh = e2::create<e2::MeshProxy>(m_session, cfg);

	buildWater();

	/*
			std::string layerName;
		std::string description;
		std::string codeSnippet;

		uint32_t shaderIndex;
		uint32_t shaderSubIndex;
	*/


	setupStages();

	applyStage(DemoStageIndex::FinalRender);

	cfg.lods.clear();
	cfg.lods.push({ 0.0f, nimbleMeshAsset });
	nimbleMesh = e2::create<e2::MeshProxy>(m_session, cfg);
	nimbleMesh->modelMatrix = glm::scale(glm::identity<glm::mat4>(), { 0.5f, 0.5f, 0.5f });
	nimbleMesh->modelMatrix = glm::translate(nimbleMesh->modelMatrix, { 0.0f, 0.25f, 0.0f });
	nimbleMesh->modelMatrixDirty = true;
	nimbleMesh->disable();


	toolWindow = e2::create<e2::ToolWindow>(this);

}

void e2::Demo::setupStages() {

	stages.resize((uint64_t)DemoStageIndex::Count);
	stages[(uint64_t)DemoStageIndex::FinalRender].id = "FinalRender";
	stages[(uint64_t)DemoStageIndex::FinalRender].layerName = "";
	stages[(uint64_t)DemoStageIndex::FinalRender].description = "";
	stages[(uint64_t)DemoStageIndex::FinalRender].shaderIndex = 0;
	stages[(uint64_t)DemoStageIndex::FinalRender].shaderSubIndex = 0;
	
	stages[(uint64_t)DemoStageIndex::Height_Basic].id = "Height_Basic";
	stages[(uint64_t)DemoStageIndex::Height_Basic].layerName = "Height Function";
	stages[(uint64_t)DemoStageIndex::Height_Basic].description = "2-dimensional voronoi noise";
	stages[(uint64_t)DemoStageIndex::Height_Basic].shaderIndex = 1;
	stages[(uint64_t)DemoStageIndex::Height_Basic].shaderSubIndex = 0;
	stages[(uint64_t)DemoStageIndex::Height_Basic].codeSnippets = {
"^9// return a height coefficient between 0.0 and 1.0",
"^7float ^-sampleHeight^5(^7vec2 ^-position^5)^-",
"^5{",
"    ^5return^- ^2pow^5(^2voronoi2d^7(^-position \\* ^62.0^7)^-, ^62.0^5)^-;",
"^5}",
"",
"^9// apply the height to the vertex like so",
"^9// heightScale is a constant, remember it!",
"vertexPosition.y += ^2sampleHeight^5(^-vertexPosition.xz^5)^- \\* heightScale;"
	};

	stages[(uint64_t)DemoStageIndex::Normal_Basic].id = "Normal_Basic";
	stages[(uint64_t)DemoStageIndex::Normal_Basic].layerName = "Normal Vector Derivation";
	stages[(uint64_t)DemoStageIndex::Normal_Basic].description = "Deriving a normal vector from the height function.";
	stages[(uint64_t)DemoStageIndex::Normal_Basic].shaderIndex = 2;
	stages[(uint64_t)DemoStageIndex::Normal_Basic].shaderSubIndex = 0;
	stages[(uint64_t)DemoStageIndex::Normal_Basic].codeSnippets = {
"^7float ^-sampleHeight^5(^7vec2 ^-position^5)^-",
"^5{",
"    ^5return^- ^2pow^5(^2voronoi2d^7(^-position \\* ^62.0^7)^-, ^62.0^5)^-;",
"^5}",
"",
"^7vec3 ^-sampleWaterNormal^5(^7vec2 ^-position^5)^-",
"^5{^-",
"    ^7float ^-eps = ^6$NRMDIST^-; ^9// normal soft/sharp^-",
"    ^7float ^-str = ^6$NRMSTR^-; ^9// normal intensity^-",
"    ^7float ^-eps2 = eps \\* ^62.0^-;",
"    ^7vec3 ^-off = ^7vec3^5(^61.0^-, ^61.0^-, ^60.0^5)^- \\* eps;^-",
"",
"    ^9// ^7@note ^9make sure to apply heightscale to these samples!",
"    ^7float ^-hL = ^2sampleHeight^5(^-position - off.xz^5)^- \\* heightScale;",
"    ^7float ^-hR = ^2sampleHeight^5(^-position + off.xz^5)^- \\* heightScale;",
"    ^7float ^-hU = ^2sampleHeight^5(^-position - off.zy^5)^- \\* heightScale;",
"    ^7float ^-hD = ^2sampleHeight^5(^-position + off.zy^5)^- \\* heightScale;",
"",
"    ^5return^- -^2normalize^5(^7vec3^7(^-hR - hL, eps2 / str, hD - hU^7)^5)^-;",
"^5}^-",
	};

	stages[(uint64_t)DemoStageIndex::Height_Blend].id = "Height_Blend";
	stages[(uint64_t)DemoStageIndex::Height_Blend].layerName = "Height Function";
	stages[(uint64_t)DemoStageIndex::Height_Blend].description = "Multiple voronoi noise octaves, animated.";
	stages[(uint64_t)DemoStageIndex::Height_Blend].shaderIndex = 1;
	stages[(uint64_t)DemoStageIndex::Height_Blend].shaderSubIndex = 1;
	stages[(uint64_t)DemoStageIndex::Height_Blend].codeSnippets = {
"^7float ^-sampleHeight^5(^7vec2 ^-position^5)^-",
"^5{",
"    ^5return^- ^2pow^5(^2voronoi2d^7(^-position \\* ^62.0^7)^-, ^62.0^5)^-;",
"^5}",
"",
"^7float ^-sampleWaterHeight^5(^7vec2 ^-position, ^7float ^-time^5)^-",
"^5{",
"    ^9// ^7@note^9 not actual implementation as seen!!1",
"    ^9// I encourage you to experiment :)",
"    ^7float ^-h = ^2sampleHeight^5(^-position^5)^-;",
"    h += ^2sampleHeight^5(^-position + ^7vec2^7(^60.25^-, ^60.25^7)^- \\* time^5)^-;",
"    h /= ^62.0^-;",
"    ^5return^- h;",
"^5}",
};

	stages[(uint64_t)DemoStageIndex::Normal_Blend].id = "Normal_Blend";
	stages[(uint64_t)DemoStageIndex::Normal_Blend].layerName = "Normal Vector Derivation";
	stages[(uint64_t)DemoStageIndex::Normal_Blend].description = "Previewing derived normal vectors from combined voronoi octaves.";
	stages[(uint64_t)DemoStageIndex::Normal_Blend].shaderIndex = 2;
	stages[(uint64_t)DemoStageIndex::Normal_Blend].shaderSubIndex = 1;
	stages[(uint64_t)DemoStageIndex::Normal_Blend].codeSnippets = {
"^7float ^-sampleHeight^5(^7vec2 ^-position^5)^-",
"^5{",
"    ^5return^- ^2pow^5(^2voronoi2d^7(^-position \\* ^62.0^7)^-, ^62.0^5)^-;",
"^5}",
"",
"^7float ^-sampleWaterHeight^5(^7vec2 ^-position, ^7float ^-time^5)^-",
"^5{",
"    ^9// ^7@note^9 not actual implementation as seen!!1",
"    ^9// I encourage you to experiment :)",
"    ^7float ^-h = ^2sampleHeight^5(^-position^5)^-;",
"    h += ^2sampleHeight^5(^-position + ^7vec2^7(^60.25^-, ^60.25^7)^- \\* time^5)^-;",
"    h /= ^62.0^-;",
"    ^5return^- h;",
"^5}",
"",
"^7vec3 ^-sampleWaterNormal^5(^7vec2 ^-position, ^7float ^-time^5)^-",
"^5{^-",
"    ^7float ^-eps = ^6$NRMDIST^-; ^9// controls normal sharpness^-",
"    ^7float ^-str = ^6$NRMSTR^-; ^9// controls normal strength^-",
"    ^7float ^-eps2 = eps \\* ^62.0^-;",
"    ^7vec3 ^-off = ^7vec3^5(^61.0^-, ^61.0^-, ^60.0^5)^- \\* eps;^-",
"",
"    ^9// ^7@note ^9make sure to apply heightscale to these samples!",
"    ^7float ^-hL = ^2sampleWaterHeight^5(^-position - off.xz, time^5)^- \\* heightScale;",
"    ^7float ^-hR = ^2sampleWaterHeight^5(^-position + off.xz, time^5)^- \\* heightScale;",
"    ^7float ^-hU = ^2sampleWaterHeight^5(^-position - off.zy, time^5)^- \\* heightScale;",
"    ^7float ^-hD = ^2sampleWaterHeight^5(^-position + off.zy, time^5)^- \\* heightScale;",
"",
"    ^5return^- -^2normalize^5(^7vec3^7(^-hR - hL, eps2 / str, hD - hU^7)^5)^-;",
"^5}^-",
	};

	stages[(uint64_t)DemoStageIndex::BaseLayer_SingleColor].id = "BaseLayer_SingleColor";
	stages[(uint64_t)DemoStageIndex::BaseLayer_SingleColor].layerName = "Base Layer";
	stages[(uint64_t)DemoStageIndex::BaseLayer_SingleColor].description = "Creating a base layer for the surface water color.";
	stages[(uint64_t)DemoStageIndex::BaseLayer_SingleColor].shaderIndex = 3;
	stages[(uint64_t)DemoStageIndex::BaseLayer_SingleColor].shaderSubIndex = 0;
	stages[(uint64_t)DemoStageIndex::BaseLayer_SingleColor].codeSnippets = {
"^7vec3 ^-baseLayer^5(^7vec2 ^-position, ^7float ^-time^5)^-",
"^5{",
"    ^7vec3 ^-baseColor = ^7vec3^5(^60.2^-, ^60.7^-, ^61.0^5)^-;",
"    ^5return^- baseColor;",
"^5}"
};


	stages[(uint64_t)DemoStageIndex::BaseLayer_HeightBlend].id = "BaseLayer_HeightBlend";
	stages[(uint64_t)DemoStageIndex::BaseLayer_HeightBlend].layerName = "Base Layer";
	stages[(uint64_t)DemoStageIndex::BaseLayer_HeightBlend].description = "Blending between colors based on water height.";
	stages[(uint64_t)DemoStageIndex::BaseLayer_HeightBlend].shaderIndex = 3;
	stages[(uint64_t)DemoStageIndex::BaseLayer_HeightBlend].shaderSubIndex = 1;
	stages[(uint64_t)DemoStageIndex::BaseLayer_HeightBlend].codeSnippets = {
"^7vec3 ^-baseLayer^5(^7vec2 ^-position, ^7float ^-time^5)^-",
"^5{",
"    ^7vec3 ^-baseColor = ^7vec3^5(^60.2^-, ^60.7^-, ^61.0^5)^-;",
"    ^7vec3 ^-brightColor = ^7vec3^5(^60.6^-, ^60.9^-, ^61.0^5)^-;",
"    ^7float ^-h = ^2sampleWaterHeight^5(^-position, time^5)^-;",
"    ^7vec3 ^-blendColor = ^2mix^5(^-baseColor, brightColor, h^5)^-;",
"    ^5return^- blendColor;",
"^5}"
};

	stages[(uint64_t)DemoStageIndex::BaseLayer_SunLight].id = "BaseLayer_SunLight";
	stages[(uint64_t)DemoStageIndex::BaseLayer_SunLight].layerName = "Base Layer";
	stages[(uint64_t)DemoStageIndex::BaseLayer_SunLight].description = "Calculating water surface sunlight.";
	stages[(uint64_t)DemoStageIndex::BaseLayer_SunLight].shaderIndex = 3;
	stages[(uint64_t)DemoStageIndex::BaseLayer_SunLight].shaderSubIndex = 2;
	stages[(uint64_t)DemoStageIndex::BaseLayer_SunLight].codeSnippets = {
"^7float ^-baseLayerSun^5(^7vec2 ^-position, ^7float ^-time^5)^-",
"^5{",
"    ^9// sample the water normal at the given position and time",
"    ^7vec3 ^-n = ^2sampleWaterNormal^5(^-position, time^5)^-;",
"",
"    ^9// ^7@note^9 needs a worldspace surfaceToLight unit vector here!",
"    ^7float ^-ndotl = ^2clamp^5(^2dot^7(^-n, surfaceToLight^7)^-, ^60.0^-, ^61.0^5)^-;",
"",
"     ^9// return a factor between 0.5 and 1.0 !", 
"    ndotl = ndotl \\* ^60.5^- + ^60.5^-;",
"",
"    ^5return ^-ndotl;",
"^5}"
	};

	stages[(uint64_t)DemoStageIndex::BaseLayer_SunLightBlend].id = "BaseLayer_SunLightBlend";
	stages[(uint64_t)DemoStageIndex::BaseLayer_SunLightBlend].layerName = "Base Layer";
	stages[(uint64_t)DemoStageIndex::BaseLayer_SunLightBlend].description = "Blending water surface with sunlight.";
	stages[(uint64_t)DemoStageIndex::BaseLayer_SunLightBlend].shaderIndex = 3;
	stages[(uint64_t)DemoStageIndex::BaseLayer_SunLightBlend].shaderSubIndex = 3;
	stages[(uint64_t)DemoStageIndex::BaseLayer_SunLightBlend].codeSnippets = {
"^7float ^-baseLayerSun^5(^7vec2 ^-position, ^7float ^-time^5)^-",
"^5{",
"    ^9// sample the water normal at the given position and time",
"    ^7vec3 ^-n = ^2sampleWaterNormal^5(^-position, time^5)^-;",
"",
"    ^9// ^7@note^9 needs a worldspace surfaceToLight unit vector here!",
"    ^7float ^-ndotl = ^2clamp^5(^2dot^7(^-n, surfaceToLight^7)^-, ^60.0^-, ^61.0^5)^-;",
"",
"     ^9// return a factor between 0.5 and 1.0 !",
"    ndotl = ndotl \\* ^60.5^- + ^60.5^-;",
"",
"    ^5return ^-ndotl;",
"^5}",
"",
"^7vec3 ^-baseLayer^5(^7vec2 ^-position, ^7float ^-time^5)^-",
"^5{",
"    ^7vec3 ^-baseColor = ^7vec3^5(^60.2^-, ^60.7^-, ^61.0^5)^-;",
"    ^7vec3 ^-brightColor = ^7vec3^5(^60.6^-, ^60.9^-, ^61.0^5)^-;",
"    ^7float ^-h = ^2sampleWaterHeight^5(^-position, time^5)^-;",
"    ^7float ^-sun = ^2baseLayerSun^5(^-position, time^5)^-;",
"    ^7vec3 ^-blendColor = ^2mix^5(^-baseColor, brightColor, h \\* sun^5)^-;",
"    ^5return^- blendColor;",
"^5}"
	};

	stages[(uint64_t)DemoStageIndex::Reflection_Cubemap].id = "Reflection_Cubemap";
	stages[(uint64_t)DemoStageIndex::Reflection_Cubemap].layerName = "Reflections";
	stages[(uint64_t)DemoStageIndex::Reflection_Cubemap].description = "Creating reflection highlights using a HDR cubemap.";
	stages[(uint64_t)DemoStageIndex::Reflection_Cubemap].shaderIndex = 4;
	stages[(uint64_t)DemoStageIndex::Reflection_Cubemap].shaderSubIndex = 0;
	stages[(uint64_t)DemoStageIndex::Reflection_Cubemap].codeSnippets = {
"^7vec3 ^-reflectionLayer^5(^7vec2 ^-position, ^7float ^-time^5)^-",
"^5{",
"    ^9// sample the water normal at the given position and time",
"    ^7vec3 ^-n = ^2sampleWaterNormal^5(^-position, time^5)^-;",
"",
"    ^9// sample a HDR cubemap at the given normal",
"    ^7vec3 ^-hdr = ^2sampleHdrCube^5(^-n^5)^-;",
"",
"    ^5return^- hdr;",
"^5}"
	};

	stages[(uint64_t)DemoStageIndex::Reflection_Fresnel].id = "Reflection_Fresnel";
	stages[(uint64_t)DemoStageIndex::Reflection_Fresnel].layerName = "Reflections";
	stages[(uint64_t)DemoStageIndex::Reflection_Fresnel].description = "Adding a fresnel coefficient to the reflection.";
	stages[(uint64_t)DemoStageIndex::Reflection_Fresnel].shaderIndex = 4;
	stages[(uint64_t)DemoStageIndex::Reflection_Fresnel].shaderSubIndex = 1;
	stages[(uint64_t)DemoStageIndex::Reflection_Fresnel].codeSnippets = {
"^7float ^-fresnelEffect^5(^7vec3 ^-normal^5)^-",
"^5{",
"    ^9// you need to provide your own worldspace viewToSurface unit vector",
"    ^7float ^-vdotn = ^2clamp^5(^2dot^7(^-viewToSurface, normal^7), ^60.0^-, ^61.0^5)^-;",
"",
"    ^5return^- vdotn;",
"^5}"
	};

	stages[(uint64_t)DemoStageIndex::Reflection_FresnelHeight].id = "Reflection_FresnelHeight";
	stages[(uint64_t)DemoStageIndex::Reflection_FresnelHeight].layerName = "Reflections";
	stages[(uint64_t)DemoStageIndex::Reflection_FresnelHeight].description = "Adding a height-based coefficient to mask out repetition and make it sharper.";
	stages[(uint64_t)DemoStageIndex::Reflection_FresnelHeight].shaderIndex = 4;
	stages[(uint64_t)DemoStageIndex::Reflection_FresnelHeight].shaderSubIndex = 2;
	stages[(uint64_t)DemoStageIndex::Reflection_FresnelHeight].codeSnippets = {
"^7float ^-fresnelEffect^5(^7vec3 ^-normal, ^7float ^-height^5)^-",
"^5{",
"    ^9// you need to provide your own worldspace viewToSurface unit vector",
"    ^7float ^-vdotn = ^2clamp^5(^2dot^7(^-viewToSurface, normal^7), ^60.0^-, ^61.0^5)^-;",
"",
"    ^9// an option is pow(vdotn, 8.0), but this gives sharper reflections",
"    ^7float ^-heightCoeff = ^2smoothstep^5(^60.1^-, ^60.7^-, height^5)^-;",
"    vdotn \\*= heightCoeff;",
"",
"    ^5return^- vdotn;",
"^5}"
	};

	stages[(uint64_t)DemoStageIndex::Reflection_Blend].id = "Reflection_Blend";
	stages[(uint64_t)DemoStageIndex::Reflection_Blend].layerName = "Reflections";
	stages[(uint64_t)DemoStageIndex::Reflection_Blend].description = "Blending the reflections with the baselayer.";
	stages[(uint64_t)DemoStageIndex::Reflection_Blend].shaderIndex = 4;
	stages[(uint64_t)DemoStageIndex::Reflection_Blend].shaderSubIndex = 3;
	stages[(uint64_t)DemoStageIndex::Reflection_Blend].codeSnippets = {
"^7float ^-fresnelEffect^5(^7vec3 ^-normal, ^7float ^-height^5)^-",
"^5{",
"    ^9// you need to provide your own worldspace viewToSurface unit vector",
"    ^7float ^-vdotn = ^2clamp^5(^2dot^7(^-viewToSurface, normal^7), ^60.0^-, ^61.0^5)^-;",
"",
"    ^9// an option is pow(vdotn, 8.0), but this gives sharper reflections",
"    ^7float ^-heightCoeff = ^2smoothstep^5(^60.1^-, ^60.7^-, height^5)^-;",
"    vdotn \\*= heightCoeff;",
"",
"    ^5return^- vdotn;",
"^5}",
"",
"^7vec3 ^-reflectionLayer^5(^7vec2 ^-position, ^7float ^-time^5)^-",
"^5{",
"    ^9// sample water normal and height",
"    ^7vec3 ^-n = ^2sampleWaterNormal^5(^-position, time^5)^-;",
"    ^7float ^-h = ^2sampleWaterHeight^5(^-position, time^5)^-;",
"",
"    ^9// sample a HDR cubemap at the given normal",
"    ^7vec3 ^-hdr = ^2sampleHdrCube^5(^-n^5)^-;",
"",
"    ^9// calculate fresnel at given normal and height",
"    ^7float ^-fresnel = ^2fresnelEffect^5(^-n, h^5)^-;",
"",
"    ^5return^- hdr \\* fresnel;",
"^5}",
"",
"^9// combine the base and reflection layers additively!",
"^9// ^7@note^9 real code would reuse calculated normal and height!",
"outColor.rgb = ^2baseLayer^5(^-position, time^5)^- + ^2reflectionLayer^5(^-position, time^5)^-;"
	};

	stages[(uint64_t)DemoStageIndex::BottomLayer_ShoreColor].id = "BottomLayer_ShoreColor";
	stages[(uint64_t)DemoStageIndex::BottomLayer_ShoreColor].layerName = "Bottom Layer";
	stages[(uint64_t)DemoStageIndex::BottomLayer_ShoreColor].description = "Blending the baselayer with the ground beneath.";
	stages[(uint64_t)DemoStageIndex::BottomLayer_ShoreColor].shaderIndex = 5;
	stages[(uint64_t)DemoStageIndex::BottomLayer_ShoreColor].shaderSubIndex = 0;
	stages[(uint64_t)DemoStageIndex::BottomLayer_ShoreColor].codeSnippets = {
"^9// ^7@note^9 this function takes the baselayer, and outputs a bottom-blend!",
"^7vec3 ^-bottomLayerBlend^5(^7vec3^- baseLayer^5)^-",
"^5{",
"    ^9// sample the front color buffer to get whats behind the output fragment",
"    ^7vec3 ^-frontBuffer = ^2sampleFrontBuffer^5()^-;",
"",
"    ^9// start by mixing a nice shallow layer",
"    ^7vec3 ^-shallowLayer = ^2mix^5(^-baseLayer, frontBuffer, ^60.15^5)^-;",
"",
"    ^5return^- shallowLayer;",
"^5}"
	};

	stages[(uint64_t)DemoStageIndex::BottomLayer_DeepBlend].id = "BottomLayer_DeepBlend";
	stages[(uint64_t)DemoStageIndex::BottomLayer_DeepBlend].layerName = "Bottom Layer";
	stages[(uint64_t)DemoStageIndex::BottomLayer_DeepBlend].description = "Making the bottom layer darker as it goes deeper.";
	stages[(uint64_t)DemoStageIndex::BottomLayer_DeepBlend].shaderIndex = 5;
	stages[(uint64_t)DemoStageIndex::BottomLayer_DeepBlend].shaderSubIndex = 1;
	stages[(uint64_t)DemoStageIndex::BottomLayer_DeepBlend].codeSnippets = {
"^9// ^7@note^9 this function takes the baselayer, and outputs a bottom-blend!",
"^7vec3 ^-bottomLayerBlend^5(^7vec3^- baseLayer^5)^-",
"^5{",
"    ^9// sample front buffer to get whats behind the output fragment",
"    ^7vec3 ^-frontBuffer = ^2sampleFrontBuffer^5()^-;",
"",
"    ^9// sample position buffer to get position behind the fragment",
"    ^7vec3 ^-frontPosition = ^2sampleFrontPosition^5()^-;",
"",
"    ^9// start by mixing a nice shallow layer",
"    ^7vec3 ^-shallowLayer = ^2mix^5(^-baseLayer, frontBuffer, ^60.15^5)^-;",
"",
"    ^9// then make a deeper blend, with a view distance coefficient",
"    ^9// ^7@note^9 need to provide worldspace position of output fragment here!",
"    ^7float ^-viewDist = ^2distance^5(^-frontPosition, fragmentPosition^5)^-;",
"    ^7float ^-shallowCoeff = ^61.0^- - ^2smoothstep^5(^60.0^-, ^60.35^-, viewDist^5)^-;",
"    ^7vec3 ^-deepLayer = ^2mix^5(^-frontBuffer \\* baseLayer, shallowLayer, shallowCoeff^5)^-;",
"",
"    ^9// preview the deep layer only for now!",
"    ^5return^- deepLayer;",
"^5}"
	};


	stages[(uint64_t)DemoStageIndex::BottomLayer_BaseBlend].id = "BottomLayer_BaseBlend";
	stages[(uint64_t)DemoStageIndex::BottomLayer_BaseBlend].layerName = "Bottom Layer";
	stages[(uint64_t)DemoStageIndex::BottomLayer_BaseBlend].description = "Blending the bottom layer to the base layer at its deepest.";
	stages[(uint64_t)DemoStageIndex::BottomLayer_BaseBlend].shaderIndex = 5;
	stages[(uint64_t)DemoStageIndex::BottomLayer_BaseBlend].shaderSubIndex = 2;
	stages[(uint64_t)DemoStageIndex::BottomLayer_BaseBlend].codeSnippets = {
"^9// ^7@note^9 this function takes the baselayer, and outputs a bottom-blend!",
"^7vec3 ^-bottomLayerBlend^5(^7vec3^- baseLayer^5)^-",
"^5{",
"    ^9// sample front buffer to get whats behind the output fragment",
"    ^7vec3 ^-frontBuffer = ^2sampleFrontBuffer^5()^-;",
"",
"    ^9// sample position buffer to get position behind the fragment",
"    ^7vec3 ^-frontPosition = ^2sampleFrontPosition^5()^-;",
"",
"    ^9// start by mixing a nice shallow layer",
"    ^7vec3 ^-shallowLayer = ^2mix^5(^-baseLayer, frontBuffer, ^60.15^5)^-;",
"",
"    ^9// then make a deeper blend, with a view distance coefficient",
"    ^9// ^7@note^9 need to provide worldspace position of output fragment here!",
"    ^7float ^-viewDist = ^2distance^5(^-frontPosition, fragmentPosition^5)^-;",
"    ^7float ^-shallowCoeff = ^61.0^- - ^2smoothstep^5(^60.0^-, ^60.35^-, viewDist^5)^-;",
"    ^7vec3 ^-deepLayer = ^2mix^5(^-frontBuffer \\* baseLayer, shallowLayer, shallowCoeff^5)^-;",
"",
"    ^9// lastly, blend base again with deep layer, and a new dist coeff",
"    ^7float ^-deepCoeff = ^61.0^- - ^2smoothstep^5(^60.0^-, ^61.0^-, viewDist^5)^-;",
"    ^7vec3 ^-finalLayer = ^2mix^5(^-baseLayer, deepLayer, deepCoeff^5)^-;",
"",
"    ^9// output the final blend",
"    ^5return^- finalLayer;",
"^5}",
"",
"^9// apply bottom layer blend to base before outputting",
"^7vec3 ^-base = ^2baseLayer^5(^-position, time^5)^-;",
"outColor.rgb = ^2bottomLayerBlend^5(^-base^5)^- + ^2reflectionLayer^5(^-position, time^5)^-;"
	};

	stages[(uint64_t)DemoStageIndex::BottomLayer_ShoreBlend].id = "BottomLayer_ShoreBlend";
	stages[(uint64_t)DemoStageIndex::BottomLayer_ShoreBlend].layerName = "Bottom Layer";
	stages[(uint64_t)DemoStageIndex::BottomLayer_ShoreBlend].description = "Making the shoreline softer.";
	stages[(uint64_t)DemoStageIndex::BottomLayer_ShoreBlend].shaderIndex = 5;
	stages[(uint64_t)DemoStageIndex::BottomLayer_ShoreBlend].shaderSubIndex = 3;
	stages[(uint64_t)DemoStageIndex::BottomLayer_ShoreBlend].codeSnippets = {
"^9// following the same example as the previous distance coefficients:",
"^7vec3 ^-base = ^2baseLayer^5(^-position, time^5)^-;",
"^7vec3 ^-blend = ^2bottomLayerBlend^5(^-base^5)^-;",
"",
"^7float ^-shoreCoeff = ^61.0^- - ^2smoothstep^5(^60.0^-, ^60.2^-, viewDist^5)^-;",
"^7vec3 ^-shoreBlend = ^2mix^5(^-blend, frontBuffer, shoreCoeff^5)^-;",
"",
"^7vec3 ^-reflection = ^2reflectionLayer^5(^-position, time^5)^-;",
"outColor.rgb = shoreBlend + reflection;"
	};

	stages[(uint64_t)DemoStageIndex::Caustics_Mask].id = "Caustics_Mask";
	stages[(uint64_t)DemoStageIndex::Caustics_Mask].layerName = "Water Caustics";
	stages[(uint64_t)DemoStageIndex::Caustics_Mask].description = "Creating caustics for the bottom layer.";
	stages[(uint64_t)DemoStageIndex::Caustics_Mask].shaderIndex = 6;
	stages[(uint64_t)DemoStageIndex::Caustics_Mask].shaderSubIndex = 0;
	stages[(uint64_t)DemoStageIndex::Caustics_Mask].codeSnippets = {
"^7float ^-causticsMask^5(^7float^- time^5)^-",
"^5{",
"    ^9// ^7@note^9 we use the front position here (!)",
"    ^7vec3 ^-frontPosition = ^2sampleFrontPosition^5()^-;",
"    ^7float ^-h = ^2sampleWaterHeight^5(^-frontPosition.xz, time^5)^-;",
"",
"    ^9// make it more shimmery",
"    h = ^2smoothstep^5(^60.0^-, ^60.5^-, h^5)^-;",
"",
"    ^5return ^-h;",
"^5}"
	};

	stages[(uint64_t)DemoStageIndex::Caustics_Only].id = "Caustics_Only";
	stages[(uint64_t)DemoStageIndex::Caustics_Only].layerName = "Water Caustics";
	stages[(uint64_t)DemoStageIndex::Caustics_Only].description = "Previewing the bottom layer with only caustics.";
	stages[(uint64_t)DemoStageIndex::Caustics_Only].shaderIndex = 6;
	stages[(uint64_t)DemoStageIndex::Caustics_Only].shaderSubIndex = 1;
	stages[(uint64_t)DemoStageIndex::Caustics_Only].codeSnippets = {
"^9// ^7@note^9 caustics is to be applied to the front color!",
"^7vec3 ^-frontBuffer = ^2sampleFrontBuffer^5()^-;",
"^7float ^-caustics = ^2causticsMask^5(^-time^5)^-;",
"",
"^9// apply the shore coeff to caustics to lessen effect near shore",
"caustics \\*= shoreCoeff;",
"",
"^9// create caustics color to blend to, by lightening the frontbuffer",
"^9// ^7@note^9 intensity  can be substituted with sun strength, for example",
"^7vec3 ^-causticsColor = frontBuffer + ^7(^-frontBuffer \\* intensity^7)^-;",
"",
"frontBuffer = ^2mix^5(^-frontBuffer, causticsColor, caustics^5)^-;",
"",
"^9// .. use frontBuffer here..."
};

	stages[(uint64_t)DemoStageIndex::Caustics_BottomBlend].id = "Caustics_BottomBlend";
	stages[(uint64_t)DemoStageIndex::Caustics_BottomBlend].layerName = "Water Caustics";
	stages[(uint64_t)DemoStageIndex::Caustics_BottomBlend].description = "Blending the caustics with the bottom layer.";
	stages[(uint64_t)DemoStageIndex::Caustics_BottomBlend].shaderIndex = 6;
	stages[(uint64_t)DemoStageIndex::Caustics_BottomBlend].shaderSubIndex = 2;
	stages[(uint64_t)DemoStageIndex::Caustics_BottomBlend].codeSnippets = {
"^9// ^7@note^9 caustics is to be applied to the front color!",
"^7vec3 ^-frontBuffer = ^2sampleFrontBuffer^5()^-;",
"^7float ^-caustics = ^2causticsMask^5(^-time^5)^-;",
"",
"^9// apply the shore coeff to caustics to lessen effect near shore",
"caustics \\*= shoreCoeff;",
"",
"^9// create caustics color to blend to, by lightening the frontbuffer",
"^9// ^7@note^9 intensity  can be substituted with sun strength, for example",
"^7vec3 ^-causticsColor = frontBuffer + ^7(^-frontBuffer \\* intensity^7)^-;",
"",
"frontBuffer = ^2mix^5(^-frontBuffer, causticsColor, caustics^5)^-;",
"",
"^9// .. use frontBuffer here..."
	};

	stages[(uint64_t)DemoStageIndex::Refraction_Offset].id = "Refraction_Offset";
	stages[(uint64_t)DemoStageIndex::Refraction_Offset].layerName = "Water Refraction";
	stages[(uint64_t)DemoStageIndex::Refraction_Offset].description = "Creating UV-coordinates for water refraction.";
	stages[(uint64_t)DemoStageIndex::Refraction_Offset].shaderIndex = 7;
	stages[(uint64_t)DemoStageIndex::Refraction_Offset].shaderSubIndex = 0;
	stages[(uint64_t)DemoStageIndex::Refraction_Offset].codeSnippets = {
"^9// calculate offset UV for front buffer",
"^7vec3 ^-viewSpaceNormal = viewMatrix \\* waterNormal;",
"^7vec2 ^-offsetUv = -viewSpaceNormal.xz;"
};

	stages[(uint64_t)DemoStageIndex::Refraction_Only].id = "Refraction_Only";
	stages[(uint64_t)DemoStageIndex::Refraction_Only].layerName = "Water Refraction";
	stages[(uint64_t)DemoStageIndex::Refraction_Only].description = "Previewing the bottom layer with only refraction.";
	stages[(uint64_t)DemoStageIndex::Refraction_Only].shaderIndex = 7;
	stages[(uint64_t)DemoStageIndex::Refraction_Only].shaderSubIndex = 1;
	stages[(uint64_t)DemoStageIndex::Refraction_Only].codeSnippets = {
"^9// calculate offset UV for front buffer",
"^7vec3 ^-viewSpaceNormal = viewMatrix \\* waterNormal;",
"^7vec2 ^-offsetUv = -viewSpaceNormal.xz;",
"",
"^9// scale offset to viewspace pixel units",
"^7vec2 ^-pixelScale = ^7vec2^5(^61.0^5)^- / resolution;",
"offsetUv \\*= pixelScale;",
"",
"^9// scale by viewdist, so deeper parts get more refraction",
"^7float ^-depthScale = ^2smoothstep^5(^60.0^-, ^61.0^-, viewDist^5)^-;",
"offsetUv \\*= depthScale;",
"",
"^9// scale by intensity, i.e. refraction will be max $REFSTR pixels wide",
"offsetUv \\*= ^6$REFSTR^-;",
"",
"^9// add this offsetUv when sampling frontbuffer and frontposition!",
"^7vec2 ^-screenUv = ^2getScreenPixelUV^5(^7gl_FragCoord^-.xy^5)^-;",
"^7vec3 ^-frontBuffer = ^2sampleFrontbuffer^5(^-screenUv + offsetUv^5)^-;"
};

	stages[(uint64_t)DemoStageIndex::Refraction_Blend].id = "Refraction_Blend";
	stages[(uint64_t)DemoStageIndex::Refraction_Blend].layerName = "Water Refraction";
	stages[(uint64_t)DemoStageIndex::Refraction_Blend].description = "Applying refraction to the bottom layer.";
	stages[(uint64_t)DemoStageIndex::Refraction_Blend].shaderIndex = 7;
	stages[(uint64_t)DemoStageIndex::Refraction_Blend].shaderSubIndex = 2;
	stages[(uint64_t)DemoStageIndex::Refraction_Blend].codeSnippets = {
"^9// calculate offset UV for front buffer",
"^7vec3 ^-viewSpaceNormal = viewMatrix \\* waterNormal;",
"^7vec2 ^-offsetUv = -viewSpaceNormal.xz;",
"",
"^9// scale offset to viewspace pixel units",
"^7vec2 ^-pixelScale = ^7vec2^5(^61.0^5)^- / resolution;",
"offsetUv \\*= pixelScale;",
"",
"^9// scale by viewdist, so deeper parts get more refraction",
"^7float ^-depthScale = ^2smoothstep^5(^60.0^-, ^61.0^-, viewDist^5)^-;",
"offsetUv \\*= depthScale;",
"",
"^9// scale by intensity, i.e. refraction will be max $REFSTR pixels wide",
"offsetUv \\*= ^6$REFSTR^-;",
"",
"^9// add this offsetUv when sampling frontbuffer and frontposition!",
"^7vec2 ^-screenUv = ^2getScreenPixelUV^5(^7gl_FragCoord^-.xy^5)^-;",
"^7vec3 ^-frontBuffer = ^2sampleFrontbuffer^5(^-screenUv + offsetUv^5)^-;"
	};

	stages[(uint64_t)DemoStageIndex::Refraction_Fixup].id = "Refraction_Fixup";
	stages[(uint64_t)DemoStageIndex::Refraction_Fixup].layerName = "Refraction Fixup";
	stages[(uint64_t)DemoStageIndex::Refraction_Fixup].description = "Fixing the refractions by checking height";
	stages[(uint64_t)DemoStageIndex::Refraction_Fixup].shaderIndex = 7;
	stages[(uint64_t)DemoStageIndex::Refraction_Fixup].shaderSubIndex = 3;
	stages[(uint64_t)DemoStageIndex::Refraction_Fixup].codeSnippets = {
"^9// create a mask for refracted samples that are above the water surface",
"fragHeight = fragmentPosition.y;",
"^7float ^-fixCoeff = ^2smoothstep^5(^-fragHeight - ^60.2^-, fragHeight - ^60.1^-, frontPosition.y^5)^-;",
"",
"^9// mix refracted color/pos with unrefracted based on coeff",
"^9// ^7@note ^9this requires us to sample front buffers twice;",
"^9// once with refraction offset, and once without(_OG)",
"frontColor = ^2mix^5(^-frontColor, frontColor_OG, fixCoeff^5)^-;",
"frontPosition = ^2mix^5(^-frontPosition, frontPosition_OG, fixCoeff^5)^-;",
};
}

void e2::Demo::shutdown()
{
	e2::destroy(toolWindow);
	e2::destroy(waterMesh);
	e2::destroy(testMesh);
	e2::destroy(nimbleMesh);
	nimbleMeshAsset = nullptr;
	nimbleSkeletonAsset = nullptr;
	nimbleAnimationAsset = nullptr;
	waterMeshAsset = nullptr;
	testMeshAsset = nullptr;
	envMaps[0].irradiance = nullptr;
	envMaps[0].radiance = nullptr;
	envMaps[1].irradiance = nullptr;
	envMaps[1].radiance = nullptr;
	envMaps[2].irradiance = nullptr;
	envMaps[2].radiance = nullptr;
	envMaps[3].irradiance = nullptr;
	envMaps[3].radiance = nullptr;
	waterMaterial = nullptr;
	waterProxy = nullptr;
	skyProxy = nullptr;
	skyMaterial = nullptr;
	e2::destroy(floorMesh);
	e2::destroy(m_session);
}

void e2::Demo::update(double seconds)
{

	bool isNormalView = (currentStage == DemoStageIndex::Normal_Basic || currentStage == DemoStageIndex::Normal_Blend);
	
	e2::UIContext* ui = m_session->uiContext();
	auto& kb = ui->keyboardState();
	auto& mouse = ui->mouseState();
	auto& leftMouse = mouse.buttons[0];

	if (kb.keys[int16_t(e2::Key::F5)].pressed)
	{
		renderManager()->invalidatePipelines();
	}

	if (kb.pressed(e2::Key::Enter) && kb.state(e2::Key::LeftAlt))
	{
		m_session->window()->setFullscreen(!m_session->window()->isFullscreen());
	}
	//ui->sliderFloat("View Angle A", m_demo->viewAngleA, -180.0f, 180.0f);
	//ui->sliderFloat("View Angle B", m_demo->viewAngleB, 0.0f, 90.f);

	if (mouse.buttonState(MouseButton::Left))
	{
		viewAngleA += mouse.moveDelta.x * 0.25;
		viewAngleB += mouse.moveDelta.y * 0.25;
	}

	if (glm::abs(mouse.scrollOffset) > 0.0f)
	{
		if(mouse.scrollOffset > 0.0)
			viewDistance *= (1.0f / 1.1f)* mouse.scrollOffset;
		else
			viewDistance *= 1.1f * glm::abs(mouse.scrollOffset);
	}

	while (viewAngleA > 360.0f)
		viewAngleA -= 360.0f;
	while (viewAngleA < -0.0f)
		viewAngleA += 360.0f;


	if (viewAngleB > 90.0f)
		viewAngleB = 90.0f;
	if (viewAngleB < 0.0f)
		viewAngleB = 0.0f;

	e2::Renderer* renderer = m_session->renderer();


	e2::RenderView view;

	glm::quat newOrientationA = glm::rotate(glm::identity<glm::quat>(), glm::radians(viewAngleA), e2::worldUpf());
	glm::quat newOrientationB = glm::rotate(glm::identity<glm::quat>(), glm::radians(viewAngleB), e2::worldRightf());

	currOrientationA = glm::slerp(currOrientationA, newOrientationA, float(seconds) * 4.0f);
	currOrientationB = glm::slerp(currOrientationB, newOrientationB, float(seconds) * 4.0f);

	glm::quat currOrientation = currOrientationA * currOrientationB;

	view.fov = viewFov;
	view.clipPlane = { 0.1f, 1000.0f };
	view.origin = glm::vec3(0.0f, isNormalView  && false? -waterHeight_Curr : 0.0f, 0.0f) + (currOrientation * glm::vec3(e2::worldForward()) * -viewDistance);
	view.orientation = currOrientation;
	renderer->setView(view);

	glm::quat sunRotation = glm::identity<glm::quat>();
	sunRotation = glm::rotate(sunRotation, glm::radians(sunAngleA), e2::worldUpf());
	sunRotation = glm::rotate(sunRotation, glm::radians(sunAngleB), e2::worldRightf());

	
	renderer->setSun(sunRotation, { 1.0f, 1.0f, 1.0f }, sunStrength);
	renderer->setIbl(iblStrength);
	renderer->whitepoint(glm::vec3(whitepoint));
	renderer->exposure(exposure);

	EnvMap map = envMaps[selectedMap];
	renderer->setEnvironment(map.irradiance->handle(), map.radiance->handle());

	map = envMaps[selectedWaterMap];
	waterProxy->reflectionHdr.set(map.radiance->handle());



	if (false && waterHeight_Animate)
	{
		waterHeight_AnimateTime += seconds;
		waterHeight_Curr = 0.0f + (glm::cos(waterHeight_AnimateTime + glm::pi<float>() / 2.0f) * 0.5f + 0.5f) * 0.5f;
	}
	else
	{
		waterHeight_AnimateTime = 0.0f;
		waterHeight_Curr = glm::mix(waterHeight_Curr, waterHeight, (float)seconds * 4.0f);
	}


	DemoData d = waterProxy->uniformData.data();


	d.water.x = waterHeight_Curr;
	d.water.y = normalStrength;
	d.water.z = normalDistance;
	d.water.w = waterScale;
	d.water2.x = waterStage;
	d.water2.y = waterStage2;
	d.water2.z = drawGrid + (drawGrid && animateGrid);
	d.albedo.x = refractionStrength;
	waterProxy->uniformData.set(d);

	m_session->tick(seconds);

	


	
	glm::vec2 winSize = m_session->window()->size();
	e2::DemoStage& stage = stages[(uint64_t)currentStage];


	glm::uvec2 resolution = renderer->resolution();
	glm::mat4 viewMatrix = renderer->view().calculateViewMatrix();
	glm::mat4 projectionMatrix = renderer->view().calculateProjectionMatrix(resolution);
	glm::mat4 viewProjectionMatrix = projectionMatrix * viewMatrix;

	auto worldToPixels = [&](glm::vec3 const& worldPosition) -> glm::vec2 {
		glm::vec4 viewPos = viewProjectionMatrix * glm::dvec4(glm::dvec3(worldPosition), 1.0);
		viewPos = viewPos / viewPos.z;

		glm::vec2 offset = (glm::vec2(viewPos.x, viewPos.y) * 0.5f + 0.5f) * glm::vec2(resolution);
		return offset;
	};

	if (isNormalView && renderExtra)
	{
		glm::vec2 c = worldToPixels({ 0.0f, -waterHeight_Curr, 0.0f });
		float cW = ui->calculateTextWidth(FontFace::Monospace, 12, "**O**");
		ui->drawQuadShadow(c - glm::vec2{ 10.0f, 10.0f }, { 20.0f, 20.0f }, 10.0f, 1.0f, 5.0f);
		ui->drawRasterText(FontFace::Monospace, 12, 0xFFFFFFFF, c - glm::vec2{cW / 2.0f, 0.0}, "**O**");

		glm::vec2 hL = worldToPixels({ -normalDistance, -waterHeight_Curr, 0.0f });
		float hLW = ui->calculateTextWidth(FontFace::Monospace, 12, "**hL**");
		ui->drawQuadShadow(hL - glm::vec2{ 10.0f, 10.0f }, { 20.0f, 20.0f }, 10.0f, 1.0f, 5.0f);
		ui->drawRasterText(FontFace::Monospace, 12, 0xFFFFFFFF, hL - glm::vec2{ hLW / 2.0f, 0.0 }, "**hL**");

		glm::vec2 hR = worldToPixels({ normalDistance, -waterHeight_Curr, 0.0f });
		float hRW = ui->calculateTextWidth(FontFace::Monospace, 12, "**hR**");
		ui->drawQuadShadow(hR - glm::vec2{ 10.0f, 10.0f }, { 20.0f, 20.0f }, 10.0f, 1.0f, 5.0f);
		ui->drawRasterText(FontFace::Monospace, 12, 0xFFFFFFFF, hR - glm::vec2{ hRW / 2.0f, 0.0 }, "**hR**");

		glm::vec2 hU = worldToPixels({ 0.0f, -waterHeight_Curr, -normalDistance });
		float hUW = ui->calculateTextWidth(FontFace::Monospace, 12, "**hU**");
		ui->drawQuadShadow(hU - glm::vec2{ 10.0f, 10.0f }, { 20.0f, 20.0f }, 10.0f, 1.0f, 5.0f);
		ui->drawRasterText(FontFace::Monospace, 12, 0xFFFFFFFF, hU - glm::vec2{ hUW / 2.0f, 0.0 }, "**hU**");

		glm::vec2 hD = worldToPixels({ 0.0f, -waterHeight_Curr, normalDistance });
		float hDW = ui->calculateTextWidth(FontFace::Monospace, 12, "**hD**");
		ui->drawQuadShadow(hD - glm::vec2{ 10.0f, 10.0f }, { 20.0f, 20.0f }, 10.0f, 1.0f, 5.0f);
		ui->drawRasterText(FontFace::Monospace, 12, 0xFFFFFFFF, hD - glm::vec2{ hDW / 2.0f, 0.0 }, "**hD**");

	}

	e2::UIStyle& style = uiManager()->workingStyle();

	float layerNameWidth = ui->calculateTextWidth(FontFace::Serif, 16, stage.layerName);
	glm::vec2 layerNamePosition;
	layerNamePosition.x = (winSize.x / 2.0f) - (layerNameWidth / 2.0f);
	layerNamePosition.y = 80.0f;
	if(renderExtra)
		ui->drawRasterText(FontFace::Serif, 24, style.windowFgColor, layerNamePosition, stage.layerName);

	float layerDescriptionWidth = ui->calculateTextWidth(FontFace::Serif, 12, stage.description);
	glm::vec2 layerDescriptionPosition;
	layerDescriptionPosition.x = (winSize.x / 2.0f) - (layerDescriptionWidth / 2.0f);
	layerDescriptionPosition.y = 80.0f + 36.0f;
	if(renderExtra)
		ui->drawRasterText(FontFace::Serif, 14, style.windowFgColor, layerDescriptionPosition, stage.description);






	/*
	if (stage.codeSnippets.size() <= 2)
	{
		float maxX = 0.0f;
		for (auto str : stage.codeSnippets)
		{
			float codeWidth = ui->calculateTextWidth(FontFace::Monospace, 12, str);
			if (codeWidth > maxX)
				maxX = codeWidth;
		}

		float curY = 116.0f;
		for (auto str : stage.codeSnippets)
		{

			glm::vec2 codePosition;
			codePosition.x = (winSize.x / 2.0f) - (maxX / 2.0f);
			codePosition.y = curY;
			ui->drawRasterText(FontFace::Monospace, 12, 0xFFFFFFFF, codePosition, str);

			curY += 14.0f;
		}
	}
	else*/
	if(renderExtra && stage.codeSnippets.size() > 0)
	{
		float maxX = 0.0f;
		for (auto str : stage.codeSnippets)
		{
			float codeWidth = ui->calculateTextWidth(FontFace::Monospace, 12, str);
			if (codeWidth > maxX)
				maxX = codeWidth;
		}
		
		glm::vec2 panelSize = { maxX + 16.0f, (stage.codeSnippets.size() * 14.0f) + 16.0f };
		glm::vec2 panelPos = { winSize.x - 600.0f, 200.0f };

		ui->drawQuadShadow(panelPos + glm::vec2(-8.0f, -12.0f), panelSize + glm::vec2(16.0f, 16.0f), 16.0f, 0.9f, 6.0f);
		//ui->drawQuad(panelPos + glm::vec2(0.0f, -4.0f), panelSize, e2::UIColor(0x0000007A));

		float curY = panelPos.y + 8.0f;
		for (auto str_src : stage.codeSnippets)
		{
			std::string str = str_src;
			str = e2::replace("$NRMDIST", std::format("{:.4f}", normalDistance), str);
			str = e2::replace("$NRMSTR", std::format("{:.4f}", normalStrength), str);
			str = e2::replace("$REFSTR", std::format("{:.1f}", refractionStrength), str);
			glm::vec2 codePosition;
			codePosition.x = panelPos.x + 8.0f;
			codePosition.y = curY;

			glm::vec2 highPos = codePosition + glm::vec2(-4.0f, -8.0f);
			glm::vec2 highSize = glm::vec2(maxX + 8.0f, 16.0f);

			glm::vec2 relPos = ui->mouseState().relativePosition;
			bool hov = relPos.x > highPos.x
				&& relPos.x < highPos.x + highSize.x
				&& relPos.y > highPos.y
				&& relPos.y < highPos.y + highSize.y;

			if(hov)
				ui->drawQuad(highPos, highSize, e2::UIColor(0x5550455F));

			//ui->drawRasterText(FontFace::Monospace, 12, 0x000000FF, codePosition + glm::vec2(1.0f, 1.0f), str, false);
			//ui->drawRasterText(FontFace::Monospace, 12, 0x000000FF, codePosition + glm::vec2(1.0f, -1.0f), str, false);
			//ui->drawRasterText(FontFace::Monospace, 12, 0x000000FF, codePosition + glm::vec2(-1.0f, 1.0f), str, false);
			//ui->drawRasterText(FontFace::Monospace, 12, 0x000000FF, codePosition + glm::vec2(-1.0f, -1.0f), str, false);
			ui->drawRasterText(FontFace::Monospace, 12, 0xFFFFFFFF, codePosition, str);



			curY += 14.0f;
		}
	}

}





e2::ToolWindow::ToolWindow(e2::Demo* demo)
	: e2::UIWindow(demo, e2::UIWindowFlags(WF_Default))
	, m_demo(demo)
{

}

e2::ToolWindow::~ToolWindow()
{
	glm::vec2 a;
	glm::normalize(a);
}

void e2::ToolWindow::update(double deltaTime)
{
	e2::UIWindow::update(deltaTime);

	e2::UIStyle& style = uiManager()->workingStyle();

	e2::UIContext* ui = uiContext();


	static e2::Name id_rootV = "rootV";
	static e2::Name id_empty = "empty";

	static float rowSlider = 128.0f;

	float rows[] = { rowSlider, 8.0f, 0.0f };
	ui->beginFlexH("h", rows, 3);


	ui->beginStackV("zzz");

	for (uint64_t i = 0; i < (uint64_t)DemoStageIndex::Count; i++)
	{
		e2::DemoStage& stage = m_demo->stages[i];

		if (i == (uint64_t)m_demo->currentStage)
		{
			ui->label(std::format("{}_label", stage.id), stage.id);
			continue;
		}

		if (ui->button(stage.id, stage.id))
		{
			m_demo->applyStage((DemoStageIndex)i);
		}
	}

	ui->endStackV();

	ui->flexHSlider("hs", rowSlider);

	ui->beginStackV("parameterStack");

	/*
		float m_viewAngleA{ 0.0f };
		float m_viewAngleB{ 0.0f };
		float m_viewDistance{ 5.0f };
		float m_viewFov{45.0f};

		float m_sunAngleA{ -75.0f };
		float m_sunAngleB{ 21.75f };
		float m_sunStrength{ 7.0f };
		float m_iblStrength{ 1.03f };

		float m_exposure{ 1.33f };
		float m_whitepoint { 5.0f };
	*/

#if defined(E2_PROFILER)
	ui->label("zzme", std::format("FPS: {:.2f}", engine()->metrics().realCpuFps));
#endif

	ui->label("a", "Camera:");
	ui->sliderFloat("View Angle A", m_demo->viewAngleA, 0.0f, 360.0f);
	ui->sliderFloat("View Angle B", m_demo->viewAngleB, 0.0f, 90.f);
	ui->sliderFloat("View Distance", m_demo->viewDistance, 0.0f, 20.0f);
	ui->sliderFloat("View FOV", m_demo->viewFov, 10.0f, 90.0f);
	ui->sliderFloat("Exposure", m_demo->exposure, 0.0f, 10.0f);
	ui->sliderFloat("Whitepoint", m_demo->whitepoint, 0.0f, 10.0f);
	ui->label("b", "-");
	ui->label("c", "Environment:");
	ui->sliderFloat("Sun Angle A", m_demo->sunAngleA, -180.0f, 180.0f);
	ui->sliderFloat("Sun Angle B", m_demo->sunAngleB, 0.0f, 90.f);
	ui->sliderFloat("Sun Strength", m_demo->sunStrength, 0.0f, 20.f);
	ui->sliderFloat("IBL Strength", m_demo->iblStrength, 0.0f, 20.f);
	ui->sliderInt("IBL Map", m_demo->selectedMap, 0, m_demo->envMaps.size() - 1);

	if (ui->checkbox("rwzzzz", m_demo->renderNimble, "Render Nimble"))
	{
		if (m_demo->renderNimble)
			m_demo->nimbleMesh->enable();
		else
			m_demo->nimbleMesh->disable();
	}


	ui->checkbox("zxfsdfgg", m_demo->drawGrid, "Render Grid");
	ui->checkbox("zxfsdfgg2", m_demo->animateGrid, "Animate Grid");
	ui->checkbox("renderextra", m_demo->renderExtra, "Render Extra");


	ui->label("d", "-");
	ui->label("e", "Water:");
	if (ui->checkbox("rw", m_demo->renderWater, "Render Water"))
	{
		if (m_demo->renderWater)
			m_demo->waterMesh->enable();
		else
			m_demo->waterMesh->disable();
	}
	ui->sliderInt("Water Map", m_demo->selectedWaterMap, 0, m_demo->envMaps.size() - 1);
	//ui->sliderInt("Water Debug", m_waterStage, 0, 10);
	//ui->sliderInt("Water Debug 2", m_waterStage2, 0, 10);
	ui->sliderFloat("Water Height", m_demo->waterHeight, 0.0f, 1.0f);

	if (ui->button("zz", "Begin"))
	{
		m_demo->renderWater = true;
		m_demo->waterMesh->enable();
		m_demo->applyStage(DemoStageIndex::FinalRender);
		m_demo->waterHeight_Curr = 10.0f;
	}
	ui->label("zzd", "-");
	ui->label("eff", "Params:");
	ui->sliderFloat("Height Scale", m_demo->waterScale, 0.05, 1.0);

	if (ui->button("resetn", "Reset Normal"))
	{
		m_demo->normalDistance = .015;
	}
	ui->sliderFloat("Normal Distance", m_demo->normalDistance, 0.001, 1.0);
	ui->sliderFloat("Normal Strength", m_demo->normalStrength, 0.001, 20.0);

	ui->sliderFloat("Refraction Strength", m_demo->refractionStrength, 0.0, 100.0);

	ui->endStackV();

	ui->endFlexH();
}

void e2::ToolWindow::onSystemButton()
{

}


e2::Engine* e2::ToolWindow::engine()
{
	return m_demo->engine();
}








e2::ApplicationType e2::Demo::type()
{
	return e2::ApplicationType::Game;
}

void e2::Demo::buildWater()
{

	auto rm = renderManager();


	glm::vec2 waterOrigin = {0.0f, 0.0f};
	glm::vec2 waterSize = {2.0f, 2.0f};
	uint32_t waterResolution = 128;
	glm::vec2 waterTileSize = waterSize / float(waterResolution);

	glm::vec2 waterOffset = -(waterSize / 2.0f);

	e2::DynamicMesh dynaWater;
	for (uint32_t y = 0; y < waterResolution; y++)
	{
		for (uint32_t x = 0; x < waterResolution; x++)
		{
			e2::Vertex tl;
			tl.position.x = waterOffset.x + float(x) * waterTileSize.x;
			tl.position.z = waterOffset.y + float(y) * waterTileSize.y;

			e2::Vertex tr;
			tr.position.x = waterOffset.x + float(x + 1) * waterTileSize.x;
			tr.position.z = waterOffset.y + float(y) * waterTileSize.y;

			e2::Vertex bl;
			bl.position.x = waterOffset.x + float(x) * waterTileSize.x;
			bl.position.z = waterOffset.y + float(y + 1) * waterTileSize.y;

			e2::Vertex br;
			br.position.x = waterOffset.x + float(x + 1) * waterTileSize.x;
			br.position.z = waterOffset.y + float(y + 1) * waterTileSize.y;

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

	waterMaterial = e2::MaterialPtr::create();
	waterMaterial->postConstruct(engine(), {});
	waterMaterial->overrideModel(rm->getShaderModel("e2::DemoModel"));
	waterProxy = m_session->getOrCreateDefaultMaterialProxy(waterMaterial)->unsafeCast<e2::DemoProxy>();
	waterMeshAsset = dynaWater.bake(waterMaterial, VertexAttributeFlags::None);



	e2::MeshProxyConfiguration cfg;
	cfg.lods.push({ 0.0f, waterMeshAsset });
	waterMesh = e2::create<e2::MeshProxy>(m_session, cfg);


	



	skyMaterial = e2::MaterialPtr::create();
	skyMaterial->postConstruct(engine(), {});
	skyMaterial->overrideModel(rm->getShaderModel("e2::SkyModel"));
	skyProxy = m_session->getOrCreateDefaultMaterialProxy(skyMaterial)->unsafeCast<e2::SkyProxy>();





	e2::DynamicMesh icoBuilder;
	icoBuilder.buildIcoSphere(4, true);
	testMeshAsset = icoBuilder.bake(skyMaterial, VertexAttributeFlags::None);

	cfg.lods.clear();
	cfg.lods.push({ 0.0f, testMeshAsset });
	testMesh = e2::create<e2::MeshProxy>(m_session, cfg);

	testMesh->modelMatrix = glm::scale(glm::identity<glm::mat4>(), { 100.0f, 100.0f, 100.0f });
	testMesh->modelMatrixDirty = true;
}

void e2::Demo::applyStage(DemoStageIndex index)
{
	currentStage = index;

	e2::DemoStage& stage = stages[(uint64_t)currentStage];
	waterStage = stage.shaderIndex;
	waterStage2 = stage.shaderSubIndex;

}

int main(int argc, char** argv)
{
	::registerGeneratedTypes();
	e2::Engine engine;
	e2::Demo demo(&engine);

	engine.run(&demo);

	return 0;
}
















e2::DemoModel::DemoModel()
	: e2::ShaderModel()
{
	m_pipelineCache.resize(uint16_t(e2::DemoFlags::Count));
	for (uint16_t i = 0; i < uint16_t(e2::DemoFlags::Count); i++)
	{
		m_pipelineCache[i] = {};
	}
}

e2::DemoModel::~DemoModel()
{
	for (uint16_t i = 0; i < m_pipelineCache.size(); i++)
	{
		if (m_pipelineCache[i].vertexShader)
			e2::destroy(m_pipelineCache[i].vertexShader);
		if (m_pipelineCache[i].fragmentShader)
			e2::destroy(m_pipelineCache[i].fragmentShader);
		if (m_pipelineCache[i].pipeline)
			e2::destroy(m_pipelineCache[i].pipeline);
	}

	e2::destroy(m_proxyUniformBuffers[0]);
	e2::destroy(m_proxyUniformBuffers[1]);
	e2::destroy(m_descriptorPool);
	e2::destroy(m_pipelineLayout);
	e2::destroy(m_descriptorSetLayout);
}

void e2::DemoModel::postConstruct(e2::Context* ctx)
{
	e2::ShaderModel::postConstruct(ctx);

	e2::DescriptorSetLayoutCreateInfo setLayoutCreateInfo{};
	setLayoutCreateInfo.bindings = {
		{ e2::DescriptorBindingType::UniformBuffer , 1}, // ubo params
		{ e2::DescriptorBindingType::Texture, 1},
	};
	m_descriptorSetLayout = renderContext()->createDescriptorSetLayout(setLayoutCreateInfo);


	e2::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sets = {
		renderManager()->rendererSetLayout(),
		renderManager()->modelSetLayout(),
		m_descriptorSetLayout
	};
	pipelineLayoutCreateInfo.pushConstantSize = sizeof(e2::PushConstantData);
	m_pipelineLayout = renderContext()->createPipelineLayout(pipelineLayoutCreateInfo);

	//m_pipelineCache.reserve(128);


	e2::DescriptorPoolCreateInfo poolCreateInfo{};
	poolCreateInfo.maxSets = e2::maxNumDemoProxies * 2;
	poolCreateInfo.numTextures = e2::maxNumDemoProxies * 2 * 1;
	poolCreateInfo.numUniformBuffers = e2::maxNumDemoProxies * 2 * 1;
	m_descriptorPool = mainThreadContext()->createDescriptorPool(poolCreateInfo);


	e2::DataBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.dynamic = true;
	bufferCreateInfo.size = renderManager()->paddedBufferSize(e2::maxNumDemoProxies * sizeof(e2::DemoData));
	bufferCreateInfo.type = BufferType::UniformBuffer;
	m_proxyUniformBuffers[0] = renderContext()->createDataBuffer(bufferCreateInfo);
	m_proxyUniformBuffers[1] = renderContext()->createDataBuffer(bufferCreateInfo);


}

e2::MaterialProxy* e2::DemoModel::createMaterialProxy(e2::Session* session, e2::MaterialPtr material)
{
	e2::DemoProxy* newProxy = e2::create<e2::DemoProxy>(session, material);

	for (uint8_t i = 0; i < 2; i++)
	{
		newProxy->sets[i] = m_descriptorPool->createDescriptorSet(m_descriptorSetLayout);
		newProxy->sets[i]->writeUniformBuffer(0, m_proxyUniformBuffers[i], sizeof(e2::DemoData), renderManager()->paddedBufferSize(sizeof(e2::DemoData)) * newProxy->id);
	}

	e2::DemoData newData;
	newData.albedo = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	newData.water = glm::vec4(0.2f, 0.0f, 0.0f, 0.0f);
	newData.water2 = glm::uvec4(0, 0, 0, 0);
	newProxy->uniformData.set(newData);

	newProxy->reflectionHdr.set(renderManager()->defaultTexture()->handle());

	return newProxy;
}

e2::IPipelineLayout* e2::DemoModel::getOrCreatePipelineLayout(e2::MeshProxy* proxy, uint8_t lodIndex, uint8_t submeshIndex, bool shadows)
{
	if (shadows)
		return nullptr;
	else
		return m_pipelineLayout;
}

e2::IPipeline* e2::DemoModel::getOrCreatePipeline(e2::MeshProxy* proxy, uint8_t lodIndex, uint8_t submeshIndex, e2::RendererFlags rendererFlags)
{
	if (!m_shadersReadFromDisk)
	{
		m_shadersOnDiskOK = true;

		if (!e2::readFileWithIncludes("shaders/demo/demo.vertex.glsl", m_vertexSource))
		{
			m_shadersOnDiskOK = false;
			LogError("failed to read vertex source from disk");
		}

		if (!e2::readFileWithIncludes("shaders/demo/demo.fragment.glsl", m_fragmentSource))
		{
			m_shadersOnDiskOK = false;
			LogError("failed to read fragment source from disk");
		}

		m_shadersReadFromDisk = true;
	}

	if (!m_shadersOnDiskOK)
	{
		return nullptr;
	}

	e2::SubmeshSpecification const& spec = proxy->lods[lodIndex].asset->specification(submeshIndex);
	e2::DemoProxy* lwProxy = static_cast<e2::DemoProxy*>(proxy->lods[lodIndex].materialProxies[submeshIndex]);

	uint16_t geometryFlags = (uint16_t)spec.attributeFlags;

	uint16_t materialFlags = 0;

	uint16_t lwFlagsInt = (geometryFlags << (uint16_t)e2::DemoFlags::VertexFlagsOffset)
		| (uint16_t(rendererFlags) << (uint16_t)e2::DemoFlags::RendererFlagsOffset)
		| (materialFlags);

	e2::DemoFlags lwFlags = (e2::DemoFlags)lwFlagsInt;

	if (m_pipelineCache[uint16_t(lwFlags)].vertexShader)
	{
		return m_pipelineCache[uint16_t(lwFlags)].pipeline;
	}

	e2::DemoCacheEntry newEntry;

	e2::ShaderCreateInfo shaderInfo;

	e2::applyVertexAttributeDefines(spec.attributeFlags, shaderInfo);

	if ((lwFlags & e2::DemoFlags::Shadow) == e2::DemoFlags::Shadow)
		shaderInfo.defines.push({ "Renderer_Shadow", "1" });

	if ((lwFlags & e2::DemoFlags::Skin) == e2::DemoFlags::Skin)
		shaderInfo.defines.push({ "Renderer_Skin", "1" });

	shaderInfo.stage = ShaderStage::Vertex;
	shaderInfo.source = m_vertexSource.c_str();
	newEntry.vertexShader = renderContext()->createShader(shaderInfo);

	shaderInfo.stage = ShaderStage::Fragment;
	shaderInfo.source = m_fragmentSource.c_str();
	newEntry.fragmentShader = renderContext()->createShader(shaderInfo);

	if (newEntry.vertexShader && newEntry.fragmentShader && newEntry.vertexShader->valid() && newEntry.fragmentShader->valid())
	{
		e2::PipelineCreateInfo pipelineInfo;
		pipelineInfo.layout = m_pipelineLayout;
		pipelineInfo.shaders = { newEntry.vertexShader, newEntry.fragmentShader };
		pipelineInfo.colorFormats = { e2::TextureFormat::RGBA32, e2::TextureFormat::RGBA32 };
		pipelineInfo.depthFormat = { e2::TextureFormat::D32 };
		pipelineInfo.alphaBlending = true;
		newEntry.pipeline = renderContext()->createPipeline(pipelineInfo);
	}
	else
	{
		LogError("shader compilation failed for the given bitflags: {:b}", lwFlagsInt);
	}

	m_pipelineCache[uint16_t(lwFlags)] = newEntry;
	return newEntry.pipeline;
}

void e2::DemoModel::invalidatePipelines()
{
	m_shadersReadFromDisk = false;
	m_shadersOnDiskOK = false;
	m_vertexSource.clear();
	m_fragmentSource.clear();


	for (uint16_t i = 0; i < uint16_t(e2::DemoFlags::Count); i++)
	{
		e2::DemoCacheEntry& entry = m_pipelineCache[i];

		if (entry.vertexShader)
			e2::discard(entry.vertexShader);
		entry.vertexShader = nullptr;

		if (entry.fragmentShader)
			e2::discard(entry.fragmentShader);
		entry.fragmentShader = nullptr;

		if (entry.pipeline)
			e2::discard(entry.pipeline);
		entry.pipeline = nullptr;
	}
}

e2::RenderLayer e2::DemoModel::renderLayer()
{
	return RenderLayer::Water;
}

e2::DemoProxy::DemoProxy(e2::Session* inSession, e2::MaterialPtr materialAsset)
	: e2::MaterialProxy(inSession, materialAsset)
{
	// @todo dont use gamesession here, it uses the default game session and thus this breaks in PIE etc.
	// instead take a sesson as parametr when creating proxies or similar
	session->registerMaterialProxy(this);
	model = static_cast<e2::DemoModel*>(materialAsset->model());
	id = model->proxyIds.create();
}

e2::DemoProxy::~DemoProxy()
{
	session->unregisterMaterialProxy(this);
	model->proxyIds.destroy(id);

	sets[0]->keepAround();
	e2::destroy(sets[0]);

	sets[1]->keepAround();
	e2::destroy(sets[1]);
}

void e2::DemoProxy::bind(e2::ICommandBuffer* buffer, uint8_t frameIndex, bool shadows)
{
	if (!shadows)
		buffer->bindDescriptorSet(model->m_pipelineLayout, 2, sets[frameIndex]);
}

void e2::DemoProxy::invalidate(uint8_t frameIndex)
{
	if (uniformData.invalidate(frameIndex))
	{
		uint32_t proxyOffset = renderManager()->paddedBufferSize(sizeof(DemoData)) * id;
		model->m_proxyUniformBuffers[frameIndex]->upload(reinterpret_cast<uint8_t const*>(&uniformData.data()), sizeof(DemoData), 0, proxyOffset);
	}
	if (reflectionHdr.invalidate(frameIndex))
	{
		e2::ITexture* tex = reflectionHdr.data();
		if (tex)
			sets[frameIndex]->writeTexture(1, tex);
	}
}





















e2::SkyModel::SkyModel()
	: e2::ShaderModel()
{
	m_pipelineCache.resize(uint16_t(e2::SkyFlags::Count));
	for (uint16_t i = 0; i < uint16_t(e2::SkyFlags::Count); i++)
	{
		m_pipelineCache[i] = {};
	}
}

e2::SkyModel::~SkyModel()
{
	for (uint16_t i = 0; i < m_pipelineCache.size(); i++)
	{
		if (m_pipelineCache[i].vertexShader)
			e2::destroy(m_pipelineCache[i].vertexShader);
		if (m_pipelineCache[i].fragmentShader)
			e2::destroy(m_pipelineCache[i].fragmentShader);
		if (m_pipelineCache[i].pipeline)
			e2::destroy(m_pipelineCache[i].pipeline);
	}

	e2::destroy(m_pipelineLayout);
}

void e2::SkyModel::postConstruct(e2::Context* ctx)
{
	e2::ShaderModel::postConstruct(ctx);

	e2::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sets = {
		renderManager()->rendererSetLayout(),
		renderManager()->modelSetLayout()
	};
	pipelineLayoutCreateInfo.pushConstantSize = sizeof(e2::PushConstantData);
	m_pipelineLayout = renderContext()->createPipelineLayout(pipelineLayoutCreateInfo);
}

e2::MaterialProxy* e2::SkyModel::createMaterialProxy(e2::Session* session, e2::MaterialPtr material)
{
	e2::SkyProxy* newProxy = e2::create<e2::SkyProxy>(session, material);

	return newProxy;
}

e2::IPipelineLayout* e2::SkyModel::getOrCreatePipelineLayout(e2::MeshProxy* proxy, uint8_t lodIndex, uint8_t submeshIndex, bool shadows)
{
	if (shadows)
		return nullptr;
	else
		return m_pipelineLayout;
}

e2::IPipeline* e2::SkyModel::getOrCreatePipeline(e2::MeshProxy* proxy, uint8_t lodIndex, uint8_t submeshIndex, e2::RendererFlags rendererFlags)
{
	if (!m_shadersReadFromDisk)
	{
		m_shadersOnDiskOK = true;

		if (!e2::readFileWithIncludes("shaders/sky/sky.vertex.glsl", m_vertexSource))
		{
			m_shadersOnDiskOK = false;
			LogError("failed to read vertex source from disk");
		}

		if (!e2::readFileWithIncludes("shaders/sky/sky.fragment.glsl", m_fragmentSource))
		{
			m_shadersOnDiskOK = false;
			LogError("failed to read fragment source from disk");
		}

		m_shadersReadFromDisk = true;
	}

	if (!m_shadersOnDiskOK)
	{
		return nullptr;
	}

	e2::SubmeshSpecification const& spec = proxy->lods[lodIndex].asset->specification(submeshIndex);
	e2::SkyProxy* lwProxy = static_cast<e2::SkyProxy*>(proxy->lods[lodIndex].materialProxies[submeshIndex]);

	uint16_t geometryFlags = (uint16_t)spec.attributeFlags;

	uint16_t materialFlags = 0;

	uint16_t lwFlagsInt = (geometryFlags << (uint16_t)e2::SkyFlags::VertexFlagsOffset)
		| (uint16_t(rendererFlags) << (uint16_t)e2::SkyFlags::RendererFlagsOffset)
		| (materialFlags);

	e2::SkyFlags lwFlags = (e2::SkyFlags)lwFlagsInt;

	if (m_pipelineCache[uint16_t(lwFlags)].vertexShader)
	{
		return m_pipelineCache[uint16_t(lwFlags)].pipeline;
	}

	e2::SkyCacheEntry newEntry;

	e2::ShaderCreateInfo shaderInfo;

	e2::applyVertexAttributeDefines(spec.attributeFlags, shaderInfo);

	if ((lwFlags & e2::SkyFlags::Shadow) == e2::SkyFlags::Shadow)
		shaderInfo.defines.push({ "Renderer_Shadow", "1" });

	if ((lwFlags & e2::SkyFlags::Skin) == e2::SkyFlags::Skin)
		shaderInfo.defines.push({ "Renderer_Skin", "1" });

	shaderInfo.stage = ShaderStage::Vertex;
	shaderInfo.source = m_vertexSource.c_str();
	newEntry.vertexShader = renderContext()->createShader(shaderInfo);

	shaderInfo.stage = ShaderStage::Fragment;
	shaderInfo.source = m_fragmentSource.c_str();
	newEntry.fragmentShader = renderContext()->createShader(shaderInfo);

	if (newEntry.vertexShader && newEntry.fragmentShader && newEntry.vertexShader->valid() && newEntry.fragmentShader->valid())
	{
		e2::PipelineCreateInfo pipelineInfo;
		pipelineInfo.layout = m_pipelineLayout;
		pipelineInfo.shaders = { newEntry.vertexShader, newEntry.fragmentShader };
		pipelineInfo.colorFormats = { e2::TextureFormat::RGBA32, e2::TextureFormat::RGBA32 };
		pipelineInfo.depthFormat = { e2::TextureFormat::D32 };
		pipelineInfo.alphaBlending = true;
		newEntry.pipeline = renderContext()->createPipeline(pipelineInfo);
	}
	else
	{
		LogError("shader compilation failed for the given bitflags: {:b}", lwFlagsInt);
	}

	m_pipelineCache[uint16_t(lwFlags)] = newEntry;
	return newEntry.pipeline;
}

void e2::SkyModel::invalidatePipelines()
{
	m_shadersReadFromDisk = false;
	m_shadersOnDiskOK = false;
	m_vertexSource.clear();
	m_fragmentSource.clear();


	for (uint16_t i = 0; i < uint16_t(e2::DemoFlags::Count); i++)
	{
		e2::SkyCacheEntry& entry = m_pipelineCache[i];

		if (entry.vertexShader)
			e2::discard(entry.vertexShader);
		entry.vertexShader = nullptr;

		if (entry.fragmentShader)
			e2::discard(entry.fragmentShader);
		entry.fragmentShader = nullptr;

		if (entry.pipeline)
			e2::discard(entry.pipeline);
		entry.pipeline = nullptr;
	}
}

e2::RenderLayer e2::SkyModel::renderLayer()
{
	return RenderLayer::Sky;
}

bool e2::SkyModel::supportsShadows()
{
	return false;
}

e2::SkyProxy::SkyProxy(e2::Session* inSession, e2::MaterialPtr materialAsset)
	: e2::MaterialProxy(inSession, materialAsset)
{
	// @todo dont use gamesession here, it uses the default game session and thus this breaks in PIE etc.
	// instead take a sesson as parametr when creating proxies or similar
	session->registerMaterialProxy(this);
	model = static_cast<e2::SkyModel*>(materialAsset->model());
}

e2::SkyProxy::~SkyProxy()
{
	session->unregisterMaterialProxy(this);
}

void e2::SkyProxy::bind(e2::ICommandBuffer* buffer, uint8_t frameIndex, bool shadows)
{

}

void e2::SkyProxy::invalidate(uint8_t frameIndex)
{

}
