#pragma once

#include <string>

namespace embedded {

bool present();
bool install(std::string& error);

}
