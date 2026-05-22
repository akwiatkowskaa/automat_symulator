#ifndef AUTOMAT_H
#define AUTOMAT_H

#define PRODUKTOW 10
#define SLOTOW_NAPOJE 5
#define MAX_STAN_MAG 8
#define SALDO_KARTY_DOMYSLNE 200.0
#define PORTFEL_GOTOWKI_DOMYSLNE 50.0 /* gotowka w portfelu do wrzucania */
#define PLIK_STANU "automat_stan.dat"

enum {
	CMD_QUIT = 0,
	CMD_GET_STATE = 1,
	CMD_COIN = 2,
	CMD_BUY = 3,
	CMD_CHANGE = 4,
	CMD_RESTOCK = 5,
	CMD_BUY_CARD = 6,
	CMD_PICKUP = 7,
	CMD_CARD_TOPUP = 8,
	CMD_RESTOCK_FILL = 9,
	CMD_PORTFEL_TOPUP = 10, /* doladowanie portfela gotowki */
};

typedef struct {
	char nazwa[36];
	double cena_pln;
	int stan;
} ProduktInfo;

typedef struct {
	double saldo_pln; /* gotowka juz wrzucona do automatu */
	double portfel_gotowki_pln; /* portfel: ile mozesz jeszcze wrzucic */
	double saldo_karty_pln;
	int oczekuje_odbioru;
	ProduktInfo produkty[PRODUKTOW];
} AutomatStan;

typedef struct {
	int cmd;
	int arg;
	double kwota_pln;
} AutomatReq;

typedef struct {
	int ok;
	char text[256];
	AutomatStan stan;
} AutomatResp;

void automat_run(int fd_req, int fd_resp);

#endif
