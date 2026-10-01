#pragma once

#include <string>

namespace inject {

bool ours();
bool focused();

void key(int vk, bool down);
void tap(int vk);
void tapLater(int vk);
void say(const std::string& text, int chatKey = 'T');
void shutdown();

}
