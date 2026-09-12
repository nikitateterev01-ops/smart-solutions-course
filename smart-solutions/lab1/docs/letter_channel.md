# Tähe kanal Atomi ja jaama vahel

Lõplik kanal valitakse pärast meeskonna kokkulepet ja laborikatset.

## Variant 1: jaam küsib Atomilt perioodiliselt

Jaam loob ühenduse Atomiga ja küsib kindla ajavahemiku järel, kas uus täht on saadaval.

- Atomi aadress: TODO
- HTTP endpoint: TODO
- Küsimise intervall: TODO
- Uue ja juba loetud tähe eristamine: TODO

## Variant 2: Atom saadab ise jaamale

Atom loob ühenduse jaamaga ning saadab tähe pärast kasutaja kinnitust.

- Jaama aadress: TODO
- HTTP endpoint või muu protokoll: TODO
- Korduskatse vea korral: TODO
- Vastuvõtu kinnitus: TODO

## Andmevorming

```json
{"letter":"A"}
```

Jaam lisab vastuvõtmisel ajatempli. Lõplikku varianti ei ole veel valitud.
