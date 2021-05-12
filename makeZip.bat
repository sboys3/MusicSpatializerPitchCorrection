mkdir tmpOut
mkdir tmpOut\Plugins
mkdir tmpOut\Libs
mkdir tmpOut\Libs\Native
copy bin\x64\Release\MusicSpatializerPitchCorrection.dll tmpOut\Plugins\MusicSpatializerPitchCorrection.dll
copy stftRecoverLib\bin\stftRecoverLib.dll tmpOut\Libs\Native\stftRecoverLib.dll
del MusicSpatializerPitchCorrectionPlugin.zip
7z a -tZip -mm=Deflate -mfb=258 -mpass=15 -r MusicSpatializerPitchCorrectionPlugin.zip ./tmpOut/*
rmdir tmpOut /s /q