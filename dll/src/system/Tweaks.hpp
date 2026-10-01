#pragma once

namespace tweaks {

void timerResolution(bool on);
void highPriority(bool on);
void noPowerThrottling(bool on);
void inputBoost(bool on);
bool wantsInputBoost();
void threadBoost(bool on);
void restore();

}
