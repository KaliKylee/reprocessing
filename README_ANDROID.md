# Half-Life: Reprocessing — port Xash3D FWGS (Android)

Código do Reprocessing + build do hlsdk-portable + freevgui (estático) + launcher e logs do Half-Force.

## Build
Suba esta pasta num repositório do GitHub e rode o workflow **Build Reprocessing (Android)**
(`.github/workflows/android.yml`). Artefatos:
- `ReprocessingLauncher-apk`  — APK (launcher + libs), versão 1.4, gamedir `reprocessing`
- `Reprocessing-libs`         — libs do APK (armeabi-v7a e arm64-v8a)
- `Reprocessing-libs-symbols` — libs SEM strip, para resolver endereços de crash (ver LOGS.md)

## Launcher (android/)
- Pacote `com.reprocessing.hl`, nome "Reprocessing", versão 1.4, gamedir `reprocessing`
- Cores: carmesim (`0xFFDC143C`) como principal, vermelho escuro (`0xFF7A0C1E` / `0xFF5C0A16`) e fundos em tons de vinho escuro
- Links de GitHub, YouTube e Discord mantidos como no launcher do Half-Force
- Ícone do APK: lambda vermelho enviado

## Logs
Ver `LOGS.md` (reprocessing_log.txt e reprocessing_crash.log em /sdcard/xash/).

## Mudanças para rodar no Xash/Android
- `game_shared/xash_filesystem.h` + `filesystem_utils.cpp`: IFileSystem próprio (sem filesystem_stdio)
- `cl_dll/input_xash.cpp`: entrada por toque (substitui inputw32.cpp)
- `cl_dll/mp3.cpp`: música via comando `music` da engine (sem FMOD), não testado
- `engine/eiface.h` e `dlls/util.*`: ponteiros de função e `MAKE_STRING` corretos em 64 bits
- `cl_dll/hl/hl_weapons.cpp`: `pStringBase` válido no cliente
- `public/build.h`: necessário para o CMake detectar Android/arquitetura
