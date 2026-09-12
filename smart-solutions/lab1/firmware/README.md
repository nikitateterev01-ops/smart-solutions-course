# AtomS3 püsivara

- Alus: KKallas/ESP32-Image-Server, `atom-image-server`
- Kasutatav PlatformIO board: `m5stack-atoms3`
- Framework: Arduino
- Display/library: M5Unified
- Serial: 115200
- Projekti `common/` failid on toodud firmware kausta sisse, et projekt oleks selles repos iseseisvalt ehitatav.
- Algkoodis olnud `../common` include tee muudeti kujule `$PROJECT_DIR/common`.
- Captive portali tarkvaraline teostus on lisatud; kontroll reaalse telefoniga on TODO.
- Settings/test osa tarkvaraline teostus on lisatud; kontroll reaalsel AtomS3-l on TODO.
- Reaalset flashimist ega nende muudatuste riistvaratesti ei ole veel tehtud.

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
- selle build'i ajal ei olnud captive portal veel lisatud

## Tarkvaraline build — 13.09.26

- PlatformIO Core: 6.2.0
- environment: `atoms3r`
- board: `m5stack-atoms3`
- build: SUCCESS
- RAM: 83 716 / 327 680 B (25,5%)
- Flash: 1 176 065 / 3 342 336 B (35,2%)
- build sisaldab captive portali ning seadete ja ekraanitesti tarkvaralist teostust
- build tehti ilma AtomS3 ühendamiseta
- uploadi ei tehtud
- kontroll reaalsel AtomS3-l ja telefonidel on TODO
