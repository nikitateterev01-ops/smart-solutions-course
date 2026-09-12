# AtomS3 lehe kirjeldus

Captive portal ning seadete ja ekraanitesti tarkvaraline teostus on lisatud. Reaalsed väärtused ja riistvaratesti tulemused lisatakse pärast kontrolli laboris.

## Võrk ja leht

| Väli | Väärtus |
| :--- | :--- |
| Wi-Fi SSID | TODO |
| Parool | TODO |
| Atom IP | TODO |
| Lehe URL | TODO |

## Captive portal

SoftAP-režiimis käivitab püsivara `DNSServer`-i. Wildcard DNS suunab kõik nimed aadressile `WiFi.softAPIP()`. STA-režiimi lülitumisel DNS peatatakse. Tundmatu URL avab AP-režiimis Atom Frameri avalehe ja annab STA-režiimis vastuse `404 Not found`.

Püsivara suunab avalehele järgmised captive portal probe'i aadressid:

- Android: `/generate_204` ja `/gen_204`
- iPhone/iOS: `/hotspot-detect.html` ja `/library/test/success.html`
- Windows ja muud kliendid: `/connecttest.txt`, `/ncsi.txt`, `/redirect`, `/canonical.html` ja `/success.txt`

- Tarkvaraline teostus: lisatud
- Automaatne avanemine Android-telefonis: TODO
- Automaatne avanemine iPhone'is: TODO
- Käitumine eri operatsioonisüsteemidega: TODO

## Pildi saatmine

- 128 × 128 pildi saatmise aeg: TODO
- Mõõtmise kuupäev ja tingimused: TODO
- Reaalne pildi saatmine AtomS3-le: TODO

## Seadete osa

Olemasolevale Atom Frameri lehele on lisatud `Settings & tests` osa järgmiste väljadega:

- Wi-Fi name (SSID)
- Wi-Fi password
- Station address

`GET /settings` tagastab salvestatud SSID, jaama aadressi, võrgurežiimi ja IP-aadressi. Salvestatud Wi-Fi parooli vastus ei sisalda ning parooliväli jääb lehe avamisel tühjaks.

Kui SSID ei muutu ja parooliväli on tühi, jätab veebileht salvestatud Wi-Fi parooli muutmata. Uue SSID korral salvestab `POST /settings` sisestatud SSID ja parooli koos.

`POST /settings` võtab vastu `application/x-www-form-urlencoded` väljad `station`, `ssid`, `pass` ja valikulise `connect=1`. Ilma `connect` väljata salvestab Atom seaded ning jääb praegusesse võrgurežiimi. Väärtusega `connect=1` saadab Atom esmalt HTTP-vastuse ja alustab seejärel ühendamist olemasoleva `startSTA` funktsiooni kaudu.

Nupud:

- `SAVE SETTINGS`
- `SAVE + CONNECT`
- `TEST DISPLAY STATUS`

## Testinupud

`POST /test/display` kutsub välja olemasoleva võrgu ja seadme olekuekraani. Endpoint ei lisa näidisandmeid ega kinnita ekraani füüsilist toimimist.

- Ekraani oleku testi tarkvaraline teostus: lisatud
- Ekraani oleku test reaalsel AtomS3-l: TODO

## Riistvaratesti kontrollnimekiri

- [ ] Laadi püsivara reaalsele AtomS3-le.
- [ ] Märgi kasutatud Wi-Fi SSID, Atom IP ja lehe URL.
- [ ] Kontrolli captive portalit Android-telefoniga.
- [ ] Kontrolli captive portalit iPhone'iga.
- [ ] Kontrolli pildi saatmist ja kuvamist AtomS3 ekraanil.
- [ ] Mõõda 128 × 128 pildi saatmise aeg.
- [ ] Kontrolli seadete püsimist pärast taaskäivitust.
- [ ] Kontrolli STA-ühendust labori võrgus.
- [ ] Kontrolli, et `GET /settings` ei tagastaks Wi-Fi parooli.
- [ ] Kontrolli `TEST DISPLAY STATUS` nuppu reaalsel ekraanil.
