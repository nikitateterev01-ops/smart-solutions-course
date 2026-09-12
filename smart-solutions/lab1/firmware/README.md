# AtomS3 püsivara

- Alus: KKallas/ESP32-Image-Server, `atom-image-server`
- Kasutatav PlatformIO board: `m5stack-atoms3`
- Framework: Arduino
- Display/library: M5Unified
- Serial: 115200
- Projekti `common/` failid on toodud firmware kausta sisse, et projekt oleks selles repos iseseisvalt ehitatav.
- Algkoodis olnud `../common` include tee muudeti kujule `$PROJECT_DIR/common`.
- Captive portal ei ole veel lisatud.
- Settings/test osa ei ole veel lisatud.
- Reaalset flashimist ega riistvaratesti ei ole veel tehtud.

Source: [https://github.com/KKallas/ESP32-Image-Server](https://github.com/KKallas/ESP32-Image-Server)

## Kontrollitud build — 12.09.26

- PlatformIO Core: 6.2.0
- environment: `atoms3r`
- board: `m5stack-atoms3`
- build: SUCCESS
- RAM: 83 580 / 327 680 B (25,5%)
- Flash: 1 166 073 / 3 342 336 B (34,9%)
- build tehti ilma AtomS3 ühendamiseta
- uploadi ei tehtud
- captive portal ei ole veel lisatud

