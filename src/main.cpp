#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>

using namespace geode::prelude;

namespace {
    // true, пока клавиша Z физически зажата
    bool g_zHeld = false;

    // Не реагируем на клавиши, если открыта пауза или экран конца уровня
    bool menuIsOpen() {
        auto scene = CCDirector::get()->getRunningScene();
        if (!scene) return true;
        return scene->getChildByType<PauseLayer>(0) != nullptr
            || scene->getChildByType<EndLevelLayer>(0) != nullptr;
    }
}

class $modify(QuickResetPlayLayer, PlayLayer) {
    struct Fields {
        // Если мы сделали ресет сами во время анимации смерти, то игра всё равно
        // позже вызовет свой "отложенный" ресет - его нужно проглотить,
        // иначе новая попытка сбросится ещё раз.
        float skipDelayedReset = 0.f;
        float checkpointTimer = 0.f;
    };

    // Быстрый ресет по клавише R
    void quickReset() {
        bool dead = m_player1 && m_player1->m_isDead;
        if (dead) {
            m_fields->skipDelayedReset = 2.f;
        }

        bool fromStart = m_isPracticeMode
            && Mod::get()->getSettingValue<bool>("practice-reset-from-start");

        if (fromStart) {
            this->resetLevelFromStart();
        } else {
            this->resetLevel();
        }
    }

    void delayedResetLevel() {
        if (m_fields->skipDelayedReset > 0.f) {
            m_fields->skipDelayedReset = 0.f;
            return;
        }
        PlayLayer::delayedResetLevel();
    }

    // Каждый кадр: пока зажат Z в practice-режиме - ставим чекпоинты
    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);

        if (m_fields->skipDelayedReset > 0.f) {
            m_fields->skipDelayedReset -= dt;
        }

        if (!g_zHeld || !m_isPracticeMode) {
            m_fields->checkpointTimer = 0.f;
            return;
        }
        if (!Mod::get()->getSettingValue<bool>("hold-checkpoints")) return;
        if (!m_player1 || m_player1->m_isDead) return;
        if (menuIsOpen()) return;

        m_fields->checkpointTimer += dt;
        float interval = static_cast<float>(
            Mod::get()->getSettingValue<double>("checkpoint-interval")
        );

        if (m_fields->checkpointTimer >= interval) {
            m_fields->checkpointTimer = 0.f;
            this->markCheckpoint();
        }
    }
};

class $modify(QuickResetKeyboard, CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(enumKeyCodes key, bool down, bool repeat, double timestamp) {
        if (key == KEY_Z) {
            g_zHeld = down;
        }
        else if (key == KEY_R && down && !repeat) {
            if (Mod::get()->getSettingValue<bool>("quick-reset")) {
                auto pl = PlayLayer::get();
                if (pl && pl->m_player1 && !menuIsOpen()) {
                    static_cast<QuickResetPlayLayer*>(pl)->quickReset();
                    return true;
                }
            }
        }
        return CCKeyboardDispatcher::dispatchKeyboardMSG(key, down, repeat, timestamp);
    }
};
