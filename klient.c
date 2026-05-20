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

static void rysuj_panel(const AutomatResp *resp, int wybor, pid_t pid_aut,
			int tryb_wlasna_kwota, const char *bufor_kwoty)
{
	const AutomatStan *s = &resp->stan;
	int wys, szer;
	getmaxyx(stdscr, wys, szer);

	clear();
	color_set(1, NULL);
	mvprintw(0, 2, " AUTOMAT Z NAPOJAMI ");
	color_set(2, NULL);
	mvprintw(1, 2, " Klient PID %-6d | Automat PID %-6d | pipe + fork ",
		 (int)getpid(), (int)pid_aut);

	attron(A_BOLD);
	mvprintw(3, 2, " SALDO: %6.2f zl ", s->saldo_pln);
	attroff(A_BOLD);

	int box_w = szer - 4;
	if (box_w > 72)
		box_w = 72;
	int box_x = (szer - box_w) / 2;
	if (box_x < 1)
		box_x = 1;

	ramka(5, box_x, NAPOJOW + 2, box_w);
	mvprintw(5, box_x + 2, " NAPOJE (strzalki / 1-%d) ", NAPOJOW);

	for (int i = 0; i < NAPOJOW; i++) {
		int y = 6 + i;
		const NapojInfo *n = &s->napoje[i];
		int zaznacz = (i + 1 == wybor);

		if (zaznacz)
			attron(A_REVERSE | A_BOLD);
		else if (n->stan <= 0)
			attron(A_DIM);

		mvprintw(y, box_x + 2, "[%d] %-22s %5.2f zl  stan:%2d",
			 i + 1, n->nazwa, n->cena_pln, n->stan);

		if (n->stan <= 0 && !zaznacz)
			addstr("  (brak)");
		if (zaznacz)
			addstr("  <<");

		if (zaznacz)
			attroff(A_REVERSE | A_BOLD);
		else if (n->stan <= 0)
			attroff(A_DIM);
	}

	int msg_y = 6 + NAPOJOW + 1;
	ramka(msg_y, box_x, 4, box_w);
	mvprintw(msg_y, box_x + 2, " KOMUNIKAT ");
	color_set(resp->ok ? 3 : 4, NULL);
	mvprintw(msg_y + 1, box_x + 2, " %-*.s ", box_w - 6,
		 resp->text[0] ? resp->text : " ");
	color_set(2, NULL);

	if (tryb_wlasna_kwota) {
		mvprintw(wys - 2, 2,
			 " Kwota (zl) + Enter | Esc = anuluj: [%s]",
			 bufor_kwoty);
	} else {
		mvprintw(wys - 3, 2,
			 " z/x/v = 1/2/5 zl | Enter/k = kup | r = reszta | "
			 "u = uzupelnij | c = inna kwota | q = wyjscie ");
		mvprintw(wys - 2, 2,
			 " strzalki lub 1-%d = wybor napoju ", NAPOJOW);
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
	char bufor_kwoty[32] = "";
	resp.ok = 1;
	snprintf(resp.text, sizeof(resp.text),
		 "Witaj! Wrzuc monety (z/x/v) i kup napoj (Enter).");

	for (;;) {
		if (przerwano) {
			snprintf(resp.text, sizeof(resp.text),
				 "Przerwano (Ctrl+C).");
			resp.ok = 0;
			break;
		}

		rysuj_panel(&resp, wybor, pid_automatu, tryb_kwota, bufor_kwoty);
		int ch = getch();

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
				double kw = strtod(bufor_kwoty, NULL);
				req.cmd = CMD_COIN;
				req.kwota_pln = kw;
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

		if (ch == 'q' || ch == 'Q') {
			req.cmd = CMD_QUIT;
			if (wyslij(fd_do_automatu, fd_od_automatu, &req, &resp) != 0)
				break;
			break;
		}
		if (ch == KEY_UP) {
			wybor--;
			if (wybor < 1)
				wybor = NAPOJOW;
			continue;
		}
		if (ch == KEY_DOWN) {
			wybor++;
			if (wybor > NAPOJOW)
				wybor = 1;
			continue;
		}
		if (ch >= '1' && ch <= '0' + NAPOJOW && ch != '\n') {
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
		if (ch == 'u' || ch == 'U') {
			req.cmd = CMD_RESTOCK;
			req.arg = 0;
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
			 "Nieznany klawisz (pomoc na dole ekranu).");
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
