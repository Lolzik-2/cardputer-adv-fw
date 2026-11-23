#include "main_menu.h"

static const char* MAIN_ITEMS[] = {
    "Infrared",
    "Wi-Fi",
    "BLE Spam",
    "Settings"
};
static constexpr int MAIN_ITEM_COUNT = sizeof(MAIN_ITEMS) / sizeof(MAIN_ITEMS[0]);

// Навигация без Fn:
// Вверх: ';' или 'w'
// Вниз:  '.' или '/' или 's'
// Выбор: Enter
// Назад: '`' или Backspace
NavInput readNavInput() {
    NavInput nav;
    auto& kb = M5Cardputer.Keyboard;

    if (!kb.isChange()) {
        return nav;
    }

    if (kb.isKeyPressed(';') || kb.isKeyPressed('w')) {
        nav.up = true;
    }
    if (kb.isKeyPressed('.') || kb.isKeyPressed('/') || kb.isKeyPressed('s')) {
        nav.down = true;
    }
    if (kb.isKeyPressed('\n') || kb.isKeyPressed('\r')) {
        nav.select = true;
    }
    if (kb.isKeyPressed('`') || kb.isKeyPressed('\b')) {
        nav.back = true;
    }

    return nav;
}

void drawMainMenu(int selectedIndex) {
    auto& d = M5Cardputer.Display;
    d.fillScreen(THEME_BG);
    d.setTextColor(THEME_FG, THEME_BG);
    d.setTextSize(2);

    // В главном меню без индикатора батареи — только чистый список как на фото
    const int startY  = 12;
    const int itemH   = 27;
    const int gapY    = 4;
    const int marginX = 10;
    const int boxW    = d.width() - marginX * 2;

    for (int i = 0; i < MAIN_ITEM_COUNT; ++i) {
        int y = startY + i * (itemH + gapY);
        bool selected = (i == selectedIndex);

        if (selected) {
            d.fillRoundRect(marginX, y, boxW, itemH, 4, THEME_HIGHLIGHT);
            d.drawRoundRect(marginX, y, boxW, itemH, 4, THEME_BORDER);
            d.drawRoundRect(marginX + 1, y + 1, boxW - 2, itemH - 2, 3, THEME_BORDER);
            d.setTextColor(THEME_FG, THEME_HIGHLIGHT);
        } else {
            d.drawRoundRect(marginX, y, boxW, itemH, 4, THEME_BORDER);
            d.setTextColor(THEME_FG, THEME_BG);
        }

        d.setCursor(marginX + 10, y + 6);
        d.print(selected ? "> " : "* ");
        d.print(MAIN_ITEMS[i]);
    }
}
