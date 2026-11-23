#pragma once
#include <M5Cardputer.h>
#include "../main_menu.h"

class SettingsModule {
public:
    void enter() {
        index_ = 0;
        subView_ = SubView::MENU;
        brightness_ = M5Cardputer.Display.getBrightness();
        draw();
    }

    bool update(const NavInput& nav) {
        if (subView_ == SubView::BATTERY_INFO) {
            if (nav.back || nav.select) {
                subView_ = SubView::MENU;
                draw();
            }
            return false;
        }

        if (nav.back) {
            return true;
        }

        if (nav.up) {
            index_ = (index_ - 1 + ITEM_COUNT) % ITEM_COUNT;
            draw();
        }
        if (nav.down) {
            index_ = (index_ + 1) % ITEM_COUNT;
            draw();
        }
        if (nav.select) {
            handleSelect();
        }
        return false;
    }

private:
    enum class SubView { MENU, BATTERY_INFO };
    static constexpr int ITEM_COUNT = 4;

    int index_ = 0;
    uint8_t brightness_ = 128;
    SubView subView_ = SubView::MENU;

    void handleSelect() {
        switch (index_) {
            case 0: // Battery Info
                subView_ = SubView::BATTERY_INFO;
                drawBatteryInfo();
                break;

            case 1: // Deep Sleep
                M5Cardputer.Display.fillScreen(THEME_BG);
                M5Cardputer.Display.setTextColor(THEME_FG, THEME_BG);
                M5Cardputer.Display.setTextSize(2);
                M5Cardputer.Display.setCursor(16, 52);
                M5Cardputer.Display.print("Deep Sleep...");
                delay(400);
                M5Cardputer.Power.deepSleep();
                break;

            case 2: // Reboot
                ESP.restart();
                break;

            case 3: // Brightness
                if (brightness_ < 64) brightness_ = 128;
                else if (brightness_ < 160) brightness_ = 192;
                else if (brightness_ < 230) brightness_ = 255;
                else brightness_ = 40;
                M5Cardputer.Display.setBrightness(brightness_);
                draw();
                break;
        }
    }

    void drawBatteryInfo() {
        auto& d = M5Cardputer.Display;
        d.fillScreen(THEME_BG);
        d.setTextColor(THEME_FG, THEME_BG);
        d.setTextSize(2);

        int level = M5Cardputer.Power.getBatteryLevel();
        int mv = M5Cardputer.Power.getBatteryVoltage();
        bool charging = M5Cardputer.Power.isCharging();

        d.setCursor(12, 12);
        d.print("Battery Info");
        d.drawFastHLine(12, 34, d.width() - 24, THEME_BORDER);

        d.setCursor(12, 44);
        d.printf("Level : %d%%", level);

        d.setCursor(12, 68);
        d.printf("Volt  : %.2fV", mv / 1000.0f);

        d.setCursor(12, 92);
        d.printf("Status: %s", charging ? "Charging" : "Discharging");

        d.setTextSize(1);
        d.setCursor(12, 120);
        d.print("Enter / ` / Backspace -> Back");
    }

    void draw() {
        auto& d = M5Cardputer.Display;
        d.fillScreen(THEME_BG);
        d.setTextSize(2);

        const char* labels[ITEM_COUNT] = {
            "Battery Info",
            "Deep Sleep",
            "Reboot",
            "Brightness"
        };

        for (int i = 0; i < ITEM_COUNT; ++i) {
            int y = 10 + i * 30;
            bool sel = (i == index_);

            if (sel) {
                d.fillRoundRect(10, y, d.width() - 20, 26, 4, THEME_HIGHLIGHT);
                d.drawRoundRect(10, y, d.width() - 20, 26, 4, THEME_BORDER);
                d.setTextColor(THEME_FG, THEME_HIGHLIGHT);
            } else {
                d.setTextColor(THEME_FG, THEME_BG);
            }

            d.setCursor(16, y + 5);
            d.print(sel ? "> " : "  ");
            d.print(labels[i]);

            if (i == 3) {
                int pct = (brightness_ * 100) / 255;
                d.printf(" %d%%", pct);
            }
        }
    }
};
