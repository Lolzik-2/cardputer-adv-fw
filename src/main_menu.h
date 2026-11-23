#pragma once
#include <M5Cardputer.h>
#include <stdint.h>

enum class AppScreen {
    MAIN_MENU,
    IR_MENU,
    WIFI_MENU,
    BLE_MENU,
    SETTINGS_MENU
};

struct NavInput {
    bool up = false;
    bool down = false;
    bool select = false;
    bool back = false;
};

// Жёлтая тема как на фото
static constexpr uint16_t THEME_BG        = 0xF5E0;
static constexpr uint16_t THEME_FG        = 0x0000;
static constexpr uint16_t THEME_BORDER    = 0x0000;
static constexpr uint16_t THEME_HIGHLIGHT = 0xFFE0;

NavInput readNavInput();
void drawMainMenu(int selectedIndex);
