
#pragma once

#include <e2/export.hpp>
#include <e2/context.hpp>
#include <e2/utils.hpp>

#include <e2/renderer/camera.hpp>

#include <e2/rhi/rendercontext.hpp>
#include <e2/rhi/threadcontext.hpp>
#include <e2/renderer/shared.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/perpendicular.hpp>

namespace e2
{
	class Camera;  
	class Session;  
	class IDescriptorSet;


	enum class RendererFlags : uint8_t
	{
		None = 0,

		/** Mesh is rendered in shadow (depth-only) pass */
		Shadow = 1 << 0,

		/** Mesh is skinned */
		Skin = 1 << 1,


	};

	struct E2_API PushConstantData
	{
		glm::mat4 normalMatrix;
		glm::uvec2 resolution;
	};

	struct E2_API RendererData
	{
		alignas(16) glm::mat4 viewMatrix;
		alignas(16) glm::mat4 projectionMatrix;
		alignas(16) glm::vec4 time; // t, sin(t), cos(t), tan(t)
		alignas(16) glm::vec4 sun1; // sun direction.xyz, ???
		alignas(16) glm::vec4 sun2; // sun color.rgb, sun strength
		alignas(16) glm::vec4 ibl1; // ibl strength, ???, ???, ???
		alignas(16) glm::vec4 cameraPosition;
		
	};

	E2_API bool isCounterClockwise(glm::vec2 const& a, glm::vec2 const& b, glm::vec2 const& c);

	struct E2_API Line2D
	{
		glm::vec2 start;
		glm::vec2 end;

		bool intersects(Line2D const& other);
	};

	struct E2_API Aabb2D
	{
		glm::vec2 min{};
		glm::vec2 max{};

		e2::StackVector<glm::vec2, 4> points();

		void push(glm::vec2 const& point);

		bool isWithin(glm::vec2 const& point) const;
	};

	struct E2_API Ray2D
	{
		bool edgeTest(glm::vec2 const& circleOrigin, float circleRadius) const;
		bool edgeTest(glm::vec2 const& point) const;

		glm::vec2 position{};
		glm::vec2 parallel{};
		glm::vec2 perpendicular{};
	};

	E2_API bool boxRayIntersection2D(Aabb2D box, Ray2D ray);

	struct E2_API ConvexShape2D
	{
		e2::StackVector<glm::vec2, 32> points;
		e2::StackVector<e2::Ray2D, 32> rays;
	};

	// A flat frustum, it represents the 4 corners of the camera frustum, projected to the flat xz plane at y=0.0
	// Can be extended to work as a generic 2D convex hull but we dont need that right now 
	struct E2_API Viewpoints2D
	{
		Viewpoints2D();
		Viewpoints2D(glm::vec2 const& _resolution, e2::RenderView const& _view);

		ConvexShape2D combine(Viewpoints2D const& other);

		union
		{
			struct
			{
				glm::vec2 topLeft;
				glm::vec2 topRight;
				glm::vec2 bottomLeft;
				glm::vec2 bottomRight;
			};
			glm::vec2 corners[4];
		};

		// these are the inputs to the viewpoints generation, not neccessarily set 
		e2::RenderView view;
		glm::vec2 resolution;

		// The following are derivatives 
		glm::vec2 center;
		Ray2D leftRay;
		Ray2D rightRay;
		Ray2D topRay;
		Ray2D bottomRay;

		bool isWithin(glm::vec2 const& point) const;
		bool isWithin(glm::vec2 const& circleOrigin, float circeRadius) const;

		bool test(e2::Aabb2D const& aabb) const;

		void calculateDerivatives();

		e2::Aabb2D toAabb() const;
	};

	struct E2_API  Line
	{
		glm::vec3 color;
		glm::vec3 start;
		glm::vec3 end;
	};

	class E2_API Renderer : public Context, public e2::ManagedObject
	{
		ObjectDeclaration()
	public:
		Renderer(e2::Session* session, glm::uvec2 const& resolution);
		virtual ~Renderer();

		virtual Engine* engine() override;

		// records and queues the current frame
		void recordFrame(double deltaTime);

		
		e2::Session* session() const;

		void setView(e2::RenderView const& renderView);

		inline e2::ITexture* colorTarget() const
		{
			return m_renderBuffers[frontBuffer()].colorTexture;
		}

		inline glm::uvec2 resolution() const
		{
			return m_resolution;
		}

		e2::Viewpoints2D const&  viewpoints() const;

		void debugLine(glm::vec3 const& color, glm::vec3 const& start, glm::vec3 const& end);
		void debugLine(glm::vec3 const& color, glm::vec2 const& start, glm::vec2 const& end);

		e2::RenderView const& view() 
		{
			return m_view;
		}

		void setEnvironment(e2::ITexture* irradiance, e2::ITexture* radiance);
		void setSun(glm::vec3 const& dir, glm::vec3 const& color, float strength);

		void setOutlineTextures(e2::ITexture* textures[2]);

	protected:
		e2::Session* m_session{};

		e2::RenderView m_view{};
		e2::Viewpoints2D m_viewPoints{};

		glm::uvec2 m_resolution{};

		struct
		{
			e2::ITexture* colorTexture{};
			e2::ITexture* positionTexture{};

			e2::ITexture* depthTexture{};
			e2::IRenderTarget* renderTarget{};
			e2::Pair<e2::IDescriptorSet*> sets{ nullptr };
		} m_renderBuffers[2];

		uint8_t m_backBuffer{};

		void swapRenderBuffers();
		uint8_t frontBuffer() const;

		// 
		e2::Pair<e2::ICommandBuffer*> m_commandBuffers {nullptr};

		RendererData m_rendererData;
		e2::Pair<e2::IDataBuffer*> m_rendererBuffers;
		

		e2::PipelineSettings m_defaultSettings;
		

		e2::IShader* m_lineFragmentShader{};
		e2::IShader* m_lineVertexShader{};
		e2::IPipeline* m_linePipeline{};

		e2::ITexture* m_irradiance{};
		e2::ITexture* m_radiance{};

		e2::ITexture* m_outlineTextures[2];

		float m_iblStrength{ 1.0f };

		glm::vec3 m_sunColor;
		glm::vec3 m_sunDirection{};
		float m_sunStrength{ 1.0f };

		// @todo set configurable or just change to dynamic because this is debug shit anyway
		std::vector<Line> m_debugLines;


	};
}

EnumFlagsDeclaration(e2::RendererFlags)

#include "renderer.generated.hpp"