# Symulator automatu 

Symulator automatu z napojami i przekąskami: **fork**, **pipe**, **SIGINT**, interfejs **ncurses**.

## Uruchomienie

```bash
make
./automat
```


## Asortyment (10 slotów)

| Nr | Typ | Produkt |
|----|-----|---------|
| 1 | napój | Coca-cola |
| 2 | napój | Woda niegazowana |
| 3 | napój | Woda gazowana |
| 4 | napój | Sok pomarańczowy |
| 5 | napój | Ice tea |
| 6 | przekąska | Paluszki solone |
| 7 | przekąska | Żelki owocowe |
| 8 | przekąska | Chipsy |
| 9 | przekąska | Orzeszki solone |
| 10 | przekąska | Baton musli |

U góry ekranu widać oba salda: **GOTOWKA** i **KARTA**.

## Płatności

### Gotówka (saldo rośnie i maleje)
- **z / x / v** — wrzut 1 / 2 / 5 zł → **saldo gotówki rośnie**
- **c** — inna kwota (wrzut)
- **Enter / k** — zakup: cena **odejmuje się** z salda gotówki (jak przy karcie)
- **r** — wydaj całą resztę (saldo gotówki → 0)

Start: gotówka **0 zł** (dopiero po wrzucie monet masz środki).

### Karta (saldo portfela)
- Start: **200 zł** na karcie (albo kwota z pliku po ponownym uruchomieniu).
- **p** — płatność kartą: cena **odejmuje się** z salda karty.
- **l** — doładuj kartę (wpisz kwotę, Enter) → saldo karty **rośnie**.

## Odbiór produktu

Po udanym zakupie (gotówka lub karta) automat wymaga **odbioru**:
- komunikat i podświetlenie slotu `<<ODBIOR>>`
- **Enter** lub **o** — potwierdzenie odbioru
- do odbioru zablokowane są inne operacje (wrzut, kolejny zakup itd.)

## Magazyn (serwis)

- **MAX_STAN_MAG = 8** — maksymalna pojemność jednego slotu (fizyczny limit automatu).
- **u** — dodaj **1 sztukę** do **wybranego** slotu (1–10).
- **U** — uzupełnij **wybrany** slot do pełna (8 szt.).

## Zapis stanu — `automat_stan.dat`

Po zakupie, wrzucie, uzupełnieniu, odbiorze i przy wyjściu (**q**) stan jest zapisywany do pliku:

**`automat_stan.dat`**

(w katalogu, z którego uruchamiasz `./automat`)

Zapisywane są m.in.:
- saldo gotówki i salda karty
- stan magazynu każdego slotu (1–10)

Przy starcie program **wczytuje** ten plik, jeśli istnieje.

### Reset do stanu początkowego

Usuń plik i uruchom program ponownie:

```bash
rm automat_stan.dat
./automat
```

Dostaniesz domyślny magazyn (5 napojów + 5 przekąsek) i kartę z **200 zł**.

## Sterowanie

| Klawisz | Akcja |
|---------|--------|
| ↑ / ↓, `1`–`10` | Wybór produktu |
| Enter / `k` | Kup za gotówkę |
| `p` | Kup kartą |
| `l` | Doładuj kartę |
| `z` / `x` / `v` | Wrzut 1 / 2 / 5 zł |
| `c` | Inna kwota gotówki |
| `r` | Reszta |
| `u` | +1 szt. w wybranym slocie |
| `U` | Pełny wybrany slot |
| Enter / `o` | Odbiór po zakupie |
| `q` | Wyjście (zapis stanu) |
| Ctrl+C | Przerwanie |

## Budowa

```bash
make clean
```
