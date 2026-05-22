#include "automat.h"
#include "ipc.h"

#include <stdio.h>
#include <string.h>

#define STAN_MAGIC 0x31544D33u /* 'ATM3' — portfel gotowki */

typedef struct {
	unsigned magic;
	double saldo_pln;
	double portfel_gotowki;
	double saldo_karty;
	int stany[PRODUKTOW];
} StanPlik;

typedef struct {
	char nazwa[36];
	double cena_pln;
	int stan;
} Produkt;

static void uzupelnij_stan_w_odpowiedzi(AutomatResp *resp, const Produkt *mag,
					double saldo, double portfel,
					double saldo_karty, int oczekuje_odbioru)
{
	resp->stan.saldo_pln = saldo;
	resp->stan.portfel_gotowki_pln = portfel;
	resp->stan.saldo_karty_pln = saldo_karty;
	resp->stan.oczekuje_odbioru = oczekuje_odbioru;
	for (int i = 0; i < PRODUKTOW; i++) {
		snprintf(resp->stan.produkty[i].nazwa,
			 sizeof(resp->stan.produkty[i].nazwa), "%s",
			 mag[i].nazwa);
		resp->stan.produkty[i].cena_pln = mag[i].cena_pln;
		resp->stan.produkty[i].stan = mag[i].stan;
	}
}

static void domyslny_magazyn(Produkt *mag)
{
	const Produkt start[PRODUKTOW] = {
	    {"Coca-cola", 3.50, 5},
	    {"Woda niegazowana", 2.50, 5},
	    {"Woda gazowana", 2.50, 5},
	    {"Sok pomaranczowy", 4.00, 4},
	    {"Ice tea", 5.00, 4},
	    {"Paluszki solone", 3.00, 6},
	    {"Zelki owocowe", 6.00, 5},
	    {"Chipsy", 5.00, 5},
	    {"Orzeszki solone", 6.00, 4},
	    {"Baton musli", 4.00, 5},
	};
	memcpy(mag, start, sizeof(start));
}

static int zapisz_do_pliku(double saldo, double portfel, double saldo_karty,
			   const Produkt *mag)
{
	StanPlik z = {.magic = STAN_MAGIC,
		      .saldo_pln = saldo,
		      .portfel_gotowki = portfel,
		      .saldo_karty = saldo_karty};
	for (int i = 0; i < PRODUKTOW; i++)
		z.stany[i] = mag[i].stan;

	FILE *f = fopen(PLIK_STANU, "wb");
	if (!f)
		return -1;
	if (fwrite(&z, sizeof(z), 1, f) != 1) {
		fclose(f);
		return -1;
	}
	fclose(f);
	return 0;
}

static int wczytaj_z_pliku(double *saldo, double *portfel, double *saldo_karty,
			   Produkt *mag)
{
	FILE *f = fopen(PLIK_STANU, "rb");
	if (!f)
		return -1;

	StanPlik z;
	if (fread(&z, sizeof(z), 1, f) != 1) {
		fclose(f);
		return -1;
	}
	fclose(f);

	if (z.magic != STAN_MAGIC)
		return -1;

	*saldo = z.saldo_pln;
	*portfel = z.portfel_gotowki;
	*saldo_karty = z.saldo_karty;
	for (int i = 0; i < PRODUKTOW; i++) {
		if (z.stany[i] < 0 || z.stany[i] > MAX_STAN_MAG)
			return -1;
		mag[i].stan = z.stany[i];
	}
	return 0;
}

static void zapisz_stan(double saldo, double portfel, double saldo_karty,
			const Produkt *mag)
{
	(void)zapisz_do_pliku(saldo, portfel, saldo_karty, mag);
}

static int blokuje_przez_odbior(int oczekuje, AutomatResp *resp, const Produkt *mag,
				double saldo, double portfel, double saldo_karty)
{
	if (oczekuje <= 0)
		return 0;
	resp->ok = 0;
	snprintf(resp->text, sizeof(resp->text),
		 "Odbierz produkt ze slotu [%d] (%s) — Enter.",
		 oczekuje, mag[oczekuje - 1].nazwa);
	uzupelnij_stan_w_odpowiedzi(resp, mag, saldo, portfel, saldo_karty,
				    oczekuje);
	return 1;
}

static int wykonaj_zakup(Produkt *mag, int nr, double *saldo, double *saldo_karty,
			 int karta, int *oczekuje, AutomatResp *resp)
{
	if (nr < 1 || nr > PRODUKTOW) {
		resp->ok = 0;
		snprintf(resp->text, sizeof(resp->text), "Zly numer (1–%d).",
			 PRODUKTOW);
		return 0;
	}
	int i = nr - 1;
	if (mag[i].stan <= 0) {
		resp->ok = 0;
		snprintf(resp->text, sizeof(resp->text), "Brak: %s.", mag[i].nazwa);
		return 0;
	}
	if (karta) {
		if (*saldo_karty + 1e-9 < mag[i].cena_pln) {
			resp->ok = 0;
			snprintf(resp->text, sizeof(resp->text),
				 "Za malo na karcie (%.2f zl, masz %.2f zl).",
				 mag[i].cena_pln, *saldo_karty);
			return 0;
		}
		*saldo_karty -= mag[i].cena_pln;
	} else if (*saldo + 1e-9 < mag[i].cena_pln) {
		resp->ok = 0;
		snprintf(resp->text, sizeof(resp->text),
			 "Za malo w automacie (%.2f zl, masz %.2f zl).",
			 mag[i].cena_pln, *saldo);
		return 0;
	} else {
		*saldo -= mag[i].cena_pln;
	}

	mag[i].stan--;
	*oczekuje = nr;
	resp->ok = 1;
	if (karta)
		snprintf(resp->text, sizeof(resp->text),
			 "Karta: -%.2f zl. Odbierz [%d] %s. Na karcie: %.2f zl.",
			 mag[i].cena_pln, nr, mag[i].nazwa, *saldo_karty);
	else
		snprintf(resp->text, sizeof(resp->text),
			 "Gotowka: -%.2f zl. Odbierz [%d] %s. W automacie: %.2f zl.",
			 mag[i].cena_pln, nr, mag[i].nazwa, *saldo);
	return 1;
}

static int dodaj_sztuke(Produkt *mag, int nr, AutomatResp *resp)
{
	if (nr < 1 || nr > PRODUKTOW) {
		resp->ok = 0;
		snprintf(resp->text, sizeof(resp->text), "Zly slot.");
		return 0;
	}
	int i = nr - 1;
	if (mag[i].stan >= MAX_STAN_MAG) {
		resp->ok = 0;
		snprintf(resp->text, sizeof(resp->text),
			 "[%d] pelny (max %d szt.).", nr, MAX_STAN_MAG);
		return 0;
	}
	mag[i].stan++;
	resp->ok = 1;
	snprintf(resp->text, sizeof(resp->text),
		 "Dodano 1 szt. do [%d] %s (stan: %d/%d).", nr, mag[i].nazwa,
		 mag[i].stan, MAX_STAN_MAG);
	return 1;
}

static int uzupelnij_slot(Produkt *mag, int nr, AutomatResp *resp)
{
	if (nr < 1 || nr > PRODUKTOW) {
		resp->ok = 0;
		snprintf(resp->text, sizeof(resp->text), "Zly slot.");
		return 0;
	}
	int i = nr - 1;
	mag[i].stan = MAX_STAN_MAG;
	resp->ok = 1;
	snprintf(resp->text, sizeof(resp->text),
		 "Uzupelniono [%d] %s do %d szt.", nr, mag[i].nazwa,
		 MAX_STAN_MAG);
	return 1;
}

void automat_run(int fd_req, int fd_resp)
{
	Produkt magazyn[PRODUKTOW];
	domyslny_magazyn(magazyn);

	double saldo_pln = 0.0;
	double portfel_gotowki_pln = PORTFEL_GOTOWKI_DOMYSLNE;
	double saldo_karty_pln = SALDO_KARTY_DOMYSLNE;
	int oczekuje_odbioru = 0;

	if (wczytaj_z_pliku(&saldo_pln, &portfel_gotowki_pln, &saldo_karty_pln,
			    magazyn) != 0) {
		portfel_gotowki_pln = PORTFEL_GOTOWKI_DOMYSLNE;
		saldo_karty_pln = SALDO_KARTY_DOMYSLNE;
	}

	for (;;) {
		AutomatReq req;
		AutomatResp resp;
		memset(&resp, 0, sizeof(resp));

		if (pelny_odczyt(fd_req, &req, sizeof(req)) <= 0)
			break;

		switch (req.cmd) {
		case CMD_QUIT:
			resp.ok = 1;
			snprintf(resp.text, sizeof(resp.text),
				 "Zapisano stan. Do widzenia.");
			zapisz_stan(saldo_pln, portfel_gotowki_pln, saldo_karty_pln,
				    magazyn);
			uzupelnij_stan_w_odpowiedzi(&resp, magazyn, saldo_pln,
						    portfel_gotowki_pln,
						    saldo_karty_pln,
						    oczekuje_odbioru);
			(void)pelny_zapis(fd_resp, &resp, sizeof(resp));
			return;

		case CMD_GET_STATE:
			resp.ok = 1;
			if (oczekuje_odbioru > 0)
				snprintf(resp.text, sizeof(resp.text),
					 "Odbierz produkt ze slotu [%d].",
					 oczekuje_odbioru);
			else
				snprintf(resp.text, sizeof(resp.text),
					 "Portfel: %.2f zl do wrzucenia.",
					 portfel_gotowki_pln);
			uzupelnij_stan_w_odpowiedzi(&resp, magazyn, saldo_pln,
						    portfel_gotowki_pln,
						    saldo_karty_pln,
						    oczekuje_odbioru);
			break;

		case CMD_PICKUP:
			if (oczekuje_odbioru <= 0) {
				resp.ok = 0;
				snprintf(resp.text, sizeof(resp.text),
					 "Brak produktu do odebrania.");
			} else {
				int slot = oczekuje_odbioru;
				resp.ok = 1;
				snprintf(resp.text, sizeof(resp.text),
					 "Odebrano ze slotu [%d]. Milego dnia!",
					 slot);
				oczekuje_odbioru = 0;
			}
			uzupelnij_stan_w_odpowiedzi(&resp, magazyn, saldo_pln,
						    portfel_gotowki_pln,
						    saldo_karty_pln,
						    oczekuje_odbioru);
			zapisz_stan(saldo_pln, portfel_gotowki_pln, saldo_karty_pln,
				    magazyn);
			break;

		case CMD_CARD_TOPUP:
			if (blokuje_przez_odbior(oczekuje_odbioru, &resp, magazyn,
						 saldo_pln, portfel_gotowki_pln,
						 saldo_karty_pln))
				break;
			if (req.kwota_pln <= 0.0 || req.kwota_pln > 500.0) {
				resp.ok = 0;
				snprintf(resp.text, sizeof(resp.text),
					 "Doladowanie: 0.01–500 zl.");
			} else {
				saldo_karty_pln += req.kwota_pln;
				resp.ok = 1;
				snprintf(resp.text, sizeof(resp.text),
					 "Karta +%.2f zl. Saldo karty: %.2f zl.",
					 req.kwota_pln, saldo_karty_pln);
			}
			uzupelnij_stan_w_odpowiedzi(&resp, magazyn, saldo_pln,
						    portfel_gotowki_pln,
						    saldo_karty_pln,
						    oczekuje_odbioru);
			zapisz_stan(saldo_pln, portfel_gotowki_pln, saldo_karty_pln,
				    magazyn);
			break;

		case CMD_PORTFEL_TOPUP:
			if (blokuje_przez_odbior(oczekuje_odbioru, &resp, magazyn,
						 saldo_pln, portfel_gotowki_pln,
						 saldo_karty_pln))
				break;
			if (req.kwota_pln <= 0.0 || req.kwota_pln > 500.0) {
				resp.ok = 0;
				snprintf(resp.text, sizeof(resp.text),
					 "Doladowanie: 0.01–500 zl.");
			} else {
				portfel_gotowki_pln += req.kwota_pln;
				resp.ok = 1;
				snprintf(resp.text, sizeof(resp.text),
					 "Portfel +%.2f zl. W portfelu: %.2f zl.",
					 req.kwota_pln, portfel_gotowki_pln);
			}
			uzupelnij_stan_w_odpowiedzi(&resp, magazyn, saldo_pln,
						    portfel_gotowki_pln,
						    saldo_karty_pln,
						    oczekuje_odbioru);
			zapisz_stan(saldo_pln, portfel_gotowki_pln, saldo_karty_pln,
				    magazyn);
			break;

		case CMD_COIN:
			if (blokuje_przez_odbior(oczekuje_odbioru, &resp, magazyn,
						 saldo_pln, portfel_gotowki_pln,
						 saldo_karty_pln))
				break;
			if (req.kwota_pln <= 0.0 || req.kwota_pln > 100.0) {
				resp.ok = 0;
				snprintf(resp.text, sizeof(resp.text),
					 "Kwota poza zakresem (0.01–100 zl).");
			} else if (req.kwota_pln > portfel_gotowki_pln + 1e-9) {
				resp.ok = 0;
				snprintf(resp.text, sizeof(resp.text),
					 "W portfelu masz tylko %.2f zl.",
					 portfel_gotowki_pln);
			} else {
				portfel_gotowki_pln -= req.kwota_pln;
				saldo_pln += req.kwota_pln;
				resp.ok = 1;
				snprintf(resp.text, sizeof(resp.text),
					 "Wrzucono %.2f zl z portfela. W automacie: "
					 "%.2f zl | portfel: %.2f zl.",
					 req.kwota_pln, saldo_pln,
					 portfel_gotowki_pln);
			}
			uzupelnij_stan_w_odpowiedzi(&resp, magazyn, saldo_pln,
						    portfel_gotowki_pln,
						    saldo_karty_pln,
						    oczekuje_odbioru);
			zapisz_stan(saldo_pln, portfel_gotowki_pln, saldo_karty_pln,
				    magazyn);
			break;

		case CMD_CHANGE:
			if (blokuje_przez_odbior(oczekuje_odbioru, &resp, magazyn,
						 saldo_pln, portfel_gotowki_pln,
						 saldo_karty_pln))
				break;
			if (saldo_pln < 1e-9) {
				resp.ok = 0;
				snprintf(resp.text, sizeof(resp.text),
					 "Brak reszty w automacie.");
			} else {
				double wydano = saldo_pln;
				portfel_gotowki_pln += saldo_pln;
				saldo_pln = 0.0;
				resp.ok = 1;
				snprintf(resp.text, sizeof(resp.text),
					 "Reszta %.2f zl -> portfel. Portfel: %.2f "
					 "zl.",
					 wydano, portfel_gotowki_pln);
			}
			uzupelnij_stan_w_odpowiedzi(&resp, magazyn, saldo_pln,
						    portfel_gotowki_pln,
						    saldo_karty_pln,
						    oczekuje_odbioru);
			zapisz_stan(saldo_pln, portfel_gotowki_pln, saldo_karty_pln,
				    magazyn);
			break;

		case CMD_RESTOCK:
			(void)dodaj_sztuke(magazyn, req.arg, &resp);
			uzupelnij_stan_w_odpowiedzi(&resp, magazyn, saldo_pln,
						    portfel_gotowki_pln,
						    saldo_karty_pln,
						    oczekuje_odbioru);
			zapisz_stan(saldo_pln, portfel_gotowki_pln, saldo_karty_pln,
				    magazyn);
			break;

		case CMD_RESTOCK_FILL:
			(void)uzupelnij_slot(magazyn, req.arg, &resp);
			uzupelnij_stan_w_odpowiedzi(&resp, magazyn, saldo_pln,
						    portfel_gotowki_pln,
						    saldo_karty_pln,
						    oczekuje_odbioru);
			zapisz_stan(saldo_pln, portfel_gotowki_pln, saldo_karty_pln,
				    magazyn);
			break;

		case CMD_BUY:
			if (blokuje_przez_odbior(oczekuje_odbioru, &resp, magazyn,
						 saldo_pln, portfel_gotowki_pln,
						 saldo_karty_pln))
				break;
			(void)wykonaj_zakup(magazyn, req.arg, &saldo_pln,
					    &saldo_karty_pln, 0, &oczekuje_odbioru,
					    &resp);
			uzupelnij_stan_w_odpowiedzi(&resp, magazyn, saldo_pln,
						    portfel_gotowki_pln,
						    saldo_karty_pln,
						    oczekuje_odbioru);
			zapisz_stan(saldo_pln, portfel_gotowki_pln, saldo_karty_pln,
				    magazyn);
			break;

		case CMD_BUY_CARD:
			if (blokuje_przez_odbior(oczekuje_odbioru, &resp, magazyn,
						 saldo_pln, portfel_gotowki_pln,
						 saldo_karty_pln))
				break;
			(void)wykonaj_zakup(magazyn, req.arg, &saldo_pln,
					    &saldo_karty_pln, 1, &oczekuje_odbioru,
					    &resp);
			uzupelnij_stan_w_odpowiedzi(&resp, magazyn, saldo_pln,
						    portfel_gotowki_pln,
						    saldo_karty_pln,
						    oczekuje_odbioru);
			zapisz_stan(saldo_pln, portfel_gotowki_pln, saldo_karty_pln,
				    magazyn);
			break;

		default:
			resp.ok = 0;
			snprintf(resp.text, sizeof(resp.text), "Nieznana komenda.");
			uzupelnij_stan_w_odpowiedzi(&resp, magazyn, saldo_pln,
						    portfel_gotowki_pln,
						    saldo_karty_pln,
						    oczekuje_odbioru);
			break;
		}

		if (pelny_zapis(fd_resp, &resp, sizeof(resp)) <= 0)
			break;
	}
}
