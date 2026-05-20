# Symulator automatu 

- **fork** + **pipe** (klient ↔ proces automatu)
- **SIGINT** (Ctrl+C) — grzeczne zamknięcie
- Interfejs **ncurses** (panel w terminalze)

## Uruchomienie

Wymagane: `ncurses` (macOS: zwykle domyślnie; jeśli brak: `brew install ncurses`).

```bash
make
./automat
```

Terminal min. ok. 80×24 znaków.

## Sterowanie

| Klawisz | Akcja |
|---------|--------|
| ↑ / ↓ lub `1`–`6` | Wybór napoju |
| Enter / `k` | Kup wybrany napój |
| `z` / `x` / `v` | Wrzut 1 / 2 / 5 zł |
| `c` | Własna kwota (wpisz, Enter) |
| `r` | Wydaj resztę (zeruje saldo) |
| `u` | Uzupełnij magazyn (demo) |
| `q` | Wyjście |
| Ctrl+C | Przerwanie |

## Budowa

```bash
make clean
```
