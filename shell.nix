{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = with pkgs; [
    # PlatformIO for ESP32 development
    platformio
    
    # Python for PlatformIO
    python3
    python3Packages.pip
    
    # Serial communication tools
    screen
    minicom
    
    # Development tools
    git
    curl
    
    # Optional: VS Code with PlatformIO extension
    # vscode-with-extensions.override {
    #   vscodeExtensions = with vscode-extensions; [
    #     platformio.platformio-ide
    #   ];
    # }
  ];

  shellHook = ''
    echo "🌅 Sunrise Alarm Clock Development Environment"
    echo "Available commands:"
    echo "  pio lib install - Install project dependencies"
    echo "  pio run          - Build the project"
    echo "  pio run -t upload - Upload to ESP32"
    echo "  pio device monitor - Monitor serial output"
    echo ""
    echo "If you want to use OTA Updates make sure to disable system firewall for this e.g. using"
    echo "  sudo nixos-firewall-tool open tcp 13351"
    echo "Make sure to configure your settings in include/config.h"
    echo "==============================="
    echo "==============================="
  '';
}
