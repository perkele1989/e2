
#pragma once 

#include <e2/export.hpp>

#include <e2/utils.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace e2
{
	enum class TransformSpace : uint8_t
	{
		Local,
		World
	};

	E2_API glm::dvec3 worldUp();
	E2_API glm::dvec3 worldRight();
	E2_API glm::dvec3 worldForward();

	E2_API glm::vec3 worldUpf();
	E2_API glm::vec3 worldRightf();
	E2_API glm::vec3 worldForwardf();

	/** @tags(arena, arenaSize=16384) */
	class E2_API Transform : public e2::Object
	{
		ObjectDeclaration()
	public:
		Transform();
		virtual ~Transform();

		glm::mat4 getTransformMatrix(e2::TransformSpace space);
		
		glm::vec3 getTranslation(e2::TransformSpace space);
		void setTranslation(glm::vec3 newTranslation, e2::TransformSpace space);
		void translate(glm::vec3 offset, e2::TransformSpace space);

		glm::quat getRotation(e2::TransformSpace space);
		void setRotation(glm::quat newRotation, e2::TransformSpace space);
		void rotate(glm::quat offsetRotation, e2::TransformSpace space);

		glm::vec3 getScale(e2::TransformSpace space);
		void setScale(glm::vec3 newScale, e2::TransformSpace space);
		void scale(glm::vec3 offset, e2::TransformSpace space);

		Transform* getTransformParent();
		void setTransformParent(Transform* newParent);
		

		void lookAt(glm::vec3 const& target, e2::TransformSpace space);

	protected:
		Transform* m_transformParent{};

		// the local parts that make up the local transform
		glm::vec3 m_translation{};
		glm::vec3 m_scale{1.0f, 1.0f, 1.0f};
		glm::quat m_rotation{ glm::identity<glm::quat>() };


		// cached
		glm::mat4 m_transform;
		bool m_dirty{true};

	};
}

#include <transform.generated.hpp>