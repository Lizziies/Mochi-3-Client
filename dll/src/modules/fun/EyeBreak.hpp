#pragma once

#include "gui/Notify.hpp"
#include "modules/Module.hpp"
#include "render/Ui.hpp"

class EyeBreak : public Module {
public:
    EyeBreak()
        : Module("20-20-20", "Erinnert dich alle 20 Minuten, 20 Sekunden lang etwas 20 Fuß (6 m) Entferntes anzuschauen.",
                 Category::Fun, {"cosmetic"}) {}

    void onEnable() override { next_ = ui::time() + interval_.f * 60.0; }

    void onFrame() override {
        if (ui::time() < next_) return;
        next_ = ui::time() + interval_.f * 60.0;
        notify::push("Augenpause", "Schau 20 Sekunden lang in die Ferne.", notify::Kind::Info, 20.f);
    }

private:
    Setting& interval_ = slider("interval", "Alle (Minuten)", 20.f, 5.f, 60.f, "%.0f min");
    double next_ = 0;
};
