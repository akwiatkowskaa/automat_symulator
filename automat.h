#ifndef AUTOMAT_H
#define AUTOMAT_H

#define NAPOJOW 6
#define MAX_STAN_MAG 8

enum {
	CMD_QUIT = 0,
	CMD_GET_STATE = 1,
	CMD_COIN = 2,
	CMD_BUY = 3,
	CMD_CHANGE = 4,
	CMD_RESTOCK = 5,
};

typedef struct {
	char nazwa[36];
	double cena_pln;
	int stan;
} NapojInfo;

typedef struct {
	double saldo_pln;
	NapojInfo napoje[NAPOJOW];
} AutomatStan;

typedef struct {
	int cmd;
	int arg; /* numer napoju 1..NAPOJOW (BUY, RESTOCK jednego) */
	double kwota_pln;
} AutomatReq;

typedef struct {
	int ok;
	char text[256];
	AutomatStan stan;
} AutomatResp;

void automat_run(int fd_req, int fd_resp);

#endif
