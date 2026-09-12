# Tähe kanal Atomi ja jaama vahel

## Valitud lahendus

Labor 1 jaoks kasutame varianti, kus **Atom algatab ühenduse ja saadab tähe
jaamale HTTP POST päringuga**.

Põhjus:

- täht on sündmus, mitte pidev andmevoog;
- polling lisaks tarbetu viite ja päringud;
- HTTP sobib Atomi olemasoleva Wi-Fi lahendusega;
- jaam on niikuinii Python/Flask rakendus;
- HTTP vastus annab Atomile selge kinnituse, kas sündmus jõudis jaama.

## Osapooled

### AtomS3

Atom valib nupuga tähe. Kui kasutaja kinnitab saatmise, saadab Atom ühe
HTTP POST päringu jaamale.

Jaama aadress ei ole lähtekoodis fikseeritud. See salvestatakse Atomi
veebilehe seadetes väljale `station address`.

### Jaam

Jaam käivitab Flask serveri:

```text
http://<JAAMA_IP>:5000
```

Reaalne `<JAAMA_IP>` lisatakse pärast laborikatset.

Endpoint:

```text
POST /api/letter
Content-Type: application/json
```

## Sõnum

Minimaalne nõutud väli on õpetaja ülesandes kirjeldatud `letter`.

Meie protokoll lisab kaks välja, mis teevad korduskatse ohutuks:

```json
{
  "letter": "A",
  "session": "atom-boot-id",
  "seq": 12,
  "atom_sent_ms": 153422
}
```

Väljad:

| Väli | Tähendus |
| :--- | :--- |
| `letter` | üks ladina suurtäht `A`–`Z` |
| `session` | Atomi boot/session identifikaator |
| `seq` | selles session'is kasvav sündmuse number |
| `atom_sent_ms` | Atomi `millis()` hetkel, kui saatmine algas |

`session + seq` võimaldab Atomil HTTP vea korral sama sündmust uuesti saata
ilma, et robot sama tähte kaks korda joonistaks.

## Jaama vastus

Uus sündmus:

```json
{
  "ok": true,
  "duplicate": false,
  "letter": "A",
  "seq": 12,
  "station_received_iso": "TODO",
  "robot_started": false
}
```

Kordussõnum:

```json
{
  "ok": true,
  "duplicate": true,
  "letter": "A",
  "seq": 12,
  "station_received_iso": "TODO"
}
```

HTTP:

- `202` — uus täht võeti vastu;
- `200` — sama `session + seq` oli juba vastu võetud ja ainult kinnitatakse;
- `400` — vigane JSON või vigane täht.

## Korduskatse

Atom peab hoidma sündmust alles kuni HTTP `200` või `202` vastuseni.

Soovituslik loogika:

1. saada kohe;
2. kui ühendus või HTTP päring ebaõnnestub, oota 250 ms;
3. saada **sama `session` ja `seq`** uuesti;
4. proovi kuni 3 korda;
5. pärast kolmandat viga näita kasutajale saatmisviga, ära loo uut `seq` väärtust automaatselt.

Reaalne retry käitumine kontrollitakse AtomS3-l.

## Ohutus

Tähe vastuvõtmine **ei tohi iseenesest tähendada, et robot liigub**.

Jaam peab enne esimese MG400 käsu saatmist kontrollima vähemalt:

- kas robot on ühendatud;
- kas robot on lubatud;
- kas robot ei täida juba teist jada;
- kas tähe trajektoor on olemas;
- kas reaalsed Z väärtused on mõõdetud.

Praegune `station.py` võtab sündmuse vastu ja logib selle, kuid ei liiguta
robotit enne päris MG400 laborikatset.

## Ajatemplid ja latentsus

Õpetaja nõuab 30 katse kohta kolme ajatemplit:

1. Atom saatis;
2. jaam sai;
3. jaam saatis esimese käsu robotile.

Praegune protokoll salvestab:

- `atom_sent_ms`;
- `station_received_iso`;
- `station_received_monotonic_ns`;
- hiljem `robot_command_monotonic_ns`.

**Oluline:** Atomi `millis()` ja arvuti monotonic clock ei ole sama kell.
Neid ei tohi lihtsalt lahutada ja nimetada tulemuseks millisekundites.

Atom → jaam täpse latentsuse mõõtmise meetod valitakse laborikatse ajal
(nt kellade offseti mõõtmine/handshake või jaama poolt mõõdetud request/ACK
meetod). Väljamõeldud latentsusnumbreid dokumenti ei lisata.

Jaam → robot hüpet saab mõõta sama arvuti monotonic clockiga, sest mõlemad
ajatemplid tekivad jaamas.

## Failid

Planeeritud repo failid:

```text
smart-solutions/lab1/src/station.py
smart-solutions/lab1/src/mock_atom.py
smart-solutions/lab1/src/requirements.txt
smart-solutions/lab1/data/letter_events.csv
smart-solutions/lab1/docs/letter_channel.md
```

`mock_atom.py` on ainult arenduseks: sellega saab HTTP kanali enne
laborisse minekut läbi proovida.

## Enne laborit kontrollitav

- [x] HTTP sõnumiformaat on valitud.
- [x] Jaama endpoint on valitud.
- [x] ACK ja duplicate käitumine on kirjeldatud.
- [x] Station receiver on kirjutatud.
- [x] Mock Atom sender on kirjutatud.
- [ ] Käivitada station lokaalselt ja teha mock test.
- [ ] Ühendada sama protokoll päris Atomi saatmisfunktsiooniga.

## Laboris kontrollitav

- [ ] Jaama tegelik IP.
- [ ] Atom ja jaam näevad teineteist võrgus.
- [ ] Päris nupuvajutus saadab tähe.
- [ ] Katkestatud ühenduse retry töötab.
- [ ] Sama `session + seq` ei käivita robotit kaks korda.
- [ ] MG400 safety gate.
- [ ] Esimese robotikäsu ajatempel.
- [ ] 30 päris latentsuskatset.
