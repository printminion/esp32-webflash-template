// AUTO-GENERATED — do not edit by hand.
// Re-generate with: python scripts/generate_boards_config.py
window.BOARDS_CONFIG = [
  { id: "seeed_xiao_esp32c3", name: "Seeed XIAO ESP32-C3", icon: "🟢", meta: "ESP32-C3 · RISC-V · 4MB Flash", chipFamily: "ESP32-C3", sku: "113991054", url: "https://www.seeedstudio.com/Seeed-XIAO-ESP32C3-p-5431.html", image: "assets/boards/seeed_xiao_esp32c3.png" },
  { id: "seeed_xiao_esp32s3", name: "Seeed XIAO ESP32-S3", icon: "🔵", meta: "ESP32-S3 · Xtensa · 8MB Flash · Camera ready", chipFamily: "ESP32-S3", sku: "113991114", url: "https://www.seeedstudio.com/XIAO-ESP32S3-p-5627.html", image: "assets/boards/seeed_xiao_esp32s3.jpg" },
  { id: "seeed_xiao_esp32c6", name: "Seeed XIAO ESP32-C6", icon: "🟡", meta: "ESP32-C6 · RISC-V · WiFi 6 · Zigbee · Thread", chipFamily: "ESP32-C6", sku: "113991254", url: "https://www.seeedstudio.com/Seeed-Studio-XIAO-ESP32C6-p-5884.html", image: "assets/boards/seeed_xiao_esp32c6.jpg" },
  { id: "generic_esp32", name: "Generic ESP32", icon: "⚪", meta: "ESP32 · Xtensa Dual-core · DevKitC", chipFamily: "ESP32", sku: null, url: null, image: null }
];

window.VARIANTS_CONFIG = [
  { id: "nowifi", label: "No WiFi", description: "Minimal build — no WiFi provisioning or OTA" },
  { id: "wifi", label: "WiFi", description: "WiFi provisioning + OTA updates" }
];

window.BRANDING_CONFIG = [
  { icon: "🐦", label: "@printminion", url: "https://x.com/printminion" },
  { icon: "🖨️", label: "3D Enclosures on Cults3D", url: "https://cults3d.com/@printminion" },
  { icon: "☕", label: "Buy Me a Coffee", url: "https://buymeacoffee.com/printminion" }
];

window.PROJECT_CONFIG = {
  title:              "ESP32 Web Flasher",
  h1:                 "ESP32 Webflash Template",
  subtitle:           "Select your board, connect via USB and flash directly from your browser — no software required.",
  description:        "A ready-to-fork ESP32 firmware template with multi-board support, WiFi provisioning, OTA updates, and a browser-based web installer — no software required.",
  youtubeUrl:         null,
  howToUrl:           null,
  baseUrl:            "https://printminion.github.io/esp32-webflash-template",
  githubUrl:          "https://github.com/printminion/esp32-webflash-template",
  badgeText:          "Web Installer",
  projectName:        "esp32-webflash-template",
  projectDescription: "Multi-board ESP32 firmware template with browser-based web installer"
};
