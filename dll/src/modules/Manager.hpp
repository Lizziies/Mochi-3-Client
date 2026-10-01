#pragma once

#include "Module.hpp"

#include <memory>
#include <string>
#include <vector>

namespace modules {

void init();
void shutdown();

const std::vector<std::unique_ptr<Module>>& all();
Module* find(const std::string& name);

template <class T>
T* get() {
    for (auto& m : all())
        if (auto* p = dynamic_cast<T*>(m.get())) return p;
    return nullptr;
}

void frame(ImDrawList* hud);
void dispatchKey(KeyEvent& ev);
void dispatchMouse(MouseEvent& ev);
void dispatchServer(const ServerEvent& ev);
void refreshSigs();
float costMs();

struct Motion {
    int x = 0;
    int y = 0;
};
Motion mouseDelta();

}
