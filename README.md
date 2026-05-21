# Symulator automatu (Systemy operacyjne)

Symulator automatu z napojami i przekąskami: **fork**, **pipe**, **SIGINT**, interfejs **ncurses**.

## Uruchomienie

```bash
make
./automat
```

Terminal ok. 80×24. Uruchamiaj z katalogu `automat_symulator` (tam powstaje plik stanu).

## Asortyment (10 slotów)

| Nr | Typ | Produkt |
|----|-----|---------|
| 1 | napój | Cola 330 ml |
| 2 | napój | Woda niegazowana |
| 3 | napój | Sok pomarańczowy |
| 4 | napój | Energetyk |
| 5 | napój | Herbata mrożona |
| 6 | przekąska | Paluszki solone |
| 7 | przekąska | Baton czekoladowy |
| 8 | przekąska | Chipsy paprykowe |
| 9 | przekąska | Orzeszki solone |
| 10 | przekąska | Baton musli |

## Płatności

### Gotówka
- **z / x / v** — wrzut 1 / 2 / 5 zł
- **c** — inna kwota
- **Enter / k** — zakup ze salda gotówki
- **r** — wydaj resztę

### Karta (portfel, nie limit jednej transakcji)
- Start: **200 zł** na karcie (albo kwota z pliku po ponownym uruchomieniu).
- **p** — płatność kartą: cena **odejmuje się** z salda karty.
- **l** — doładuj kartę (wpisz kwotę, Enter).

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

### Dlaczego wcześniej wszystko znikało?

Stan był tylko w **pamięci procesu potomka** (logika automatu po `fork`). Po **q** proces się kończył → przy ponownym `./automat` program startował od zera.

### Jak jest teraz?

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

> Stary plik z wersji 6 produktów nie pasuje do nowego formatu (10 slotów) — wtedy też wczytywany jest stan domyślny. Po pierwszym zapisie powstaje nowy `automat_stan.dat`.

Plik jest w `.gitignore` — nie trafia na GitHub (stan u Ciebie lokalnie).

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
