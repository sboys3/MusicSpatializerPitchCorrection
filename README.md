# Music Spatializer Pitch Correction
This plugin for Beat Saber allows the Music Spatializer plugin to maintain normal pitch when a song is not played at its original speed.

:warning: The algorithm that is used for recovering the stft phase information is private and not included in this repository. The file is recover.h and will cause build errors due to its absence. If you would like more information contact me on discord.

Dependencies: BSIPA, BSML and Music Spatializer

## Building

1. Download or clone to get a local copy
2. Edit `bslink.bat` to point to a BeatSaber installation
3. Run `bslink.bat` as administrator to create a symlink to your BeatSaber installation
4. Open `spatializerPitch.sln` in in visual studio 2019
5. Change the configuration from `Debug` to `Release` and Change the target from `Any Cpu` to `x64`
6. Build solution(ctrl+shift+b)