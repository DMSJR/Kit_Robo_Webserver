#ifndef CONST_H
#define CONST_H

const uint8_t PIN_IN = 2;
const uint8_t PIN_OUT = 0;
const uint8_t PIN_RX = 3;
const uint8_t TAMANHO_MAXIMO_DADOS = 7;
const uint8_t I_SSID = 0, I_PASS = 1, I_IP = 2, I_GATEWAY = 3;
const unsigned TEMPO_INICIO_WIFI = 5000;
const unsigned INTERVALO_WIFI = 15000;
const uint8_t QTD_ARQUIVOS = 4;
const char CARACTERE_INICIO = ':';
const char CARACTERE_FINAL = '\n';
const char *PARAM_INPUT_CRED = "cred";
const char *NOME_AP = "ROBO_ELETROGATE", *SENHA_AP = "123456789";
const char PARAM_VEC[][8] = {"ssid", "pass", "ip", "gateway"};
const char PATHS[][13] = {"/ssid.txt" ,"/pass.txt" ,"/ip.txt" ,"/gateway.txt"};

#endif
