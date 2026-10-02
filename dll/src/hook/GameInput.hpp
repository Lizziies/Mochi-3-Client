#pragma once

namespace gameinput {

void tick();
bool active();
const char* status();

void hold(int vk);
void drop(int vk);
void scaleMouse(float factor);

void beginFrame();
void publish();

}
