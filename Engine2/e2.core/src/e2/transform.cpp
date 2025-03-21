
#include "e2/transform.hpp"

e2::Transform::Transform()
{

}

e2::Transform::~Transform()
{

}

glm::mat4 e2::Transform::getTransformMatrix(TransformSpace space /*= TS_Local*/)
{
	if (m_dirty)
	{
		glm::mat4 translateMatrix = glm::translate(glm::mat4(1.0f), m_translation);
		glm::mat4 rotateMatrix = glm::mat4_cast(m_rotation);
		glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), m_scale);

		m_transform = translateMatrix * rotateMatrix * scaleMatrix;

		m_dirty = false;
	}

	if (space == TransformSpace::Local || !m_transformParent)
	{
		return m_transform;
	}
	else
	{
		return m_transform * m_transformParent->getTransformMatrix(TransformSpace::World);
	}
}


e2::Transform* e2::Transform::getTransformParent()
{
	return m_transformParent;
}

void e2::Transform::setTransformParent(Transform* newParent)
{
	m_transformParent = newParent;
}

void e2::Transform::lookAt(glm::vec3 const& target, TransformSpace space)
{
	setRotation(glm::quatLookAt(glm::normalize(target - getTranslation(space)), e2::worldUpf()), space);
}

glm::vec3 e2::Transform::getForward(e2::TransformSpace space)
{
	glm::vec3 worldFwd = e2::worldForwardf();
	glm::vec4 fwd = getTransformMatrix(space) * glm::vec4(worldFwd.x, worldFwd.y, worldFwd.z, 0.0);
	return glm::vec3(fwd.x, fwd.y, fwd.z);
}

glm::vec3 e2::Transform::getTranslation(TransformSpace space /*= TS_Local*/)
{
	if (space == e2::TransformSpace::Local || !m_transformParent)
	{
		return m_translation;
	}
	else
	{
		return m_transformParent->getTransformMatrix(e2::TransformSpace::World) * glm::vec4(m_translation, 1.0f);
	}
}

void e2::Transform::setTranslation(glm::vec3 newTranslation, TransformSpace space /*= TS_Local*/)
{
	if (space == e2::TransformSpace::Local || !m_transformParent)
	{
		m_translation = newTranslation;
	}
	else
	{
		m_translation = glm::inverse(m_transformParent->getTransformMatrix(e2::TransformSpace::World)) * glm::vec4(m_translation, 1.0f);
	}

	m_dirty = true;
}

void e2::Transform::translate(glm::vec3 offset, TransformSpace space /*= TS_Local*/)
{
	if (space == e2::TransformSpace::Local || !m_transformParent)
	{
		m_translation += offset;
	}
	else
	{
		m_translation = m_translation + glm::vec3(glm::inverse(m_transformParent->getTransformMatrix(e2::TransformSpace::World)) * glm::vec4(offset, 1.0f));
	}

	m_dirty = true;
}

glm::quat e2::Transform::getRotation(TransformSpace space /*= TS_Local*/)
{
	if (space == e2::TransformSpace::Local || !m_transformParent)
	{
		return m_rotation;
	}
	else
	{
		return m_rotation * m_transformParent->getRotation(e2::TransformSpace::World);
	}
}

void e2::Transform::setRotation(glm::quat newRotation, TransformSpace space /*= TS_Local*/)
{
	if (space == e2::TransformSpace::Local || !m_transformParent)
	{
		m_rotation = newRotation;
	}
	else 
	{
		m_rotation = newRotation * glm::inverse(m_transformParent->getRotation(e2::TransformSpace::World));
	}

	m_dirty = true;
}

void e2::Transform::rotate(glm::quat offsetRotation, e2::TransformSpace space)
{
	if (space == e2::TransformSpace::Local || !m_transformParent)
	{
		m_rotation = offsetRotation * m_rotation;
	}
	else 
	{
		glm::quat worldRotation = getRotation(e2::TransformSpace::World);
		m_rotation = worldRotation * offsetRotation * glm::inverse(worldRotation) * m_rotation;
	}

	m_dirty = true;
}



glm::vec3 e2::Transform::getScale(TransformSpace space /*= TS_Local*/)
{
	if (space == e2::TransformSpace::Local || !m_transformParent)
	{
		return m_scale;
	}
	else
	{
		return m_transformParent->getScale(e2::TransformSpace::World) * m_scale;
	}
}

void e2::Transform::setScale(glm::vec3 newScale, TransformSpace space /*= TS_Local*/)
{
	if (space == e2::TransformSpace::Local || !m_transformParent)
	{
		m_scale = newScale;
	}
	else
	{
		m_scale = newScale / m_transformParent->getScale(e2::TransformSpace::World);
	}

	m_dirty = true;
}

void e2::Transform::scale(glm::vec3 offset, e2::TransformSpace space)
{
	glm::vec3 s = getScale(space);
	s *= offset;
	setScale(s, space);
}


glm::dvec3 e2::worldUp()
{
	return {0.0, -1.0, 0.0};
}

glm::dvec3 e2::worldRight()
{
	return {1.0, 0.0, 0.0};
}

glm::dvec3 e2::worldForward()
{
	return {0.0, 0.0, -1.0};
}

glm::vec3 e2::worldUpf()
{
	return worldUp();
}

glm::vec3 e2::worldRightf()
{
	return worldRight();
}

glm::vec3 e2::worldForwardf()
{
	return worldForward();
}
