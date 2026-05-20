#include <WiFi.h> // Inclusão da biblioteca WiFi para gerenciar a conexão Wi-Fi
#include <WebServer.h> // Inclusão da biblioteca WebServer para criar um servidor web
//teste html
Webserver server(80);
//o wifi que sera conectado
const char* ssid = "Moto bomba";
const char* senha = "4ca142c8";
//parte do html
void handleRoot(){
    server.send(200,"text/html","""
        <!DOCTYPE html>
        <html lang="pt-br">
        <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>esp 32 seguidor: nome</title>
        </head>
        <body>
        <style>
            body{
                background-color: black;
                display: flex;
                justify-content: center;
                align-items: center;
            }
        </style>
        <button onclick="iniciar()" style="width: 30%;height: 40%;">iniciar</button>
        </body>
        </html>
        """)
}

void setup(){
    serial.begin(115200);
    WiFi.begin(ssid, senha); // Tenta conectar-se à rede Wi-Fi especificada com a senha
    
    while (WiFi.status() != WL_CONNECTED) { // Enquanto não estiver conectado ao Wi-Fi
        delay(1000); // Aguarda 1 segundo
        Serial.print("."); // Imprime um ponto para indicar tentativas de conexão
    }
}

void loop(){
    server.handleClient();
}