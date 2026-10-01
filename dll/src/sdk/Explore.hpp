#pragma once

#include <string>

namespace explore {

// development builds only (-DMOCHI_DEV=ON): runs a script from the explore folder next to the dll, see tools/explore
void run(const std::string& name);

}
