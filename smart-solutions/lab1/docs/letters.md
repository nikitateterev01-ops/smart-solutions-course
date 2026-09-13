# Tähtede punktid ja mõõtmised

Fail `config/letters.json` kirjeldab tähtede A, L ja N tarkvaralisi trajektoore. Need trajektoorid ei kinnita, et MG400 on tähti füüsiliselt joonistanud.

## Tarkvaraline trajektoor

Koordinaadid kasutavad suhtelist ala `0,0 .. 1,0`. X kasvab vasakult paremale ja Y alt üles. Väärtused ei ole millimeetrid ega MG400 koordinaadid. Iga `stroke` on eraldi joon; liikumisplaan tõstab pliiatsi joonte vahel üles.

### A

Joon 1:

```text
(0,10; 0,00) -> (0,50; 1,00) -> (0,90; 0,00)
```

Joon 2:

```text
(0,30; 0,45) -> (0,70; 0,45)
```

### L

Joon 1:

```text
(0,20; 1,00) -> (0,20; 0,00) -> (0,85; 0,00)
```

### N

Joon 1:

```text
(0,15; 0,00) -> (0,15; 1,00) -> (0,85; 0,00) -> (0,85; 1,00)
```

## Laboris mõõdetav

- Joonistusala alguspunkt: TODO
- Joonistusala laius: TODO
- Joonistusala kõrgus: TODO
- Normaliseeritud punktide vastendus reaalsele XY-alale: TODO
- Pliiats üleval Z: TODO
- Pliiats all Z: TODO
- R: TODO
- Kiirus: TODO

## Mõõdetud tulemus

- Täht A: TODO
- Täht L: TODO
- Täht N: TODO

## Tarkvara ja roboti ühendamine

- Normaliseeritud punkti teisendamine kalibreeritud abstraktseks XY-punktiks: tarkvaraliselt valmis.
- Reaalsed MG400 koordinaadid ja joonistusala mõõdetakse laboris: TODO
- Ohutu pliiatsi tõstmise ja langetamise järjekord: TODO
- Esimene füüsiline joonistus 20% kiirusel: TODO

Kalibratsioon on praegu tahtlikult puudulik ja reaalsed väärtused on failis `config/robot_calibration.json` märgitud `null`. Täielik kalibratsioon lubab ainult koordinaatide tarkvaralist teisendamist; see ei tähenda MG400 käskude automaatset saatmist.

Kalibratsiooni struktuur ja ohutuspiirangud on kirjeldatud failis `docs/robot_calibration.md`.
