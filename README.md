# Sega Model 3 Arcade Emulator (Sinden)

This is a fork of [model3emu-code](https://www.supermodel3.com) to add native **Sinden** light gun support.

It will attempt to track SVN commits.

### RetroPie package
Use `supermodel3.sh` for _RetroPie-Setup_

    wget https://raw.githubusercontent.com/DirtBagXon/model3emu-code-sinden/main/supermodel3.sh -O $HOME/RetroPie-Setup/scriptmodules/emulators/supermodel3.sh

    sudo $HOME/RetroPie-Setup/retropie_setup.sh

### Additional arguments:

    -borders=<n>      Sinden border configuration for gun games:
                      0=none [Default], 1=standard, 2=wide
    -nomousecursor    Disable desktop mouse cursor in SDL Windowed mode
    -grabcursor       Limit cursor movement to SDL Window

For `arm` based systems, clone the `arm` branch:

    git clone --single-branch --branch arm https://github.com/DirtBagXon/model3emu-code-sinden.git


![model3](screenshot.png)

