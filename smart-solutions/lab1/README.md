> **Meie lisatud märkus (ei kuulu õpetaja originaalülesande teksti):** See fail on elav tööleht. Reaalsed mõõtmised, koordinaadid ja katsetulemused lisatakse laboris.

---

## Nutikad Lahendused: Labor 1 — Robot, ekraan ja tähemasin

**Töömaht:** 28 tundi | **Hindamine:** 20 punkti | **Meeskond:** 3 tudengit | **Välja antud:** 12.09.26 | **Tellimise kuupäev:** 22.09.26 | **Esimene kaitsmine:** 06.10.26, veebis

### Kuidas see dokument töötab

* Kopeeri see fail esimesel päeval oma repo laborikausta `README.md`-ks ja täida seal, töö käigus.
* KAARDISTA ise on puudu, sest vastust ei tea veel keegi. Sina ise mõõdad ja kirjutad numbri ja põhjuse siia.
* Midagi ei kustutata. Vale number jääb, kuupäevaga, parandus tuleb tema alla.
* Kirjuta nii, et meeskonnakaaslane, kes sel päeval ruumis ei olnud, saab aru: päris failinimed, päris numbrid, ühikud.
* Skeemid ja simulatsioonid lähevad dokumenti pildina, pildi juurde link elavale failile, et teine saaks selle lahti teha ja edasi muuta. Näited on Andmehõive Labori 1 töölehel: Falstadi simulatsioon ja draw.io skeem. Tee enda omad samade tööriistadega.
* Tähtaeg ei ole tähtis. Tähtis on, et asi saab tehtud ja sa saad aru. Ei tulnud esimesel korral välja, tule homme tagasi ja proovi uuesti. Kaitsta saab nii mitu korda, kui vaja.

### Eesmärk

Sama meeskond teeb kõiki kolme ainet. Iga aine annab ühe tüki:

* **Andmehõive** teeb AtomS3, mille nupp valib tähe ja saadab selle välja.
* **3D printimine** teeb pastakahoidiku, mis käib roboti käe otsa (flantsi külge).
* **Robotil MG400** on võrguport, kust ta võtab vastu liikumiskäske.

Igaüks neist töötab eraldi. Aga eraldi ei joonista neist ükski. Puudu on see osa, mis võtab tähe Atomist vastu ja teeb sellest roboti liigutused. Selle osa teed sina selles laboris. Kõik kolm ainet lõpevad ühe demoga: **vajuta Atomil tähte, robot joonistab selle paberile.**

**Jaam** on selles dokumendis sinu sülearvuti, kus jookseb Pythoni programm. Jaam räägib Atomiga üle WiFi ja robotiga üle Etherneti.

Selles laboris on kolm osa:

1. **Robot.** Pane MG400 tööle oma sülearvutist. Õppejõud annab baaspaketi: Pythoni programm, mis avab brauseris lehe liugurite, salvestatud asendite ja pumba nuppudega. Sinu töö: pane pakett tööle, kontrolli, kas paketi eeldused (aadressid, pumba liinid) vastavad tõele, ja õpeta robotile neli asendit.
2. **Ekraan.** Laadi AtomS3-le PlatformIO-st püsivara, mis teeb oma WiFi võrgu ja näitab lehte, kust saab pildi ekraanile saata. Kui telefon selle võrguga liitub, peab leht ise lahti minema, ilma et keegi aadressi trükiks. Lehele tuleb ka seadete ja testinuppude osa. See leht jääb kogu aastaks: kõik, mis hiljem Atomi külge tuleb (rõhuandur, UART, klapp, LED), saab oma seaded ja testinupu siia, mitte eraldi lehele.
3. **Täht.** Atomi nupp valib tähe. Täht jõuab jaama. Jaam saadab robotile liikumiskäsud ja robot joonistab tähe paberile. Vähemalt kolm tähte.

Esimene asi on tellimus. Esimesel päeval uusi osi ei ole: mõtle välja, mida see labor üldse vajab ja mis riiulil puudu on, ja kirjuta see tellimuseks, mis läheb välja 22.09. Tellitu jõuab kohale selle labori ajal. Seni ehita sellest, mis riiulil on.

*See on elav dokument. Uuenda eesmärke, kui need töö käigus muutuvad — uued teadmised teevad vanad eesmärgid vahel mõttetuks. Mõte on hoida meeskond kogu aeg sihil, et ei eksitaks detailide metsa ja põhiprobleem ei jääks lahendamata.*

**KAARDISTA ISE — eesmärk nii, nagu ta tegelikult välja tuli.**

### Kontrollnimekiri

**Peab olema tehtud**

- [ ] Tellimus 22.09: mis selle labori jaoks riiulil puudu on, failis `docs/bom.md`.
- [ ] Robot on API-režiimis. `mg400 status` vastab. Leht liigutab robotit. Pump imeb ja puhub käsurealt.
- [ ] Neli asendit õpetatud ja failis `data/positions.json`. Robot tõstab proovitüki allikast valmis pessa kümme korda järjest.
- [ ] AtomS3 püsivara on PlatformIO-st peale laetud. Atom teeb oma WiFi võrgu. Telefon liitub ja leht avaneb ise, ilma aadressi trükkimata. Pilt jõuab lehelt ekraanile.
- [ ] Atomi lehel on seadete ja testide osa. Fail `docs/atom_page.md` ütleb, mis seal on.
- [ ] Täht: Atomi nupp valib tähe, jaam saab selle kätte, robot joonistab. Kolm tähte.
- [ ] Repo ja arenduspäevik täidetud, tag `smart-solutions-lab1`.

**KAARDISTA ISE — kuupäevad ja sinu enda sammud.**

### Sisendid

* Riiulilt: AtomS3, USB-C kaabel, USB-C → Ethernet adapter, LAN kaabel, marker, maalriteip, paber. Proovitükk tõstmise jaoks: AtomS3 näidissilt või mis tahes lameda pealsega asi, umbes 24 × 24 mm.
* Õppejõult: MG400, mis on juba API-režiimis, koos pumbakasti ja iminapa komplektiga. MG400 baaspakett.
* Andmehõive L1-st: täht. Atom saadab ühe JSON rea. Kuidas see rida jaama jõuab (kanal), lepite ise esimesel nädalal kokku. Leppige kokku enne, kui kumbki pool koodi kirjutab.
* 3D printimise L1-st: pastakahoidik, mis käib flantsi külge. Kuni hoidikut ei ole, teibi marker flantsi külge.

### Vahendid

1. MG400 koos iminapa komplektiga ja pumbakastiga
2. Sülearvuti Ethernet pordi või adapteriga; Python 3.11+, venv, pip, Flask
3. MG400 baaspakett: `KKallas/mg400-base` (eraldi repo)
4. AtomS3, USB-C kaabel; VS Code ja PlatformIO laiendus; M5Unified
5. ESP32-Image-Server alguspunktiks (link taustainfos)
6. Telefon, millega Atomi võrku minna
7. Marker, maalriteip, paber; pastakahoidik 3D printimise L1-st, kui valmis
8. Git, üks repo meeskonna kohta, `AGENTS.md` juurkaustas
9. draw.io

*Kui plaan muutub, uuenda ka vahendeid, või tee draw.io skeem, mis näitab, kuidas asjad omavahel töötavad.*

**KAARDISTA ISE — mida sa päriselt kasutasid.**

### Taustainfo

* **MG400 baaspakett**: README ütleb, kuhu kaabel käib ja mis aadress on, ning kirjeldab käsurea ja HTTP API. API-režiim on robotil juba sees. Kui ei ole, on `docs/dobot-api-mode-windows.md` ühekordne juhend Windowsi arvutist ja `docs/dobot-api-mode.md` Macist.
  [https://github.com/KKallas/mg400-base](https://github.com/KKallas/mg400-base)
* **Dobot TCP/IP protokoll**: port 29999 on käsud (EnableRobot, ClearError, DO, GetPose), port 30003 on liikumine (MovL, ServoP), port 30004 on tagasiside iga 8 ms.
  [https://github.com/Dobot-Arm/TCP-IP-Protocol](https://github.com/Dobot-Arm/TCP-IP-Protocol)
  Doboti enda Pythoni näide: [https://github.com/Dobot-Arm/TCP-IP-4Axis-Python](https://github.com/Dobot-Arm/TCP-IP-4Axis-Python)
* **Pumbakast**: otsi fraasi "Dobot MG400 vacuum pump box IO control". Kast on kahel DO liinil. Klemmid on kasti juhendis.
* **AtomS3**: viigud, ekraan, nupp
  [https://docs.m5stack.com/en/core/AtomS3](https://docs.m5stack.com/en/core/AtomS3)
* **PlatformIO**: [https://docs.platformio.org/](https://docs.platformio.org/) ja M5Unified: [https://github.com/m5stack/M5Unified](https://github.com/m5stack/M5Unified)
* **ESP32-Image-Server**: AtomS3 püsivara, mis teeb WiFi võrgu, näitab pilti ja pakub lehte, kust pilt üles laadida. Captive portalit seal veel ei ole. Selle teed sina.
  [https://github.com/KKallas/ESP32-Image-Server](https://github.com/KKallas/ESP32-Image-Server)
* **ESP32 WiFi AP**: [https://randomnerdtutorials.com/esp32-access-point-ap-web-server/](https://randomnerdtutorials.com/esp32-access-point-ap-web-server/)
* **Captive portal**: otsi fraasi "ESP32 captive portal DNSServer generate_204 hotspot-detect". Telefon küsib WiFi-ga liitumisel ühte kindlat aadressi. Kui vastus ei ole see, mida ta ootab, avab ta lehe ise.
* **Flask**: [https://flask.palletsprojects.com/en/stable/quickstart/](https://flask.palletsprojects.com/en/stable/quickstart/)

*Lisa siia oma allikaid ja kasulikku infot, mis aitaks sul projektist aru saada ka aastaid hiljem, kui selle uuesti lahti teed.*

**KAARDISTA ISE — sinu allikad.**

### Osad

#### 1. Robot

Robot on ühendatud LAN1 porti ja tema aadress on 192.168.1.6. Robot on API-režiimis. See tähendab, et ta kuulab porte 29999, 30003 ja 30004 kogu aeg, kui vool on peal, ja ootab sealt käske.

Pane oma arvuti Ethernet pordile käsitsi aadress 192.168.1.50, mask 255.255.255.0, gateway tühi. Kõigepealt ping. Kui ping käib, aga port ei vasta, on API-režiim väljas. Kuidas see sisse lülitada, ütleb baaspaketi kaustas `docs/` olev juhend.

Baaspakett annab kaks käsku:

* `mg400 status` ütleb roboti režiimi ja praeguse asendi.
* `mg400 serve` avab brauseris lehe. Lehel on nupud "ühenda" ja "luba", liugurid X/Y/Z/R, kiirus, kümme salvestatud asendit ja pumba nupud.

Esimene liigutus 20 % kiirusel, käsi hädastopi juures. Robotile saadab liikumiskäske korraga ainult üks programm.

Pumbakast on ühendatud roboti kahe digitaalväljundi (DO) külge. Baaspakett eeldab, et DO2 on imemine ja DO1 puhumine. Kontrolli seda kasti juhendist ja multimeetriga enne, kui midagi ühendad. Alates teisest nädalast juhib Andmehõive meeskond pumpa sinu käsurea kaudu.

Õpeta robotile neli asendit: `above_source` (allika kohal), `source` (allikas), `above_finished` (valmis pesa kohal), `finished` (valmis pesa). Test: robot tõstab proovitüki allikast ja paneb valmis pessa, kümme korda järjest, 20 % kiirusel.

Kirjuta üles:

* aadressiplaan: roboti aadress, arvuti aadress, liides, mask;
* pordid ja mida igaüks teeb;
* DO numbrid ja kuidas sa need üle kontrollisid;
* neli asendit failis `data/positions.json`;
* kümme tõstmist failis `docs/pick_test.csv` (kas tõstis, kas pani, märkus);
* mis baaspaketis oli valesti või puudu. Paranduse kohta tee pull request õppejõu repole.

#### Tegelik MG400 katse Raimo arvutil

- See MG400 katse toimus sama meeskonna ühise laboritöö käigus ja oli seotud ka Andmehõive Lab 1 tööga.
- Smart Solutions dokumenteerib sellest ainult robotiliikumise, võrgu, positsioonide, pick-and-place'i ja tähejoonistamise jaoks olulise osa.

- MG400 ühendati Raimo arvutiga LAN1 kaudu.
- Jaama IPv4 oli `192.168.1.50`.
- `ping 192.168.1.6` õnnestus.
- `mg400 status` töötas.
- `mg400 serve` töötas.
- Veebilehel töötasid **Connect** ja **Enable**.
- MG400 liikus veebilehe kaudu.
- Liikumist kontrolliti 20% kiirusel.
- Pumba funktsiooni test tehti.

Pooleli:

- DO1/DO2 tegelik vastavus
- Neli nõutud positsiooni
- 10 järjestikust pick-and-place tsüklit
- `positions.json`
- `pick_test.csv`

#### 2. Ekraan

Alguspunkt on ESP32-Image-Server, kaust `atom-image-server`. Ava see PlatformIO-s ja laadi AtomS3 peale. Plaadi nimi on `m5stack-atoms3`. M5Unified tunneb ekraani ise ära. Seeriaport 115200. Kui Atomil ei ole salvestatud võrku, teeb ta oma WiFi võrgu aadressiga 192.168.4.1. Sellel aadressil on leht: lõika pilt, saada slotti, pilt on ekraanil.

Sinu esimene töö on captive portal. See tähendab: telefon liitub Atomi WiFi võrguga ja leht avaneb ise, keegi ei trüki aadressi. Kuidas see töötab: Atomi DNS vastab igale nimele Atomi enda aadressiga. Telefon küsib liitumisel mõnda kindlat kontrollaadressi. Atom vastab sellele lehega. Logi, mida telefon küsis. Android ja iPhone küsivad erinevaid aadresse.

Sinu teine töö on seadete ja testide osa lehel. Praegu läheb sinna: võrgu nimi ja parool, jaama aadress, testinupp, mis näitab ekraanil olekut. See osa jääb ja kasvab. Laboris 2 tulevad siia rõhuanduri lugem ja UART test, hiljem klapp ja LED. Reegel: iga riistvara, mis Atomi külge tuleb, saab oma seaded ja testinupu sellele samale lehele. Eraldi lehti ei tehta.

Kirjuta üles:

* püsivara laadimise sammud ja kui kaua see võtab;
* WiFi võrgu nimi, Atomi aadress, lehe URL;
* kui kaua võtab ühe 128 × 128 pildi saatmine üle Atomi WiFi;
* kontrollaadressid, mida telefon liitumisel küsis;
* lehe seadete ja testide nimekiri failis `docs/atom_page.md`.

#### 3. Täht

Andmehõive Labori 1 osas 5 teeb meeskond Atomi nupu nii, et lühike vajutus valib tähe ja pikk vajutus saadab selle. Sinu töö algab sealt, kus täht Atomist välja läheb.

Kanal on see, kuidas täht Atomist jaama jõuab. Selle lepite ise kokku. Atom on WiFi võrk ja HTTP server, seega on kaks võimalust: jaam küsib Atomilt aeg-ajalt, kas uut tähte on, või Atom saadab tähe ise jaamale. Kumbki sobib. Täht on üks JSON rida. Jaam paneb vastuvõtmisel ajatempli juurde.

```
Atom → jaam:   {"letter":"A"}
jaam → robot:  täht → punktide nimekiri → MovL punkt-punktilt, pliiats üles joonte vahel
```
```
täht tuleb:  kui robot ei ole lubatud → midagi ei liigu, leht näitab põhjust
             muidu: pliiats üles → esimene punkt → pliiats alla → punktid → pliiats üles
```

Iga täht on punktide nimekiri. Vähemalt kolm tähte: meeskonna initsiaalid. Pliiatsi allasõidu kõrgus (Z) leia kõigepealt markeriga, hiljem pastakahoidikuga. Hoidik annab järele, nii et kui õpetatud kõrgus on paar millimeetrit paigast ära, jääb hoidik terveks. Esimene joonistus 20 % kiirusel ja pliiats 20 mm paberist kõrgemal, õhus.

Kirjuta üles:

* kanal failis `docs/letter_channel.md`: kes võtab kellega ühendust, aadress, formaat;
* kolm tähte punktidena ja pliiatsi Z failis `docs/letters.md`; joonistatud täht mõõdetud joonlauaga ja võrreldud kavandatud suurusega;
* kolmkümmend nupuvajutust failis `docs/latency.csv`. Iga vajutuse kohta kolm ajatemplit: Atom saatis, jaam sai, jaam saatis esimese käsu robotile. Iga hüppe kohta keskmine ja maksimum.

**KAARDISTA ISE — vastused.** Iga osa kohta: numbrid, ühikud, kus fail on. Tegemata asja kohta üks rida, miks.

### Ohutus

* MG400 ulatub 440 mm kaugusele. Kui käsk on ootel, ei ole kellegi käed selles alas. Enne iga käivitust ütleb keegi "liigub".
* Ainus stopp, mida usaldad, on hädastopp roboti alusel. Stopp-nupp lehel on mugavus: testi seda, aga hoia käsi hädastopi lähedal, kui uus jada esimest korda jookseb.
* Iga uue tähe või jada esimene jooks 20 % kiirusel, ilma proovitükita, pliiats või iminapp 20 mm pinnast kõrgemal.
* Robotile saadab liikumiskäske korraga ainult üks programm.
* Pumbakast on 24 V. DO juhtmed ühenda ainult siis, kui robot on keelatud ja kast vooluvõrgust väljas.
* Kui Atom laborist välja läheb, vaheta WiFi parool vaikimisi omast ära.

### Komponendid selle labori jaoks

Tellimus läheb välja 22.09.26 ja jõuab kohale enne kaitsmist. Valmis nimekirja ei ole: meeskond käib labori alguses läbi ja paneb tellimuse ise kokku faili `docs/bom.md`, iga rea juures üks lause, milline osa seda küsib. Mõtle näiteks, kas igal sülearvutil on Etherneti port või adapter, kas USB-C kaableid jätkub ja millega robot joonistab, kuni hoidikut ei ole.

#### Esimese laborikülastuse kontrollnimekiri

- [ ] Kontrolli BOM-i järgi, mis on laboris olemas.
- [ ] Kontrolli, kas sülearvutil on Ethernet või USB-C → Ethernet adapter.
- [ ] Leia MG400 LAN1 port.
- [ ] Kontrolli, et hädastopp oleks käeulatuses.
- [ ] Kontrolli MG400 API-režiimi.
- [ ] Seadista arvuti Ethernet aadress 192.168.1.50 / 255.255.255.0.
- [ ] Pingi 192.168.1.6.
- [ ] Kontrolli porte 29999, 30003, 30004.
- [ ] Käivita `mg400 status`.
- [ ] Käivita `mg400 serve`.
- [ ] Tee esimene liigutus ainult 20% kiirusel.
- [ ] Kontrolli pumbakasti DO liinid juhendi ja multimeetriga.
- [ ] Ära ühenda 24 V pumbakasti juhtmeid enne kontrolli.
- [ ] Pane kõik reaalsed tulemused README-sse ja vajalikesse docs failidesse.

### Hindamiskriteeriumid

| Kategooria | Punktid |
| :--- | :--- |
| Tööfailid — baaspaketi seadistus ja sinu muudatused, Atomi püsivara, täheteed, `positions.json` | 5 p |
| Analüüs — aadressiplaan, DO kontroll, tõstmise tabel, üleslaadimise aeg, latentsus hüpe-haaval | 5 p |
| Prototüüp — leht liigutab robotit, pump käsurealt, Atom teeb võrgu ja telefon satub lehele, pilt ekraanil, robot joonistab Atomil valitud tähe | 5 p |
| Dokumentatsioon — README, arenduspäevik, `atom_page.md`, `letters.md`, `letter_channel.md`, `bom.md`, AGENTS.md | 5 p |
| **Kokku** | **20 p** |

### Kaitsmine

Link git repole, tag `smart-solutions-lab1`.

Kaitsmine on lihtne suuline 15 minuti jutuajamine. Näitad, kuidas Atomil vajutatud tähe robot joonistab, avad telefonist Atomi lehe ja oma arenduspäeviku. Õppejõud küsib umbes viis küsimust selle kohta, kuidas sa selle tegid. Kui esimesel korral ei õnnestu, tuled uuesti.

Repos on kaustas `smart-solutions/lab1/`:

* `src/` jaama kood: baaspaketi seadistus, tähe kanal, täheteed
* `firmware/` Atomi PlatformIO projekt
* `data/positions.json`
* `docs/`: `atom_page.md`, `letters.md`, `letter_channel.md`, `bom.md`, `pick_test.csv`, `latency.csv`, fotod, draw.io skeem ahelast
* see fail kui `README.md`
* `AGENTS.md` uuendatud

### Arenduspäevik

**KAARDISTA ISE — päevik.** Üks sissekanne iga töösessiooni kohta, kirjutatud iseendale, nii et inimene, kes seal ei olnud, saab aru. Sissekandeid lisatakse, mitte ei muudeta.

**PP.KK.AA — kes olid kohal**
* Tegime:
* Juhtus (numbrid):
* Otsustasime, ja miks:
* Lahti järgmiseks korraks:

**12.09.26 — kes olid kohal**
* Osalejad: TODO
* Tegime:
  * Repo struktuur loodi.
  * Õpetaja ülesanne kopeeriti README-sse.
  * Kohustuslike failide mallid loodi.
  * BOM-i kontrollnimekiri valmistati ette.
  * Esimese laborikülastuse sammud pandi kirja.
* Juhtus (numbrid): Reaalseid mõõtmisi ei tehtud.
* Otsustasime, ja miks: Riistvaraandmed jäävad TODO-ks kuni laborikontrollini, et dokumenti ei lisataks oletusi.
* Lahti järgmiseks korraks: Kontrollida laboris BOM-i ja esimese laborikülastuse kontrollnimekirja punkte.

**TODO kuupäev — MG400 katse Raimo arvutil**
* Osalejad: TODO
* Tegime:
  * Kloonisime `mg400-base`.
  * Seadistasime Python keskkonna.
  * Ühendasime MG400 LAN1 kaudu.
  * Seadistasime jaama IPv4 aadressile `192.168.1.50`.
  * Pingisime robotit.
  * Käivitasime `mg400 status`.
  * Käivitasime `mg400 serve`.
  * Kasutasime **Connect** ja **Enable**.
  * Liigutasime robotit veebilehe kaudu.
  * Kontrollisime 20% kiirust.
  * Testisime pumpa.
* Juhtus (numbrid):
  * Jaama IP: `192.168.1.50`
  * Roboti IP: `192.168.1.6`
  * Kontrollitud liikumiskiirus: `20%`
  * Muud reaalsed väärtused: TODO
* Otsustasime, ja miks:
  * DO, positsioonid ja 10 tsükli tulemused lisatakse alles pärast tegelikku kontrolli.
* Lahti järgmiseks korraks:
  * DO mapping
  * Neli positsiooni
  * 10 pick-and-place tsüklit
  * `positions.json`
  * `pick_test.csv`

### Väljundid ja tulemused

**Väljundid**
* Andmehõive L1 saab siit: pumba juhtimine käsurealt (imemine ja puhumine) alates teisest nädalast; tähe kanali kokkulepe.
* 3D printimise L1 saab siit: robot, mis joonistab hoidikuga tähe.
* Nutikad Lahendused L2 saab siit: Atomi leht, kuhu tööriistaplaadi seaded ja testid juurde tulevad; jaam, mille külge tööriistaplaat käib.

**KAARDISTA ISE, lõpus.**
* Git repo ja tag:
* Numbrid, mille see labor andis, ühikutega:
* Mida me teeksime teisiti:
* Mida järgmine labor peaks enne alustamist teadma:

### Tagasiside
