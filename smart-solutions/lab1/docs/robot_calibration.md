# Roboti joonistusala kalibratsioon

Fail `config/robot_calibration.json` hoiab normaliseeritud tähepunktide teisendamiseks vajalikke laboriväärtusi. Reaalsed koordinaadid, Z-kõrgused, R ja kiirus mõõdetakse MG400-l laboris. Seetõttu on need väärtused praegu tahtlikult `null` ning kalibratsioon on puudulik.

## Teisendus

Tarkvara kasutab ainult järgmist abstraktset XY-teisendust:

```text
robot_x = origin_x + normalized_x * width
robot_y = origin_y + normalized_y * height
```

Normaliseeritud X ja Y peavad olema vahemikus `0..1`. Joonistusala laius ja kõrgus peavad olema positiivsed arvud. `bool` ei ole arvuline kalibratsiooniväärtus.

## Ohutu puudulik olek

Puudulik kalibratsioon on enne laborimõõtmisi normaalne ja ohutu olek. `src/robot_mapping.py` keeldub sel juhul punkti teisendamast ning annab selge vea. Fail ei sisalda MG400 võrguühendust, API-kõnesid ega roboti käsunimesid.

Ka täielik ja valideeritud kalibratsioon tähendab ainult seda, et abstraktse punkti saab arvutada. See ei luba ega käivita automaatselt MG400 käskude saatmist.

## Laboris täidetav kontrollnimekiri

- [ ] Mõõda joonistusala alguspunkt X.
- [ ] Mõõda joonistusala alguspunkt Y.
- [ ] Mõõda joonistusala laius.
- [ ] Mõõda joonistusala kõrgus.
- [ ] Mõõda pliiatsi ohutu Z-asend.
- [ ] Mõõda pliiatsi joonistamise Z-asend.
- [ ] Kontrolli R väärtus.
- [ ] Kinnita esimese katse kiirus, maksimaalselt 20%.
- [ ] Sisesta kinnitatud väärtused faili `config/robot_calibration.json`.
- [ ] Käivita tarkvaratestid uuesti.
- [ ] Kontrolli teisendatud punkte enne robotiadapteri loomist.

## Veel tegemata

- Reaalsete väärtuste mõõtmine: TODO
- Robotiadapter: TODO
- MG400 käskude saatmine: TODO
- Füüsiline joonistuskatse: TODO
