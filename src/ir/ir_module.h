#pragma once
#include <M5Cardputer.h>
#include <IRremote.hpp>
#include "../main_menu.h"

class IrModule {
public:
    static constexpr int IR_TX_PIN = 44;

    enum class IrProto {
        NEC_CODE,
        SAMSUNG_CODE,
        LG_CODE,
        SONY_CODE,
        RC5_CODE,
        RC6_CODE
    };

    struct TvPowerCode {
        const char* brand;
        IrProto proto;
        uint32_t address;
        uint32_t command;
        uint8_t bits;
    };

    void enter() {
        running_ = false;
        currentCodeIdx_ = 0;
        sentCount_ = 0;
        initTx();
        draw();
    }

    bool update(const NavInput& nav) {
        if (nav.back) {
            running_ = false;
            return true;
        }

        if (nav.select) {
            running_ = !running_;
            draw();
        }

        if (running_) {
            stepSend();
        }

        return false;
    }

private:
    bool irTxInitialized_ = false;
    bool running_ = false;
    int currentCodeIdx_ = 0;
    uint32_t sentCount_ = 0;
    unsigned long lastSendMs_ = 0;

    static constexpr unsigned long SEND_INTERVAL_MS = 110;
    static const TvPowerCode TV_CODES[];
    static const int TV_CODE_COUNT;

    void initTx() {
        if (!irTxInitialized_) {
            IrSender.begin(IR_TX_PIN, DISABLE_LED_FEEDBACK);
            irTxInitialized_ = true;
        }
    }

    void sendOne(const TvPowerCode& c) {
        switch (c.proto) {
            case IrProto::NEC_CODE:
                IrSender.sendNEC(static_cast<uint16_t>(c.address),
                                   static_cast<uint16_t>(c.command), 1);
                break;
            case IrProto::SAMSUNG_CODE:
                IrSender.sendSamsung(static_cast<uint16_t>(c.address),
                                       static_cast<uint16_t>(c.command), 1);
                break;
            case IrProto::LG_CODE:
                IrSender.sendLG(static_cast<uint16_t>(c.address),
                                  static_cast<uint16_t>(c.command), 1);
                break;
            case IrProto::SONY_CODE:
                IrSender.sendSony(static_cast<uint16_t>(c.address),
                                    static_cast<uint8_t>(c.command), 2, c.bits);
                break;
            case IrProto::RC5_CODE:
                IrSender.sendRC5(static_cast<uint8_t>(c.address),
                                   static_cast<uint8_t>(c.command), 1, true);
                break;
            case IrProto::RC6_CODE:
                IrSender.sendRC6(static_cast<uint8_t>(c.address),
                                   static_cast<uint8_t>(c.command), 1, true);
                break;
        }
    }

    void stepSend() {
        unsigned long now = millis();
        if (now - lastSendMs_ < SEND_INTERVAL_MS) {
            return;
        }
        lastSendMs_ = now;

        const TvPowerCode& code = TV_CODES[currentCodeIdx_];
        sendOne(code);
        sentCount_++;

        currentCodeIdx_ = (currentCodeIdx_ + 1) % TV_CODE_COUNT;
        drawProgressOnly(code);
    }

    void draw() {
        auto& d = M5Cardputer.Display;
        d.fillScreen(THEME_BG);
        d.setTextColor(THEME_FG, THEME_BG);
        d.setTextSize(2);

        d.setCursor(12, 10);
        d.print("TV-B-Gone");

        d.drawRoundRect(10, 34, d.width() - 20, 28, 4, THEME_BORDER);
        if (running_) {
            d.fillRoundRect(11, 35, d.width() - 22, 26, 3, THEME_HIGHLIGHT);
            d.setTextColor(THEME_FG, THEME_HIGHLIGHT);
        }
        d.setCursor(18, 40);
        d.printf("State: %s", running_ ? "SENDING" : "READY");
        d.setTextColor(THEME_FG, THEME_BG);

        drawProgressOnly(TV_CODES[currentCodeIdx_]);

        d.setTextSize(1);
        d.setCursor(12, 122);
        d.print("Enter: Start/Stop | `/Bksp: Back");
    }

    void drawProgressOnly(const TvPowerCode& code) {
        auto& d = M5Cardputer.Display;
        d.fillRect(10, 68, d.width() - 20, 48, THEME_BG);
        d.setTextColor(THEME_FG, THEME_BG);
        d.setTextSize(2);

        d.setCursor(12, 70);
        d.printf("%-10s %d/%d", code.brand, currentCodeIdx_ + 1, TV_CODE_COUNT);

        d.setTextSize(1);
        d.setCursor(12, 98);
        d.printf("Sent: %lu  |  Addr:0x%X Cmd:0x%X",
                 static_cast<unsigned long>(sentCount_),
                 static_cast<unsigned int>(code.address),
                 static_cast<unsigned int>(code.command));
    }
};

const IrModule::TvPowerCode IrModule::TV_CODES[] = {
    {"Sony",      IrProto::SONY_CODE,    0x01, 0x15, 12},
    {"Sony-15",   IrProto::SONY_CODE,    0x97, 0x15, 15},
    {"Sony-20",   IrProto::SONY_CODE,    0x1A, 0x15, 20},
    {"Samsung",   IrProto::SAMSUNG_CODE, 0x07, 0x02, 32},
    {"Samsung2",  IrProto::SAMSUNG_CODE, 0x07, 0x0F, 32},
    {"Samsung3",  IrProto::SAMSUNG_CODE, 0x06, 0x02, 32},
    {"LG",        IrProto::LG_CODE,      0x04, 0x08, 32},
    {"LG-2",      IrProto::LG_CODE,      0x04, 0xC4, 32},
    {"LG-3",      IrProto::LG_CODE,      0x20, 0x08, 32},
    {"NEC-TV1",   IrProto::NEC_CODE,     0x00, 0x12, 32},
    {"NEC-TV2",   IrProto::NEC_CODE,     0x04, 0x08, 32},
    {"NEC-TV3",   IrProto::NEC_CODE,     0x08, 0x0C, 32},
    {"Hisense",   IrProto::NEC_CODE,     0x00, 0x0D, 32},
    {"Hisense2",  IrProto::NEC_CODE,     0x02, 0x0D, 32},
    {"TCL",       IrProto::NEC_CODE,     0x00, 0x15, 32},
    {"TCL-2",     IrProto::NEC_CODE,     0x01, 0x15, 32},
    {"Skyworth",  IrProto::NEC_CODE,     0x0E, 0x0A, 32},
    {"Haier",     IrProto::NEC_CODE,     0x10, 0x0C, 32},
    {"Changhong", IrProto::NEC_CODE,     0x05, 0x03, 32},
    {"Konka",     IrProto::NEC_CODE,     0x02, 0x0A, 32},
    {"Vizio",     IrProto::NEC_CODE,     0x00, 0x18, 32},
    {"Sharp",     IrProto::NEC_CODE,     0x12, 0x11, 32},
    {"Toshiba",   IrProto::NEC_CODE,     0x02, 0x16, 32},
    {"Hitachi",   IrProto::NEC_CODE,     0x00, 0x16, 32},
    {"Sanyo",     IrProto::NEC_CODE,     0x00, 0x1A, 32},
    {"Funai",     IrProto::NEC_CODE,     0x00, 0x1C, 32},
    {"Orion",     IrProto::NEC_CODE,     0x00, 0x1E, 32},
    {"Philips",   IrProto::RC5_CODE,     0x00, 0x0C, 13},
    {"Grundig",   IrProto::RC5_CODE,     0x00, 0x0C, 13},
    {"Thomson",   IrProto::RC6_CODE,     0x00, 0x0C, 20},
    {"PhilipsRC6",IrProto::RC6_CODE,     0x00, 0x0C, 20},
    {"GenNEC-1",  IrProto::NEC_CODE,     0x01, 0x02, 32},
    {"GenNEC-2",  IrProto::NEC_CODE,     0x02, 0x02, 32},
    {"GenNEC-3",  IrProto::NEC_CODE,     0x03, 0x02, 32},
    {"GenNEC-4",  IrProto::NEC_CODE,     0x04, 0x12, 32},
    {"GenNEC-5",  IrProto::NEC_CODE,     0x06, 0x12, 32},
    {"GenNEC-6",  IrProto::NEC_CODE,     0x07, 0x12, 32},
    {"GenNEC-7",  IrProto::NEC_CODE,     0x08, 0x12, 32},
    {"GenNEC-8",  IrProto::NEC_CODE,     0x09, 0x12, 32},
    {"GenNEC-9",  IrProto::NEC_CODE,     0x0A, 0x12, 32},
    {"GenNEC-10", IrProto::NEC_CODE,     0x0B, 0x12, 32},
    {"GenNEC-11", IrProto::NEC_CODE,     0x0C, 0x12, 32},
    {"GenNEC-12", IrProto::NEC_CODE,     0x0D, 0x12, 32},
    {"GenNEC-13", IrProto::NEC_CODE,     0x0E, 0x12, 32},
    {"GenNEC-14", IrProto::NEC_CODE,     0x0F, 0x12, 32},
    {"GenNEC-15", IrProto::NEC_CODE,     0x80, 0x0C, 32},
    {"GenNEC-16", IrProto::NEC_CODE,     0x80, 0x12, 32},
    {"GenNEC-17", IrProto::NEC_CODE,     0xFE, 0x01, 32},
    {"GenNEC-18", IrProto::NEC_CODE,     0xFF, 0x00, 32}
};

const int IrModule::TV_CODE_COUNT = sizeof(IrModule::TV_CODES) / sizeof(IrModule::TV_CODES[0]);
