#pragma once

#include "I18n.hpp"
#include "gui/Notify.hpp"
#include "modules/Module.hpp"
#include "render/Ui.hpp"

class EyeBreak : public Module {
public:
    EyeBreak()
        : Module("20-20-20", "Reminds you every 20 minutes to look at something 20 feet (6 m) away for 20 seconds.",
                 Category::Fun, {"cosmetic"}) {
        sub("Games");
    }

    void onEnable() override { last_ = ui::time(); }

    void onFrame() override {
        if (ui::time() - last_ < interval_.f * 60.0) return;
        last_ = ui::time();
        notify::push(i18n::tr("Eye break"), i18n::tr("Look into the distance for 20 seconds."), notify::Kind::Info, 20.f);
    }

private:
    Setting& interval_ = slider("interval", "Every (minutes)", 20.f, 5.f, 60.f, "%.0f min");
    double last_ = 0;
};
