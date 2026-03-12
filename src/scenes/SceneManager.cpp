#include "SceneManager.hpp"
#include "SceneContext.hpp"

void SceneManager::start(std::unique_ptr<IScene> initial, SceneContext& ctx) {
    m_current = std::move(initial);
    m_current->onEnter(ctx);
}

void SceneManager::replace(std::unique_ptr<IScene> next) {
    m_pending = std::move(next);
}

void SceneManager::handleEvent(SDL_Event& ev, SceneContext& ctx) {
    if (m_current) m_current->handleEvent(ev, ctx);
}

void SceneManager::update(float dt, SceneContext& ctx) {
    // Apply any pending scene transition
    if (m_pending) {
        if (m_current) m_current->onExit();
        m_current = std::move(m_pending);
        m_current->onEnter(ctx);
    }
    if (m_current) m_current->update(dt, ctx);
}

void SceneManager::render(SceneContext& ctx) {
    if (m_current) m_current->render(ctx);
}
