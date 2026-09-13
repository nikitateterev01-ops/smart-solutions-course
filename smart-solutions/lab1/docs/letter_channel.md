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

Atomil on eraldi seadistatav tähe nupurežiim. Selles režiimis valib lühike
vajutus järgmise tähe `A`–`Z` ja pikk vajutus kinnitab saatmise. Kui tähe
nupurežiim on välja lülitatud, töötavad varasemad sloti short/long/double
žestid muutmata kujul.

Jaama aadress ei ole lähtekoodis fikseeritud. See salvestatakse Atomi
veebilehe seadetes väljale `station address`. Saatja normaliseerib palja
hosti või IP-aadressi kujule `http://<host>:5000/api/letter` ning lisab
juba skeemi ja pordiga baasaadressile ainult `/api/letter` tee.

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
- `422` — tähe trajektoor ei ole jaamas seadistatud.

## Atomi saatja teostus

Püsivaras luuakse igal käivitusel `esp_random()` põhine session'i tunnus.
See tunnus püsib ühe boot'i jooksul sama ja uue käivituse järel luuakse uus.
Järjekorranumber `seq` algab väärtusest `1` ning suureneb täpselt ühe korra
iga järjekorda vastu võetud kasutaja saatmissündmuse kohta. Sama sündmuse
korduskatsed kasutavad muutmata tähte, session'it, `seq` väärtust ja
`atom_sent_ms` väärtust.

Saatmine toimub `loop()`-is väikese olekumasinaga:

- `idle` — saatmist ei ole;
- `pending` — uus sündmus ootab esimest katset;
- `waiting_retry` — sama sündmus ootab järgmist katset;
- `success` — jaam vastas HTTP `200` või `202`;
- `failed` — aadress puudub või kõik katsed ebaõnnestusid.

Ühe sündmuse jaoks tehakse kuni kolm HTTP POST katset. Võrguvea, timeout'i
või muu kui `200`/`202` HTTP vastuse järel oodatakse ligikaudu 250 ms ja
proovitakse sama sündmust uuesti. HTTP ühenduse ja vastuse timeout on
piiratud. Kogu retry-tsüklit ei käivitata veebipäringu handler'is.

Kui `station address` on tühi, lõpetab saatja sündmuse arusaadava veaga ega
tee võrgupäringut. Serial logi sisaldab sündmuse tähte, session'it, `seq`
väärtust, katse numbrit ja ACK või vea olekut, kuid mitte Wi-Fi parooli.

## Tarkvaraline test

Endpoint:

```text
POST /test/letter
Content-Type: application/x-www-form-urlencoded
letter=A
```

Endpoint valideerib ühe tähe vahemikus `A`–`Z`, lisab uue sündmuse samasse
saatja järjekorda ja tagastab kohe JSON vastuse olekuga `pending`. See ei oota
HTTP handler'is retry-tsükli lõppu. Settings & tests lehe nupp
`TEST LETTER SEND` kasutab seda endpoint'i ja sama saatmisteed nagu füüsilise
nupu pikk vajutus.

## Ekraani tagasiside

Tähe nupurežiimis näitab ekraan valitud tähte. Saatmise ajal kuvatakse
`SENDING`, ACK järel `SENT`, vea korral `FAILED` ning seadistamata jaama puhul
`NO STATION`. Tagasiside ei kirjuta üle `frameBuf` sisu ega salvesta slotte.
Pärast lühikest tagasisidet taastatakse valitud tähe vaade või varasem
pildi-/olekuvaade vastavalt aktiivsele režiimile.

Reaalne ekraanikuva, nupud, retry ja Atom ↔ jaam ühendus kontrollitakse
AtomS3-l laboris.

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
- [x] Atomi püsivara saatmisfunktsioon ja retry-olekumasin on kirjutatud.
- [x] `/test/letter` tarkvaraline testitee on kirjutatud.
- [ ] Käivitada station lokaalselt ja teha mock test.
- [x] Ehitada ja kontrollida saatmisfunktsioon PlatformIO-ga.

## Laboris kontrollitav

- [ ] Jaama tegelik IP.
- [ ] Atom ja jaam näevad teineteist võrgus.
- [ ] Päris nupuvajutus saadab tähe.
- [ ] Katkestatud ühenduse retry töötab.
- [ ] `/test/letter` saadab sündmuse päris Atomilt jaamale.
- [ ] Tähe nupurežiim ei riku tavalisi sloti žeste.
- [ ] Sama `session + seq` ei käivita robotit kaks korda.
- [ ] MG400 safety gate.
- [ ] Esimese robotikäsu ajatempel.
- [ ] 30 päris latentsuskatset.
