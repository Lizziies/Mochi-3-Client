#pragma once

#include <functional>

namespace bg {

void run(std::function<void()> work);
void drain(int timeoutMs);

}
