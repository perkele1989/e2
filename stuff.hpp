
namespace e2 
{
	/**
	* The render proxy, of a given mesh in a given world.
	* Belongs to the world itself, separate to any renderer or active camera.
	* 
	* @tags(arena, arenaSize=16384)
	*/
	class RenderProxy
	{
		E2_CLASS_DECLARATION()
		public:
		
		
		protected:
	};

	/**
	* Contains render proxies for a world.
	* Belongs to the world itself, separate to any renderer or active camera.
	* 
	* @tags(arena, arenaSize=16)
	*/
   class RenderSet : public e2::ManagedObject, public e2::RenderCallbacks
   {
	   E2_CLASS_DECLARATION()
   public:
   
   
   
   protected:
	   std::set<RenderProxy*> m_proxies;
   };
}


/*

RenderManager 
    e2::IDescriptorSetLayout *rendererLayout ;
    e2::IDescriptorSetLayout *meshLayout; // this is a cache of layouts, depending on things like skin etc.

Renderer 
    // contains camera matrices, lights, etc.
    e2::IDescriptorSet *rendererSet { rendererLayout }

ShaderModel 
    e2::IDescriptorSetLayout materialLayout;

RenderSet

MaterialProxy
    // contains albeod, normal etc.
    // likely to be updated just once, however can be updated every frame 
    e2::IDescriptorSet *materialProxySet { materialLayout }

RenderProxy
    // contains model matrix, skin, etc.
    // likely to be updated every frame 
    e2::IDescriptorSet *meshSet { meshLayout }



*/