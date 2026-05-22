#include "klient.h"

#include "automat.h"
#include "ipc.h"

#include <curses.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static volatile sig_atomic_t przerwano = 0;

static void obsluga_sigint(int nr)
{
	(void)nr;
	przerwano = 1;
}

static int wyslij(int fd_w, int fd_r, const AutomatReq *req, AutomatResp *resp)
{
	if (pelny_zapis(fd_w, req, sizeof(*req)) != (ssize_t)sizeof(*req))
		return -1;
	if (pelny_odczyt(fd_r, resp, sizeof(*resp)) != (ssize_t)sizeof(*resp))
		return -1;
	return 0;
}

static void ramka(int y, int x, int h, int w)
{
	mvaddch(y, x, ACS_ULCORNER);
	mvaddch(y, x + w - 1, ACS_URCORNER);
	mvaddch(y + h - 1, x, ACS_LLCORNER);
	mvaddch(y + h - 1, x + w - 1, ACS_LRCORNER);
	mvhline(y, x + 1, ACS_HLINE, w - 2);
	mvhline(y + h - 1, x + 1, ACS_HLINE, w - 2);
	mvvline(y + 1, x, ACS_VLINE, h - 2);
	mvvline(y + 1, x + w - 1, ACS_VLINE, h - 2);
}

static void wypisz_komunikat(int y, int x, int szer_box, const char *tekst,
			     int ok)
{
	int n = szer_box - 6;
	if (n < 10)
		n = 10;
	for (int i = 0; i < n; i++)
		mvaddch(y, x + 2 + i, ' ');
	color_set(ok ? 3 : 4, NULL);
	if (tekst && tekst[0])
		mvprintw(y, x + 2, "%.*s", n, tekst);
	else
		mvprintw(y, x + 2, "(czekam na akcje...)");
	color_set(2, NULL);
}

static void rysuj_panel(const AutomatResp *resp, int wybor, pid_t pid_aut,
			int tryb_wlasna_kwota, const char *bufor_kwoty,
			int tryb_doladowanie, const char *bufor_doladowanie,
			int tryb_portfel, const char *bufor_portfel)
{
	const AutomatStan *s = &resp->stan;
	int wys, szer;
	getmaxyx(stdscr, wys, szer);
	int odbior = s->oczekuje_odbioru;

	clear();
	color_set(1, NULL);
	mvprintw(0, 2, " AUTOMAT: NAPOJE + PRZEKASKI ");
	color_set(2, NULL);
	mvprintw(1, 2, " Klient PID %-6d | Automat PID %-6d | zapis: %s ",
		 (int)getpid(), (int)pid_aut, PLIK_STANU);

	attron(A_BOLD);
	mvprintw(3, 2, " W automacie: %5.2f zl | Portfel: %5.2f zl | Karta: %6.2f zl ",
		 s->saldo_pln, s->portfel_gotowki_pln, s->saldo_karty_pln);
	attroff(A_BOLD);

	if (odbior > 0) {
		color_set(3, NULL);
		attron(A_BLINK);
		mvprintw(4, 2,
			 " >>> ODEBIERZ PRODUKT ZE SLOTU [%d] — Enter / o <<< ",
			 odbior);
		attroff(A_BLINK);
		color_set(2, NULL);
	}

	int box_w = szer - 4;
	if (box_w > 72)
		box_w = 72;
	int box_x = (szer - box_w) / 2;
	if (box_x < 1)
		box_x = 1;

	ramka(6, box_x, PRODUKTOW + 2, box_w);
	mvprintw(6, box_x + 2,
		 " 1-5 NAPOJE | 6-10 PRZEKASKI (max %d szt./slot) ", MAX_STAN_MAG);

	for (int i = 0; i < PRODUKTOW; i++) {
		int y = 7 + i;
		const ProduktInfo *n = &s->produkty[i];
		int nr = i + 1;
		int zaznacz = (nr == wybor);
		int slot_odbioru = (odbior == nr);

		if (slot_odbioru)
			attron(A_BOLD | COLOR_PAIR(3));
		else if (zaznacz && !odbior)
			attron(A_REVERSE | A_BOLD);
		else if (n->stan <= 0)
			attron(A_DIM);

		mvprintw(y, box_x + 2, "[%d] %-22s %5.2f zl  %2d/%d",
			 nr, n->nazwa, n->cena_pln, n->stan, MAX_STAN_MAG);

		if (slot_odbioru)
			addstr("  <<ODBIOR>>");
		else if (n->stan <= 0 && !zaznacz)
			addstr("  (brak)");
		else if (zaznacz && !odbior)
			addstr("  <<");

		if (slot_odbioru)
			attroff(A_BOLD | COLOR_PAIR(3));
		else if (zaznacz && !odbior)
			attroff(A_REVERSE | A_BOLD);
		else if (n->stan <= 0)
			attroff(A_DIM);
	}

	int msg_y = 7 + PRODUKTOW + 1;
	ramka(msg_y, box_x, 5, box_w);
	mvprintw(msg_y, box_x + 2, " KOMUNIKAT ");
	wypisz_komunikat(msg_y + 2, box_x, box_w, resp->text, resp->ok);

	if (tryb_portfel) {
		mvprintw(wys - 2, 2,
			 " Doladuj portfel gotowki (zl) + Enter | Esc = anuluj: [%s]",
			 bufor_portfel);
	} else if (tryb_doladowanie) {
		mvprintw(wys - 2, 2,
			 " Doladuj karte (zl) + Enter | Esc = anuluj: [%s]",
			 bufor_doladowanie);
	} else if (tryb_wlasna_kwota) {
		mvprintw(wys - 2, 2,
			 " Kwota gotowki (zl) + Enter | Esc = anuluj: [%s]",
			 bufor_kwoty);
	} else if (odbior > 0) {
		mvprintw(wys - 2, 2,
			 " Enter / o = odbior  |  q = wyjscie (stan sie zapisze) ");
	} else {
		mvprintw(wys - 3, 2,
			 " z/x/v/c - wrzut z portfela | Enter/k - kup | p - karta | r - reszta do portfela | l/g - doladuj ");
		mvprintw(wys - 2, 2,
			 " u/U - uzupelnij slot | q - wyjscie | 1-10 wybor ");
	}

	refresh();
}

int klient_run(int fd_do_automatu, int fd_od_automatu, pid_t pid_automatu)
{
	AutomatResp resp;
	AutomatReq req = {.cmd = CMD_GET_STATE};
	if (wyslij(fd_do_automatu, fd_od_automatu, &req, &resp) != 0)
		return 1;

	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);
	curs_set(0);
	if (has_colors()) {
		start_color();
		use_default_colors();
		init_pair(1, COLOR_CYAN, -1);
		init_pair(2, COLOR_WHITE, -1);
		init_pair(3, COLOR_GREEN, -1);
		init_pair(4, COLOR_RED, -1);
	}

	signal(SIGINT, obsluga_sigint);

	int wybor = 1;
	int tryb_kwota = 0;
	int tryb_doladowanie = 0;
	int tryb_portfel = 0;
	char bufor_kwoty[32] = "";
	char bufor_doladowanie[32] = "";
	char bufor_portfel[32] = "";
	resp.ok = 1;
	snprintf(resp.text, sizeof(resp.text),
		 "Portfel %.0f zl do wrzucenia. z/x/v/c = wrzut z portfela.",
		 PORTFEL_GOTOWKI_DOMYSLNE);

	for (;;) {
		if (przerwano) {
			snprintf(resp.text, sizeof(resp.text),
				 "Przerwano (Ctrl+C).");
			resp.ok = 0;
			break;
		}

		int odbior = resp.stan.oczekuje_odbioru;
		rysuj_panel(&resp, wybor, pid_automatu, tryb_kwota, bufor_kwoty,
			    tryb_doladowanie, bufor_doladowanie, tryb_portfel,
			    bufor_portfel);
		int ch = getch();

		if (tryb_portfel) {
			if (ch == 27) {
				tryb_portfel = 0;
				bufor_portfel[0] = '\0';
				snprintf(resp.text, sizeof(resp.text),
					 "Anulowano doladowanie portfela.");
				resp.ok = 1;
				continue;
			}
			if (ch == '\n' || ch == KEY_ENTER) {
				req.cmd = CMD_PORTFEL_TOPUP;
				req.kwota_pln = strtod(bufor_portfel, NULL);
				if (wyslij(fd_do_automatu, fd_od_automatu, &req,
					   &resp) != 0)
					break;
				tryb_portfel = 0;
				bufor_portfel[0] = '\0';
				continue;
			}
			if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
				size_t len = strlen(bufor_portfel);
				if (len > 0)
					bufor_portfel[len - 1] = '\0';
				continue;
			}
			if ((ch >= '0' && ch <= '9') || ch == '.' || ch == ',') {
				size_t len = strlen(bufor_portfel);
				if (len < sizeof(bufor_portfel) - 2) {
					bufor_portfel[len] =
					    (char)((ch == ',') ? '.' : ch);
					bufor_portfel[len + 1] = '\0';
				}
				continue;
			}
			continue;
		}

		if (tryb_doladowanie) {
			if (ch == 27) {
				tryb_doladowanie = 0;
				bufor_doladowanie[0] = '\0';
				snprintf(resp.text, sizeof(resp.text),
					 "Anulowano doladowanie karty.");
				resp.ok = 1;
				continue;
			}
			if (ch == '\n' || ch == KEY_ENTER) {
				req.cmd = CMD_CARD_TOPUP;
				req.kwota_pln = strtod(bufor_doladowanie, NULL);
				if (wyslij(fd_do_automatu, fd_od_automatu, &req,
					   &resp) != 0)
					break;
				tryb_doladowanie = 0;
				bufor_doladowanie[0] = '\0';
				continue;
			}
			if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
				size_t len = strlen(bufor_doladowanie);
				if (len > 0)
					bufor_doladowanie[len - 1] = '\0';
				continue;
			}
			if ((ch >= '0' && ch <= '9') || ch == '.' || ch == ',') {
				size_t len = strlen(bufor_doladowanie);
				if (len < sizeof(bufor_doladowanie) - 2) {
					bufor_doladowanie[len] =
					    (char)((ch == ',') ? '.' : ch);
					bufor_doladowanie[len + 1] = '\0';
				}
				continue;
			}
			continue;
		}

		if (tryb_kwota) {
			if (ch == 27) {
				tryb_kwota = 0;
				bufor_kwoty[0] = '\0';
				snprintf(resp.text, sizeof(resp.text),
					 "Anulowano wrzut.");
				resp.ok = 1;
				continue;
			}
			if (ch == '\n' || ch == KEY_ENTER) {
				req.cmd = CMD_COIN;
				req.kwota_pln = strtod(bufor_kwoty, NULL);
				if (wyslij(fd_do_automatu, fd_od_automatu, &req,
					   &resp) != 0)
					break;
				tryb_kwota = 0;
				bufor_kwoty[0] = '\0';
				continue;
			}
			if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
				size_t len = strlen(bufor_kwoty);
				if (len > 0)
					bufor_kwoty[len - 1] = '\0';
				continue;
			}
			if ((ch >= '0' && ch <= '9') || ch == '.' || ch == ',') {
				size_t len = strlen(bufor_kwoty);
				if (len < sizeof(bufor_kwoty) - 2) {
					bufor_kwoty[len] =
					    (char)((ch == ',') ? '.' : ch);
					bufor_kwoty[len + 1] = '\0';
				}
				continue;
			}
			continue;
		}

		if (odbior > 0) {
			if (ch == '\n' || ch == KEY_ENTER || ch == 'o' ||
			    ch == 'O') {
				req.cmd = CMD_PICKUP;
				if (wyslij(fd_do_automatu, fd_od_automatu, &req,
					   &resp) != 0)
					break;
				continue;
			}
			if (ch == 'q' || ch == 'Q') {
				req.cmd = CMD_QUIT;
				if (wyslij(fd_do_automatu, fd_od_automatu, &req,
					   &resp) != 0)
					break;
				break;
			}
			snprintf(resp.text, sizeof(resp.text),
				 "Najpierw odbierz produkt (Enter).");
			resp.ok = 0;
			continue;
		}

		if (ch == 'q' || ch == 'Q') {
			req.cmd = CMD_QUIT;
			if (wyslij(fd_do_automatu, fd_od_automatu, &req, &resp) != 0)
				break;
			break;
		}
		if (ch == KEY_UP) {
			wybor--;
			if (wybor < 1)
				wybor = PRODUKTOW;
			continue;
		}
		if (ch == KEY_DOWN) {
			wybor++;
			if (wybor > PRODUKTOW)
				wybor = 1;
			continue;
		}
		if (ch >= '1' && ch <= '0' + PRODUKTOW) {
			wybor = ch - '0';
			continue;
		}
		if (ch == '\n' || ch == KEY_ENTER || ch == 'k' || ch == 'K') {
			req.cmd = CMD_BUY;
			req.arg = wybor;
			if (wyslij(fd_do_automatu, fd_od_automatu, &req, &resp) != 0)
				break;
			continue;
		}
		if (ch == 'p' || ch == 'P') {
			req.cmd = CMD_BUY_CARD;
			req.arg = wybor;
			if (wyslij(fd_do_automatu, fd_od_automatu, &req, &resp) != 0)
				break;
			continue;
		}
		if (ch == 'l' || ch == 'L') {
			tryb_doladowanie = 1;
			bufor_doladowanie[0] = '\0';
			continue;
		}
		if (ch == 'g' || ch == 'G') {
			tryb_portfel = 1;
			bufor_portfel[0] = '\0';
			continue;
		}
		if (ch == 'z' || ch == 'Z') {
			req.cmd = CMD_COIN;
			req.kwota_pln = 1.0;
			if (wyslij(fd_do_automatu, fd_od_automatu, &req, &resp) != 0)
				break;
			continue;
		}
		if (ch == 'x' || ch == 'X') {
			req.cmd = CMD_COIN;
			req.kwota_pln = 2.0;
			if (wyslij(fd_do_automatu, fd_od_automatu, &req, &resp) != 0)
				break;
			continue;
		}
		if (ch == 'v' || ch == 'V') {
			req.cmd = CMD_COIN;
			req.kwota_pln = 5.0;
			if (wyslij(fd_do_automatu, fd_od_automatu, &req, &resp) != 0)
				break;
			continue;
		}
		if (ch == 'r' || ch == 'R') {
			req.cmd = CMD_CHANGE;
			if (wyslij(fd_do_automatu, fd_od_automatu, &req, &resp) != 0)
				break;
			continue;
		}
		if (ch == 'u') {
			req.cmd = CMD_RESTOCK;
			req.arg = wybor;
			if (wyslij(fd_do_automatu, fd_od_automatu, &req, &resp) != 0)
				break;
			continue;
		}
		if (ch == 'U') {
			req.cmd = CMD_RESTOCK_FILL;
			req.arg = wybor;
			if (wyslij(fd_do_automatu, fd_od_automatu, &req, &resp) != 0)
				break;
			continue;
		}
		if (ch == 'c' || ch == 'C') {
			tryb_kwota = 1;
			bufor_kwoty[0] = '\0';
			continue;
		}
		snprintf(resp.text, sizeof(resp.text),
			 "Nieznany klawisz — pomoc na dole ekranu.");
		resp.ok = 0;
	}

	endwin();

	if (przerwano) {
		req.cmd = CMD_QUIT;
		(void)wyslij(fd_do_automatu, fd_od_automatu, &req, &resp);
		printf("\n%s\n", resp.text);
	} else {
		printf("%s\n", resp.text);
	}

	return 0;
}
