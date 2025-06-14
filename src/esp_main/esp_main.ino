/* @autor: Eletrogate
   @licença: GNU GENERAL PUBLIC LICENSE Version 3 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <LittleFS.h>
#include "constantes.h"
#include "paginaBase.h"

//#define DEBUG
//#define DEBUG_LOOP

bool evtConectado, srvRestart, apagarCredencial, deveIniciarAPSTA; //declara booleanos globais

String parametrosVariaveis[QTD_ARQUIVOS];  // strings variaveis
String selCred;                           // globais

AsyncWebServer server(80);  // instancia o servidor e o atribui à porta 80
AsyncWebSocket ws("/ws");   // instancia o websocket

#ifdef DEBUG
void imprimeTodosArquivos(const char * msg) {
  Serial.print("Inicia imprime todos os arquivos em: "); Serial.println(msg);
  File file[QTD_ARQUIVOS];
  for(size_t i = 0; i < QTD_ARQUIVOS; i ++) {
    Serial.print(PARAM_VEC[i]);
    file[i] = LittleFS.open(PATHS[i], "r");
    Serial.print(": "); Serial.println(file[i].size());
    while(file[i].available())
      Serial.print((char) file[i].read());
    Serial.println("-----");
    file[i].close();
  }
  Serial.print("Encerra imprime todos os arquivos em: "); Serial.println(msg);
}
#endif

String modelos(const String& var) {                                          // callback para construcao da pagina
  String retorno;                                                            // declara string que será enviada para pagina
  if(var == "modeloLista") {                                                 // se for o modelo para receber a lista de redes
    File fileSSID = LittleFS.open(PATHS[I_SSID], "r");                        // abre o arquivo com os nomes das redes
    while(fileSSID.available()) {                                            // enquanto houver conteudo
      String nomeSSID = fileSSID.readStringUntil(CARACTERE_FINAL);            // armazena a linha em nomeSSID
      retorno += "<option value=" + nomeSSID + ">" + nomeSSID + "</option>"; // armazena a tag para listar a rede
    }
    fileSSID.close();                                     // fecha o arquivo com os nomes das redes
  } else if(var == "modeloIP") {                          // se for o modelo para receber o IP atribuido ao ESP
    if(WiFi.getMode() == WIFI_AP)                         // se estiver em AP...
      retorno = "<p>Robo nao conseguiu se conectar.</p>"; // nao houve conexao
    else                                                  // caso contrario
      retorno = "<p>IP: " + WiFi.localIP().toString() + "</p><p>Gateway: " + WiFi.gatewayIP().toString() + "</p>"                               // informa o IP e o gateway
                  + (WiFi.getMode() == WIFI_AP_STA ? R"==(<button class="button" id="dAP" onclick="desligaAP()">Desligar AP</button>)==" : ""); // se estiver em AP_STA, permite desligar AP
  }
  return retorno; // envia a string para a pagina
}

inline bool caractereValido(char c) __attribute__((always_inline)); // declara funcao como inline para poupar processamento (possivelmente o compilador faria isso sozinho)
bool caractereValido(char c) {
  return ((c >= '0' and c <= '9') or c == ' ');  // verifica se o caractere recebido do webserver é um número ou um espaço
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)  { // definição da tratativa de evento de servidor assíncrono
  if(type == WS_EVT_DATA) {                                                        // se for o recebimento de dados e o Arduino puder receber
    char *data_ = (char*) data;                                                    // registra todos os dados recebidos do servidor
    Serial.write(CARACTERE_INICIO);                                                 // envia o caractere de inicio de transmissao
    for(uint8_t i = 0; caractereValido(data_[i]) and i < TAMANHO_MAXIMO_DADOS; i ++) // enquanto for um caractere valido e o tamanho for inferior ao maximo
      Serial.write(data_[i]);                                                      // envia este dado na serial
    Serial.write(CARACTERE_FINAL); }                                                // envia o caractere final para tratamento
  else if(type == WS_EVT_CONNECT) { // se for a conexao à pagina
    digitalWrite(PIN_OUT, LOW);      // indica colocando a saída em nível baixo
    evtConectado = true;            // registra que conectou
    #ifdef DEBUG
      Serial.println("conectado ao websocket");
    #endif
  }
} 

void appendFile(fs::FS &fs, const char * path, const char * message) {
  File file = fs.open(path, "a"); // abre o arquivo para escrita
  file.print(message);            // escreve o conteudo no arquivo
  file.print(CARACTERE_FINAL);     // escreve o caractere finalizador
  file.close();                   // fecha o arquivo
}

void apagaCredencial(fs::FS &fs, const char *credencial) {
  File fileSSID = fs.open(PATHS[I_SSID], "r"); // abre o arquivo com o nome das redes
  uint8_t linhaLeitura = 0;                   // declara o numero da linha lida como 0
  bool credCadastrada = false;                // variavel de controle

  while(fileSSID.available() and !credCadastrada)                               // enquanto houver conteudo e nao tiver encontrado o nome da rede
    if(linhaLeitura++; fileSSID.readStringUntil(CARACTERE_FINAL) == credencial) // incrementa o numero da linha e verifica se o conteudo desta é o nome da rede buscada
      credCadastrada = true;                                                    // se sim, registra que encontrou

  fileSSID.close();           // fecha o arquivo
  if(!credCadastrada) return; // se acabou o conteudo e nao encontrou, nao faz nada

  #ifdef DEBUG
    imprimeTodosArquivos("apagaCredencial1");
  #endif
  for(size_t i = 0; i < QTD_ARQUIVOS; i ++) {                             // ao longo dos quatro arquivos pertinentes às redes
    uint8_t linhaEscrita = 1;                                            // inicia a linha a receber conteudo em 1
    String novoConteudo = "";                                            // inicia o conteudo como string vazia
    char caractereArquivo;                                               // declara variavel para armazenar o caractere
    File file = LittleFS.open(PATHS[i], "r");                            // abre o arquivo
    while(file.available()) {                                            // enquanto houver conteudo
      caractereArquivo = file.read();                                    // le um caractere
      if(linhaEscrita != linhaLeitura) novoConteudo += caractereArquivo; // se nao estiver na linha a ser apagada, armazena o caractere em novoConteudo
      if(caractereArquivo == CARACTERE_FINAL) linhaEscrita ++;            // se for caractere de quebra de linha, incrementa o numero da linha
    }
    file.close();                                                // fecha o arquivo
    file = LittleFS.open(PATHS[i], "w");                         // abre o arquivo para escrita
    file.print(novoConteudo);                                    // escreve o novo conteudo no arquivo
    if(linhaLeitura == linhaEscrita) file.print(CARACTERE_FINAL); // se tiver apagado a ultima linha, escreve uma quebra de linha
    file.close();                                                // fecha o arquivo
  }
  #ifdef DEBUG
    imprimeTodosArquivos("apagaCredencial2");
  #endif
}

void resetaCredenciais(fs::FS &fs) {
  for(size_t i = 0; i < QTD_ARQUIVOS; i ++) // para todos os arquivos pertinentes às redes
    fs.remove(PATHS[i]);                   // apaga o conteudo do arquivo
}

bool initWiFi() {
  #ifdef DEBUG
    imprimeTodosArquivos("initWifi");
  #endif
  WiFi.mode(WIFI_STA);                                  // configura o modo como STA
  bool controleWiFi = false, IPouGatewayVazios = false; // declara variaveis de controle

  File file[QTD_ARQUIVOS];                   // declara vetor de manipulador de arquivos
  for(size_t i = 0; i < QTD_ARQUIVOS; i ++)  // para todos os arquivos
    file[i] = LittleFS.open(PATHS[i], "r"); // abre o arquivo
  
  while(file[0].available()) {                                          // enquanto houver redes cadastradas a serem verificadas
    for(size_t i = 0; i < QTD_ARQUIVOS; i ++)                            // para todos os arquivos
      parametrosVariaveis[i] = file[i].readStringUntil(CARACTERE_FINAL); // armazena o conteudo da linha na correspondente posicao do vetor de parametros

    IPouGatewayVazios = false;                                                  // reseta variavel de controle
    if(parametrosVariaveis[I_IP] == "" or parametrosVariaveis[I_GATEWAY] == "") { // se o IP ou o gateway estiver vazio...
      #ifdef DEBUG
        Serial.println("IP e Gateway vazios");
      #endif
      IPouGatewayVazios = true;                                                    // registra que o IP ou o gateway está vazio
    } else {                                                                       // senao...
      IPAddress IPLocal, gatewayLocal;                                             // declara variaveis referentes a IP
      IPLocal.fromString(parametrosVariaveis[I_IP].c_str());                        // armazena IP baseado na string do parametro IP
      gatewayLocal.fromString(parametrosVariaveis[I_GATEWAY].c_str());              // armazena gateway baseado na string do parametro gateway
      if(!WiFi.config(IPLocal, gatewayLocal, IPAddress(255, 255, 0, 0))) continue; // se falhar na configuracao, pula a parte do loop
    }
    #ifdef DEBUG
    Serial.print("rede testada por initWiFi:");
    for(size_t i = 0; i < QTD_ARQUIVOS; i ++) {
      Serial.print(' '); Serial.print(parametrosVariaveis[i]);
    } 
    Serial.println(" encerra detalhes da rede testada por initWiFi:");
    #endif
    WiFi.begin(parametrosVariaveis[I_SSID].c_str(), parametrosVariaveis[I_PASS].c_str()); // inicia o WiFi
    delay(TEMPO_INICIO_WIFI);                                                             // aguarda o tempo estimado
    if(controleWiFi = (WiFi.status() == WL_CONNECTED)) break;                           // se conectar, sai do loop
  }

  for(size_t i = 0; i < QTD_ARQUIVOS; i ++) // para todos os arquivos
    file[i].close();                       // fecha o arquivo
  #ifdef DEBUG
  if(controleWiFi) {
    Serial.print("rede conectada por initWiFi:");
    for(size_t i = 0; i < QTD_ARQUIVOS; i ++) {
      Serial.print(' '); Serial.print(parametrosVariaveis[i]);
    }
    Serial.print(" IP real: "); Serial.print(WiFi.localIP());
    Serial.print(" Gateway real: "); Serial.print(WiFi.gatewayIP());
    Serial.println(" encerra detalhes da rede conectada por initWiFi:");
  } else
    Serial.println("Nao conectou :(");
  #endif
  if(controleWiFi and IPouGatewayVazios) // se tiver conectado e o IP ou o gateway da rede nao tiverem sido configurados
    deveIniciarAPSTA = true;             // registra que deve abrir APSTA para verificar o IP
  return controleWiFi;                   // retorna o estado da conexao
}

void setup() {
  #ifdef DEBUG
  Serial.begin(9600);
  #endif

  pinMode(PIN_IN, INPUT);      // inicia a entrada
  pinMode(PIN_OUT, OUTPUT);    // e a saída
  digitalWrite(PIN_OUT, HIGH); // digitais

  deveIniciarAPSTA = evtConectado = srvRestart = apagarCredencial = false; // inicializa variaveis de controle

  LittleFS.begin(); // inicia o sistema de arquivos

  ws.onEvent(onWsEvent);  // indica qual função deve ser chamada ao perceber um evento
  server.addHandler(&ws); // indica que o servidor será tratado de acordo com o WebSocket

  constroiPag(server, LittleFS); // funcao para construir as paginas Web
  
  server.on("/Cadastra", HTTP_POST, [](AsyncWebServerRequest *request) { // quando receber solicitação post com esta url
    uint8_t params = request->params();                                  // registra a quantidade de parametros
    #ifdef DEBUG
      Serial.print("qtd de parametros: "); Serial.println(params);
      imprimeTodosArquivos("appendFile1");
    #endif
    for(uint8_t i = 0; i < params; i ++) {         // para cada parametro
      const AsyncWebParameter* p = request->getParam(i); // registra o parametro
      #ifdef DEBUG
        Serial.print("parametro cadastrado: ");
        Serial.print(p->name()); Serial.print(' ');
      #endif
      for(size_t j = 0; j < QTD_ARQUIVOS; j ++) { // para todos os arquivos
        if(p->name() == PARAM_VEC[j]) {           // se o parametro for referente ao arquivo
          #ifdef DEBUG
            Serial.println(p->value());
          #endif
          parametrosVariaveis[j] = p->value();                            // registra o parametro no vetor global de parametros
          appendFile(LittleFS, PATHS[j], parametrosVariaveis[j].c_str()); // adiciona o parametro ao respectivo arquivo
        }
      }
    }
    #ifdef DEBUG
      imprimeTodosArquivos("appendFile2");
      Serial.println("/-----/");
    #endif
    request->redirect("/WM"); // redireciona para o gerenciador
    srvRestart = true;        // registra que o chip deve reiniciar
  });

  server.on("/Apaga", HTTP_POST, [](AsyncWebServerRequest *request) {              // quando receber solicitacao post com esta url
    if(const AsyncWebParameter* p = request->getParam(0); p->name() == PARAM_INPUT_CRED) { // se o parametro for o de exclusao de rede
      selCred = p->value();                                                        // armazena o parametro
      #ifdef DEBUG
        Serial.print("Rede deletada: "); Serial.println(selCred);
      #endif
      apagarCredencial = true; // registra que deverá apagar o parametro
    }
    request->redirect("/WM"); // redireciona para o gerenciador
    srvRestart = true;        // registra que o chip deve reiniciar
  });

  server.on("/dAP", HTTP_GET, [](AsyncWebServerRequest *request) { // quando receber solicitacao get com esta url
    WiFi.mode(WIFI_STA);                                           // configura para STA
    WiFi.softAPdisconnect(true);                                   // desconecta o AP
    #ifdef DEBUG
      Serial.println("AP desligada");
    #endif
    request->send(200); // responde com bem sucedido
  });

  if((!initWiFi()) or deveIniciarAPSTA) {                // se não conseguir se conectar a uma rede ou se conectar sem configurar o IP
    WiFi.mode(deveIniciarAPSTA ? WIFI_AP_STA : WIFI_AP); // configura o wifi de acordo com o caso
    WiFi.softAP(NOME_AP, SENHA_AP);                // inicia o AP
    #ifdef DEBUG
      Serial.print("modo: "); Serial.println(WiFi.getMode());
    #endif
  }
  
  server.begin(); // inicia o servidor
  initWiFi();

  #ifdef DEBUG
    Serial.flush();
    Serial.end();
  #endif
  pinMode(PIN_RX, INPUT_PULLUP);  // configura o pino Rx como input com pullup
  if(!digitalRead(PIN_RX)) {      // se o pino estiver conectado ao GND...
    resetaCredenciais(LittleFS); // reseta as credenciais
    ESP.restart();               // reinicia o ESP
  }

  Serial.begin(9600); // inicia a serial
}

void loop() {

  static bool controleAutoRec = false; // declara variavel de controle local
  static unsigned tempoDecorrido = 0;  // declara variavel de tempo local

  if(srvRestart and !apagarCredencial) ESP.restart(); // se for para o sistema reiniciar e ja tiver apagado a credencial, reinicia

  if(apagarCredencial) {                        // se for pra apagar alguma credencial...
    apagaCredencial(LittleFS, selCred.c_str()); // apaga a credencial
    apagarCredencial = false;                   // registra que apagou
  }

  if(evtConectado and digitalRead(PIN_IN)) { // se houve conexão à página
    digitalWrite(PIN_OUT, HIGH);             // envia um sinal digital de nível alto
    evtConectado = false;                   // reseta a variavel de controle
  }

  if(((WiFi.getMode() == WIFI_STA or WiFi.getMode() == WIFI_AP_STA) and WiFi.status() == WL_CONNECTED) and controleAutoRec == false)  { // se estiver conectado ao WiFi pela primeira vez neste loop
    WiFi.setAutoReconnect(true);  // ativa a autoreconexão
    WiFi.persistent(true);        // ativa a persistência
    controleAutoRec = true;       // indica que a robustez de WiFi já foi configurada após a reconexão
    #ifdef DEBUG_LOOP
      Serial.println("primeira conexao desde que desconectou");
    #endif
    if(deveIniciarAPSTA) {                  // se for necessario iniciar o AP para verificar o IP
      WiFi.mode(WIFI_AP_STA);               // configura para APSTA
      WiFi.softAP(NOME_AP, SENHA_AP); // inicia o AP
      #ifdef DEBUG_LOOP
        Serial.println("abre AP para verificar IP");
      #endif
    }
  }

  if(controleAutoRec == true and ((WiFi.getMode() != WIFI_STA and WiFi.getMode() != WIFI_AP_STA) or WiFi.status() != WL_CONNECTED)) {  // se estiver desconectado ou em modo diferente de STA
    controleAutoRec = false;      // indica que houve a desconexão
    #ifdef DEBUG_LOOP
      Serial.println("desconectou :c");
    #endif
  }

  if(millis() - tempoDecorrido >= INTERVALO_WIFI) { // a cada INTERVALO_WIFI ms
    tempoDecorrido = millis();                     // atualiza o tempo
    #ifdef DEBUG_LOOP
      Serial.println("confere conexao");
    #endif
    if(WiFi.status() != WL_CONNECTED and WiFi.getMode() == WIFI_STA) { // se estiver desconectado e em modo STA
      WiFi.reconnect();                                                // tenta reconectar
      #ifdef DEBUG_LOOP
        Serial.println("tentando reconectar a(crase) STA");
      #endif
    }
  }
}
