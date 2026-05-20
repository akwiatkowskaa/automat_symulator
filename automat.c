#include "automat.h"
#include "ipc.h"

#include <stdio.h>
#include <string.h>

typedef struct {
	char nazwa[36];
	double cena_pln;
	int stan;
} Napoj;

static void uzupelnij_stan_w_odpowiedzi(AutomatResp *resp, const Napoj *mag,
					double saldo)
{
	resp->stan.saldo_pln = saldo;
	for (int i = 0; i < NAPOJOW; i++) {
		snprintf(resp->stan.napoje[i].nazwa,
			 sizeof(resp->stan.napoje[i].nazwa), "%s",
			 mag[i].nazwa);
		resp->stan.napoje[i].cena_pln = mag[i].cena_pln;
		resp->stan.napoje[i].stan = mag[i].stan;
	}
}

static void uzupelnij_magazyn(Napoj *mag, int nr)
{
	if (nr == 0) {
		for (int i = 0; i < NAPOJOW; i++)
			mag[i].stan = MAX_STAN_MAG;
		return;
	}
	if (nr >= 1 && nr <= NAPOJOW)
		mag[nr - 1].stan = MAX_STAN_MAG;
}

void automat_run(int fd_req, int fd_resp)
{
	Napoj magazyn[NAPOJOW] = {
	    {"Cola 330 ml", 3.50, 5},
	    {"Woda niegazowana", 2.50, 5},
	    {"Sok pomaranczowy", 4.00, 5},
	    {"Energetyk", 4.50, 3},
	    {"Herbata mrozona", 5.00, 4},
	    {"Kawa mrozona", 6.00, 4},
	};
	double saldo_pln = 0.0;

	for (;;) {
		AutomatReq req;
		AutomatResp resp;
		memset(&resp, 0, sizeof(resp));

		if (pelny_odczyt(fd_req, &req, sizeof(req)) <= 0)
			break;

		switch (req.cmd) {
		case CMD_QUIT:
			resp.ok = 1;
			snprintf(resp.text, sizeof(resp.text), "Automat wylaczony.");
			uzupelnij_stan_w_odpowiedzi(&resp, magazyn, saldo_pln);
			(void)pelny_zapis(fd_resp, &resp, sizeof(resp));
			return;

		case CMD_GET_STATE:
			resp.ok = 1;
			snprintf(resp.text, sizeof(resp.text), "Stan automatu.");
			uzupelnij_stan_w_odpowiedzi(&resp, magazyn, saldo_pln);
			break;

		case CMD_COIN:
			if (req.kwota_pln <= 0.0 || req.kwota_pln > 100.0) {
				resp.ok = 0;
				snprintf(resp.text, sizeof(resp.text),
					 "Kwota poza zakresem (0.01–100 zl).");
			} else {
				saldo_pln += req.kwota_pln;
				resp.ok = 1;
				snprintf(resp.text, sizeof(resp.text),
					 "Wrzucono %.2f zl.", req.kwota_pln);
			}
			uzupelnij_stan_w_odpowiedzi(&resp, magazyn, saldo_pln);
			break;

		case CMD_CHANGE:
			if (saldo_pln < 1e-9) {
				resp.ok = 0;
				snprintf(resp.text, sizeof(resp.text),
					 "Brak reszty do wydania.");
			} else {
				double wydano = saldo_pln;
				saldo_pln = 0.0;
				resp.ok = 1;
				snprintf(resp.text, sizeof(resp.text),
					 "Wydano reszte: %.2f zl.", wydano);
			}
			uzupelnij_stan_w_odpowiedzi(&resp, magazyn, saldo_pln);
			break;

		case CMD_RESTOCK:
			uzupelnij_magazyn(magazyn, req.arg);
			resp.ok = 1;
			if (req.arg == 0)
				snprintf(resp.text, sizeof(resp.text),
					 "Uzupelniono caly magazyn.");
			else
				snprintf(resp.text, sizeof(resp.text),
					 "Uzupelniono napoj nr %d.", req.arg);
			uzupelnij_stan_w_odpowiedzi(&resp, magazyn, saldo_pln);
			break;

		case CMD_BUY: {
			int nr = req.arg;
			if (nr < 1 || nr > NAPOJOW) {
				resp.ok = 0;
				snprintf(resp.text, sizeof(resp.text),
					 "Zly numer (1–%d).", NAPOJOW);
				uzupelnij_stan_w_odpowiedzi(&resp, magazyn,
							    saldo_pln);
				break;
			}
			int i = nr - 1;
			if (magazyn[i].stan <= 0) {
				resp.ok = 0;
				snprintf(resp.text, sizeof(resp.text),
					 "Brak: %s.", magazyn[i].nazwa);
			} else if (saldo_pln + 1e-9 < magazyn[i].cena_pln) {
				resp.ok = 0;
				snprintf(resp.text, sizeof(resp.text),
					 "Za malo srodkow (%.2f zl).",
					 magazyn[i].cena_pln);
			} else {
				saldo_pln -= magazyn[i].cena_pln;
				magazyn[i].stan--;
				resp.ok = 1;
				snprintf(resp.text, sizeof(resp.text),
					 "Wydano: %s.", magazyn[i].nazwa);
			}
			uzupelnij_stan_w_odpowiedzi(&resp, magazyn, saldo_pln);
		} break;

		default:
			resp.ok = 0;
			snprintf(resp.text, sizeof(resp.text), "Nieznana komenda.");
			uzupelnij_stan_w_odpowiedzi(&resp, magazyn, saldo_pln);
			break;
		}

		if (pelny_zapis(fd_resp, &resp, sizeof(resp)) <= 0)
			break;
	}
}
