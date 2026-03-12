#include "Entity.hpp"

void Entity::transformVerts() {
    m_worldVerts.resize(m_localVerts.size());
    for (size_t i = 0; i < m_localVerts.size(); ++i) {
        m_worldVerts[i] = m_localVerts[i].rotated(angle) + pos;
    }
}
