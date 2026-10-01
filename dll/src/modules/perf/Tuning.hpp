#pragma once

namespace perf {

void begin();
void apply();

void lowLatency();
void tearing();
void limit(float fps);

}
