#include <Wire.h>
#include "RTClib.h"
#include <FAB_LED.h>
#define   M_PI   3.14159265358979323846 /* pi */
#define   M_E   2.7182818284590452354
#define   white_pin  10
#define   blue_pin  11 
RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};



char cmd[100];
int cmdIndex;



//Variáveis para controle da fita
// Declare the LED protocol and the port
sk6812b<D, 6>  strip;
// How many pixels to control
const uint8_t numPixels = 30;
grbw  pixels[numPixels] = {};


byte red = 0;
byte green = 0;
byte blue = 0;
byte white = 0; //Esse valor será compartilhado com a fita
byte Mblue = 0; //Esse será apenas dos LEDs de 3W

//Variáveis Millis
unsigned long previousMillis = 0;
const long interval = 1000;

//Variáveis de controle
byte start_time = 16;
byte duration = 4;
byte mode = 2;
String halo;
byte rgb_start = 12;
byte rgb_end = 21;

//Variáveis para as funções matemáticas
byte rf = 1;
byte gr = 1;
byte bf = 1;
byte stored_minute = 0;
byte stored_hour = 0;
byte max_rgb = 100;
byte max_white = 200;


void updateColors(char r, char g, char b, char w)
{
  for (int i = 0; i < numPixels; i++)
  {
    pixels[i].r = r;
    pixels[i].g = g;
    pixels[i].b = b;
    pixels[i].w = w;
  }
}

void exeCmd() {
  if (strcmp(cmd, "MRGB") == 0) {mode = 1;Serial.print("log M.RGB\n");}
  if (strcmp(cmd, "AUTO") == 0) {mode = 2;Serial.print("log F.Auto\n");}
  if (strcmp(cmd, "MANUAL") == 0){mode = 3;Serial.print("log F.Manual\n");}
  if (strcmp(cmd, "CONFIG") == 0){
    mode = 4;
    Serial.print("debug Config mode\n");
    strip.clear(2 * numPixels);
    analogWrite(blue_pin,0);
    analogWrite(white_pin,0);    
    }
  if (mode == 1 || mode == 3) { //Se estiver no modo Manual RGB ou  Full Manual
    if ( (cmd[0] == 'r' || cmd[0] == 'g' || cmd[0] == 'b' || cmd[0] == 'w' || cmd[0] == 'B') && cmd[1] == ' ' ) {
      // "r", "g", "b" , "w" are the ids for red, green, blue and white leds
      int val = 0;
      for (int i = 2; cmd[i] != 0; i++) {
        val = val * 10 + (cmd[i] - '0');
      }
      // if cmd is "r 100", val will be 100
      if (cmd[0] == 'r') red = val;
      else if (cmd[0] == 'g') green = val;
      else if (cmd[0] == 'b') blue = val;
      else if (cmd[0] == 'w') white = val;
      else if (cmd[0] == 'B') Mblue = val;

    }
     
      if (mode == 3){ //Apenas atualize esses valores se estiver no Full Manual
        analogWrite(blue_pin,Mblue);
        analogWrite(white_pin,white);
      }
    // Write the pixel array white
    updateColors(red, green, blue, white);
    // Display the pixels on the LED strip

    strip.sendPixels(numPixels, pixels);

  }

  if (mode == 4){ //Se estiver no modo de configuração
    
    //Se o comando vier de um dos sliders, cmd será: O(On time)|D(duration)|MW(Max white)|MY(max rgb)
    if ((cmd[0] == 'O' || cmd[0] == 'D' || cmd[0] == 'M' || cmd[0] == 'R') && (cmd[1] == ' ' || cmd[2] == ' ' )){ 
      int val = 0;
      if(cmd[1] == ' '){ //Se o comando tiver apenas um caractere(O,D)
        for (int i = 2; cmd[i] != 0; i++) {
          val = val * 10 + (cmd[i] - '0');
        }
        if(cmd[0] == 'O'){
          start_time = val;
          halo = (String) "debug "+ "Start: " + start_time + "\n";
          Serial.print(halo);
        }
        else if(cmd[0] == 'D'){
          duration = val; 
          halo = (String) "debug "+ "duration: " + duration + "\n";
          Serial.print(halo);               
        }
      }
      else { //Senão, terá 2 caracteres (MW,MY)
        for (int i = 3; cmd[i] != 0; i++) {
          val = val * 10 + (cmd[i] - '0');
        }
        if(cmd[1] == 'W'){       
          max_white = val;
          halo = (String) "debug "+ "MaxW: " + max_white + "\n";
          Serial.print(halo);            
        }
        else if(cmd[1] == 'Y'){
          max_rgb = val;
          halo = (String) "debug "+ "MaxRGB: " + max_rgb + "\n";
          Serial.print(halo);           
        }
        else if(cmd[1] == 'S'){
          rgb_start = val;
          halo = (String) "debug "+ "StartRGB: " + rgb_start + "\n";
          Serial.print(halo);           
        }
        else if(cmd[1] == 'E'){
          rgb_end = val;
          halo = (String) "debug "+ "EndRGB: " + rgb_end + "\n";
          Serial.print(halo);           
        }
      }
    
    }
  }

}


void setup() {
  pinMode(blue_pin,OUTPUT);
  pinMode(white_pin,OUTPUT);
#ifndef ESP8266
  while (!Serial); // for Leonardo/Micro/Zero
#endif

  Serial.begin(9600); // Bluetooth default baud is 115200
  delay(500);  
  Serial.print("Inicio do Setup");
  strip.clear(2 * numPixels);
  delay(500);
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  Serial.println("Achei o RTC");
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
  cmdIndex = 0;
  Serial.println("Fim do SETUP");
}


void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    DateTime now = rtc.now(); //Puxa os dados do RTC

    int dia = now.day();
    if (dia == 165) { //Caso o dia seja 165, há um problema na comunicação com o RTC
      //APARGAR TODAS AS LUZES E LIGAR O AVISO
      strip.clear(2 * numPixels);
      analogWrite(blue_pin,0);
      analogWrite(white_pin,0);
      while (1) {
        delay(500);
        digitalWrite(13, HIGH);
        delay(500);
        digitalWrite(13, LOW);
        Serial.print("log ERROR\ndebug error \n");
      }
    }
    

    //Logging routine (Mostrar o dia e horário)
    halo = (String) "log " + daysOfTheWeek[now.dayOfTheWeek()] + " | " + now.day() + "/";
    //"id " + value + "\n" + "id " + value + "\n" and so on...
    halo = halo + now.month() + " | " + now.hour() + ":" + now.minute() + ":" + now.second() + "\n"; 
    Serial.print(halo);
    
    if(mode == 2 || mode == 1){

      byte hour = now.hour();
      byte minute = now.minute();
      byte second = now.second();
      
      
        long elapsed_minutes = ((hour-start_time)*60)+ minute;
        if(mode == 2){
           if(hour >= rgb_start && hour < rgb_end){ 
            if(stored_hour != hour){
             stored_hour = hour;
             rf = random(1,6);
             gr = random(1,6);
             bf = random(1,6);
            }
            float x = ((minute*60)+second)*M_PI/1800; 
            red = (max_rgb/2)*(1 + sin(rf*x));
            green = (max_rgb/2)*(1 + sin(rf*x + M_PI/2));
            blue = (max_rgb/2)*(1 + sin(rf*x + M_PI/4));
            for (int i = 0; i < numPixels; i++){
              pixels[i].r = red;
              pixels[i].g = green;
              pixels[i].b = blue;
            }
           }
           else{
            for (int i = 0; i < numPixels; i++){
              pixels[i].r = 0;
              pixels[i].g = 0;
              pixels[i].b = 0;
            }
           }
        }
        if(hour >= start_time && hour < (start_time+duration)){ 
        //Implementar a subida exponencial do branco!!!
        float up = pow((elapsed_minutes - duration*30),2);
        float down = (2*(pow(20,sqrt(duration*1.3))));
        float output_float = -up/down;
        
        halo =(String) "debug UP" + up + "\n" + "debug OUT" + output_float + "\n";
        output_float = pow(M_E,output_float);
        halo = halo + "debug END" + output_float + "\n";
        byte output = max_white*output_float;
        
        halo = halo + "debug H" + (elapsed_minutes - duration*30) + "\n";        
        Serial.print(halo);       
        for (int i = 0; i < numPixels; i++){
            pixels[i].w = output;
          }
        analogWrite(white_pin,output);
        analogWrite(blue_pin,output);
        
      }
      else{        
        updateColors(red, green, blue, 0);
        analogWrite(white_pin,0);
        analogWrite(blue_pin,0);
        }
    strip.sendPixels(numPixels, pixels);
    }
    
    
  
  }






  //Recebe os comandos do celular (DEVE FICAR FORA DO MILLIS)
  if (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n') {
      cmd[cmdIndex] = 0;
      exeCmd();  // execute the command
      cmdIndex = 0; // reset the cmdIndex
    }
    else {
      cmd[cmdIndex] = c;
      if (cmdIndex < 99) cmdIndex++;
    }
  }

}

