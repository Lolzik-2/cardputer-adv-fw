#include <M5Cardputer.h>
#include "main_menu.h"
#include "ir/ir_module.h"
#include "wifi/wifi_module.h"
#include "ble/ble_spam.h"
#include "settings/settings_module.h"

static AppScreen currentScreen = AppScreen::MAIN_MENU;
static int mainIndex = 0;

static IrModule irMod;
static WifiModule wifiMod;
static BleSpamModule bleMod;
static SettingsModule settingsMod;

static NavInput readCombinedNavInput() {
    NavInput nav = readNavInput();

    static bool prevSelect = false;
    static bool prevBack = false;

    bool curSelect = nav.select;
    bool curBack = nav.back;

    if (M5Cardputer.Keyboard.isPressed()) {
        auto ks = M5Cardputer.Keyboard.keysState();
        if (ks.enter) {
            curSelect = true;
        }
        if (ks.del) {
            curBack = true;
        }
        for (char c : ks.word) {
            if (c == '\r' || c == '\n') {
                curSelect = true;
            } else if (c == '`' || c == '\b') {
                curBack = true;
            }
        }
    }

    if (M5Cardputer.BtnA.isPressed()) {
        curSelect = true;
    }

    if (curSelect && !prevSelect) {
        nav.select = true;
    }
    if (curBack && !prevBack) {
        nav.back = true;
    }

    prevSelect = curSelect;
    prevBack = curBack;

    return nav;
}

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg);
    M5Cardputer.Display.setRotation(0);
    M5Cardputer.Display.setBrightness(140);

    drawMainMenu(mainIndex);
}

void loop() {
    M5Cardputer.update();
    NavInput nav = readCombinedNavInput();

    switch (currentScreen) {
        case AppScreen::MAIN_MENU: {
            if (nav.up) {
                mainIndex = (mainIndex - 1 + 4) % 4;
                drawMainMenu(mainIndex);
            }
            if (nav.down) {
                mainIndex = (mainIndex + 1) % 4;
                drawMainMenu(mainIndex);
            }
            if (nav.select) {
                if (mainIndex == 0) {
                    currentScreen = AppScreen::IR_MENU;
                    irMod.enter();
                } else if (mainIndex == 1) {
                    currentScreen = AppScreen::WIFI_MENU;
                    wifiMod.enter();
                } else if (mainIndex == 2) {
                    currentScreen = AppScreen::BLE_MENU;
                    bleMod.enter();
                } else if (mainIndex == 3) {
                    currentScreen = AppScreen::SETTINGS_MENU;
                    settingsMod.enter();
                }
            }
            break;
        }
        case AppScreen::IR_MENU:
            if (irMod.update(nav)) {
                currentScreen = AppScreen::MAIN_MENU;
                drawMainMenu(mainIndex);
            }
            break;
        case AppScreen::WIFI_MENU:
            if (wifiMod.update(nav)) {
                currentScreen = AppScreen::MAIN_MENU;
                drawMainMenu(mainIndex);
            }
            break;
        case AppScreen::BLE_MENU:
            if (bleMod.update(nav)) {
                currentScreen = AppScreen::MAIN_MENU;
                drawMainMenu(mainIndex);
            }
            break;
        case AppScreen::SETTINGS_MENU:
            if (settingsMod.update(nav)) {
                currentScreen = AppScreen::MAIN_MENU;
                drawMainMenu(mainIndex);
            }
            break;
    }

    delay(10);
}
