# Symulator automatu

Symulator automatu z napojami i przekaskami napisany w C.

## Cel projektu

Program symuluje dzialanie automatu sprzedajacego:
- wybór produktu
- platnosc gotowka lub karta
- odbior produktu
- uzupelnianie magazynu
- zapis i odczyt stanu po ponownym uruchomieniu

## Architektura programu

Program dziala jako dwa procesy:
- **proces klienta (rodzic)**: interfejs `ncurses`, obsluga klawiatury
- **proces automatu (potomek)**: logika biznesowa i stan automatu

Procesy komunikuja sie przez dwa potoki:
- klient -> automat (zadania)
- automat -> klient (odpowiedzi)

## Konstrukcje z zajec wykorzystane w projekcie

1. **Procesy (`fork`)**
   - rozdzielenie interfejsu i logiki na osobne procesy
   - plik: `main.c`

2. **Potoki (`pipe`) / IPC**
   - komunikacja miedzy procesami klient-automat
   - pliki: `main.c`, `ipc.c`, `automat.c`, `klient.c`

3. **Sygnaly (`SIGINT`)**
   - bezpieczne przerwanie programu przez Ctrl+C
   - plik: `klient.c`

4. **Operacje na plikach**
   - utrwalenie stanu w `automat_stan.dat` (fopen/fread/fwrite)
   - plik: `automat.c`


## Asortyment (10 slotow)

| Nr | Typ | Produkt |
|----|-----|---------|
| 1 | napoj | Coca-cola |
| 2 | napoj | Woda niegazowana |
| 3 | napoj | Woda gazowana |
| 4 | napoj | Sok pomaranczowy |
| 5 | napoj | Ice tea |
| 6 | przekaska | Paluszki solone |
| 7 | przekaska | Zelki owocowe |
| 8 | przekaska | Chipsy |
| 9 | przekaska | Orzeszki solone |
| 10 | przekaska | Baton musli |

## Platnosci i salda

W interfejsie widoczne sa trzy wartosci:
- **w automacie** - gotowka wrzucona do automatu
- **portfel** - gotowka, ktora mozesz jeszcze wrzucic
- **karta** - saldo portfela karty

### Gotowka
- `z / x / c` - wrzut 1 / 2 / 5 zl z portfela
- `v` - inna kwota wrzutu z portfela
- `Enter / k` - zakup za gotowke z automatu
- `r` - wydanie reszty z automatu z powrotem do portfela
- `g` - doladowanie portfela gotowki

### Karta
- `p` - platnosc karta
- `l` - doladowanie karty

## Odbior produktu

Po udanym zakupie automat przechodzi w tryb odbioru:
- pojawia sie komunikat z numerem slotu
- potwierdzenie: `Enter` lub `o`
- do czasu odbioru inne operacje sa zablokowane

## Magazyn

- `MAX_STAN_MAG = 8` - maksymalna pojemnosc jednego slotu
- `u` - dodaj +1 sztuke do wybranego slotu
- `U` - uzupelnij wybrany slot do pelna

## Zapis stanu

Stan zapisywany jest do pliku:
- `automat_stan.dat`

Zapisywane sa m.in.:
- stan magazynu
- saldo gotowki w automacie
- saldo portfela gotowki
- saldo karty

Plik jest aktualizowany po operacjach i przy wyjsciu (`q`).
Po ponownym uruchomieniu stan jest odczytywany z pliku.

### Reset do stanu poczatkowego

```bash
rm automat_stan.dat
./automat
```

## Uruchomienie

```bash
make
./automat
```

## Budowa

```bash
make clean
```
