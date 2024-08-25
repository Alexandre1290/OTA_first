#include <SoftwareSerial.h>


#define P_16 0xA001

static unsigned short crc_tab16[256];  // массив для таблицы CRC
unsigned char writeData[19] = {0};     // массив для отправляемых данных


#define LEN_B   34
#define LEN_S   35

#define S_MODE      12  //0x60(3) - DRY, 0x40(0) - cool, 0x80(1) - heat, 0x20(2) - auto, 0xA0(5) - fan
#define S_POWER     11  //on/off  00 - off, 01 - on,    43 - on, 42 - off 
#define S_FAN_SPD   13  //Скорость 28 - min, 3C - mid, 50 - max, 66 - auto
//#define S_SET_TMP   12  //Установленная температура
//#define S_CUR_TMP   21  //Текущая температура

#define R_MODE      12  //0x60(3) - DRY, 0x40(0) - cool, 0x80(1) - heat, 0x20(2) - auto, 0xA0(5) - fan

#define R_POWER     11  //on/off  00 - off, 01 - on,    43 - on, 42 - off 
#define R_SET_TMP   12  //Установленная температура
#define R_FAN_SPD   13  //Скорость 28 - min, 3C - mid, 50 - max, 66 - auto
#define R_CUR_TMP   21  //Текущая температура

const int LED = 2;

int power;
int cur_temp;
int cur_temp_prev;
int set_temp;
int set_temp_prev;
int fan_speed;
int fan_speed_prev;
int Mode;
int Mode_prev;

int power1;
int set_temp1;
int fan_speed1;
int Mode1;
int temp_data;
int flow1; 
int powerfull1;
int power1_prev;
int Mode1_prev;
int flow1_prev;
int set_temp1_prev;
int fan_speed1_prev;
int powerfull1_prev;

long prev = 0;
byte inCheck = 0;
int ii = 0;
int i; 
char raw0[72];
String raw_str0;
char raw1[69];
String raw_str1;
String raw_str1_prev;
byte perio[] = {0xAA,0x20,0xAC,0x00,0x00,0x00,0x00,0x00,0x03,0x03,0x41,0x81,0x00,0xFF,0x03,0xFF,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0xCD,0x99};
byte len0 = 1;

byte data[34] = {}; //Массив данных от кондиционера 35 элементов
byte send_data[35] = {}; //Массив данных для кондиционера 36 элементов
byte hdl_control[26] = {}; //Массив управления кондиционера по запросу HDL
byte hdl_status[14] = {}; // массив запросв статуса кондиционера
byte hdl_responce[26] = {}; // массив ответа устройству управления HDL
byte temp[24] ={};
byte crc1;
byte len1 = 24;
//HDL crc
unsigned short crc_hdl, crc_lo, crc_hi;
unsigned short table[256];
unsigned short temp1, a;

byte crc2;
byte len2 = 34;

byte speed_prev = 0x00;
byte temp_prev = 0x00;
byte mode_prev = 0x00;
byte power_prev = 0x00;

//byte MBdata[11] = {}; //Массив данных

byte send_off[18] = {0x80, 0xBC, 0x81, 0x11, 0x82, 0x17, 0x83, 0x50, 0x84, 0x02, 0x85, 0xE1, 0x86, 0x00, 0x87, 0x80, 0x48, 0x76};
byte send_on[18] = {0x80, 0xBC, 0x81, 0x11, 0x82, 0x17, 0x83, 0x50, 0x84, 0x02, 0x85, 0xE1, 0x86, 0x00, 0x87, 0x81, 0x48, 0x77};

SoftwareSerial MBSerial;

void SendData(byte req[], size_t size){

  Serial.write(req, sizeof(req));
  
}

inline unsigned char toHex( char ch ){
   return ( ( ch >= 'A' ) ? ( ch - 'A' + 0xA ) : ( ch - '0' ) ) & 0x0F;
}

/* Подсчет CRC8 массива mas длиной Len */
byte calc_crc(const byte *data, byte len) {
  byte crc = 0x00;
  while (len--) {
    byte extract = *data++;
    for (byte tempI = 8; tempI; tempI--) {
      byte sum = (crc ^ extract) & 0x01;
      crc >>= 1;
      if (sum) {
        crc ^= 0x8C;
      }
      extract >>= 1;
    }
  }
  return crc;
}

byte getCRC(byte req[], size_t size){
  byte crc = 0;
  //Serial.println("");
  for (int i=1; i < size+1; i++){
      crc += req[i];
  //Serial.println("");
  //Serial.print(req[i], HEX);
  //Serial.print(" ");
  }
  //Serial.println("");
  return crc;
}

// инициализация таблицы для расчета CRC
  void init_crc16_tab(){
    
      unsigned int i, j;
      unsigned short crc, c;
 
      for ( i=0; i < 256; i++ ){
          crc = 0;
          c = i;
          
          for ( j = 0; j < 8; j++ ){
              if ( ( crc ^ c ) & 0x0001 ){
                  crc = ( crc >> 1 ) ^ P_16;
              }
                  else{
                      crc =   crc >> 1;
                  }
          c = c >> 1;
          }
       crc_tab16[ i ] = crc;
      }
  }

 // расчет CRC
  unsigned short update_crc_16( unsigned short crc, char c ){
    unsigned short tmp, short_c;
    short_c = 0x00ff & ( unsigned short ) c;
    tmp =  crc ^ short_c;
    crc = ( crc >> 8 ) ^ crc_tab16[ tmp & 0xff ];
    return crc;
  }
// HDL crc
unsigned short crc_16(byte req[], size_t reqLen){      
      unsigned short crc1 = 0x0000;                                         //  начальная crc 0x0000
         for (int i = 0; i < 256; ++i){
            temp1 = 0;
            a = (unsigned short)(i << 8);
            for (int j = 0; j < 8; ++j){
                 if (((temp1 ^ a) & 0x8000) != 0){
                     temp1 = (unsigned short)((temp1 << 1) ^ 0x1021);               // полином 0x1021
                 } else {
                   temp1 <<= 1;
                 }
                 a <<= 1;
            }
            table[i] = temp1;
         }
         for (int i = 0; i < reqLen; ++i){
             crc1 = (unsigned short)((crc1 << 8) ^ table[((crc1 >> 8) ^ (0xff & req[i]))]);
         }
         return crc1;
  }



void setup() {
  
  Serial.begin(9600);
  //pinMode(LED, OUTPUT);
  //digitalWrite(LED, LOW);//GPIO13
  //!!!19200
  MBSerial.begin(9600, SWSERIAL_8E1, 13, 5, false, 95); //myPort.begin(38400, SWSERIAL_8N1, MYPORT_RX, MYPORT_TX, false);
  delay(100) ;
  //MBSerial.println("Power_on");  

  init_crc16_tab( ); // инициализация таблицы для расчета CRC
  
  // устанавливаем физический адрес и активируем функцию авто подтверждения

}



void loop() {

// Читаем порт между контроллером и кондиционером, записываем в массив
  if(Serial.available() > 0){
    Serial.readBytes(data, 35); //35//37
// Разбираем массив состояния
    InsertData(data, 35);
    
    //clean port!
    while(Serial.available()){
      delay(2);
      Serial.read();
    } 
  }
// Read MBSerial port
// MBSerial.available() проверяет наличие данных в серийном порту

  if (MBSerial.available()){
    //!MBSerial.readBytes(MBdata,8);
    MBSerial.readBytes(hdl_control,32);  //hdl_control[32]

// Разбираем посылку hdl_control //MBdata 
    //InsertDataSend(MBdata,30);
    InsertDataSend(hdl_control,32);
     /*
      for (int i = 0; i < 10; i++) {
        //!Serial.println(MBdata[i], HEX);
      }
      */
      //clean port!
      while(MBSerial.available()){
        delay(2);
        MBSerial.read();
    } 
     
  }
    
//Посылаем в порт сигнал запроса состояния кондиционера
  long now = millis();
  if (now - prev > 5000) {
    prev = now;
    Serial.write(perio, sizeof(perio));
  }
}


void InsertDataSend(byte hdl_control[], size_t size){
  // если групповой адрес 0х50, то делаем дальнейший разбор
  if ((hdl_control[7] == 0x19) && (hdl_control[8] == 0x3A) && (hdl_control[9] == 0x02) && (hdl_control[10] == 0x04)){
  ////////power
  //#define S_POWER     11  //on/off  00 - off, 01 - on,    43 - on, 42 - off 
    
      if (hdl_control[19] != power_prev){
         if (hdl_control[19] == 0x00){ 
           send_data[11] = 0x42; //S_POWER
        }
        if (hdl_control[19] == 0x01){
           send_data[11] = 0x43;
        }
        //send_data[12] = Mode + set_tmp;
        //send_data[12] = mode_prev + temp_prev;//! при вкл/выкл ставит предыдущую температуру!!!
        //send_data[11] = power_prev;
        //send_data[13] = speed_prev;
        send_data[17] = 0x30;
        send_data[18] = 0x00;
        send_data[20] = 0x00;
        //!!!    
        if (mode_prev == 0x00) {
          mode_prev = 0x40;
        }
        if (temp_prev == 0x00) {
          temp_prev = 0x12;
        }
        send_data[12] = mode_prev + temp_prev;
      
        if (speed_prev == 0x00) {
          speed_prev = 0x28;
        }
        send_data[13] = speed_prev; 
      
        power_prev = send_data[11];  
      }
         
///////////set_temp
//// 
    /*
      if (MBdata[8] == 0x06 && MBdata[9] == 0x40) { set_temp = 16; temp_prev = 0x10;}
      if (MBdata[8] == 0x06 && MBdata[9] == 0xA4) { set_temp = 17; temp_prev = 0x11;}
      if (MBdata[8] == 0x07 && MBdata[9] == 0x08) { set_temp = 18; temp_prev = 0x12;}
      if (MBdata[8] == 0x07 && MBdata[9] == 0x6C) { set_temp = 19; temp_prev = 0x13;}
      if (MBdata[8] == 0x07 && MBdata[9] == 0xD0) { set_temp = 20; temp_prev = 0x14;}
      if (MBdata[8] == 0x0C && MBdata[9] == 0x1A) { set_temp = 21; temp_prev = 0x15;}
      if (MBdata[8] == 0x0C && MBdata[9] == 0x4C) { set_temp = 22; temp_prev = 0x16;}
      if (MBdata[8] == 0x0C && MBdata[9] == 0x7E) { set_temp = 23; temp_prev = 0x17;}
      if (MBdata[8] == 0x0C && MBdata[9] == 0xB0) { set_temp = 24; temp_prev = 0x18;}
      if (MBdata[8] == 0x0C && MBdata[9] == 0xE2) { set_temp = 25; temp_prev = 0x19;}
      if (MBdata[8] == 0x0D && MBdata[9] == 0x14) { set_temp = 26; temp_prev = 0x1A;}
      if (MBdata[8] == 0x0D && MBdata[9] == 0x46) { set_temp = 27; temp_prev = 0x1B;}
      if (MBdata[8] == 0x0D && MBdata[9] == 0x78) { set_temp = 28; temp_prev = 0x1C;}
      if (MBdata[8] == 0x0D && MBdata[9] == 0xAA) { set_temp = 29; temp_prev = 0x1D;}
      if (MBdata[8] == 0x0D && MBdata[9] == 0xDC) { set_temp = 30; temp_prev = 0x1E;}
      if (MBdata[8] == 0x0E && MBdata[9] == 0x0E) { set_temp = 31; temp_prev = 0x1F;}
      if (MBdata[8] == 0x0E && MBdata[9] == 0x40) { set_temp = 32; temp_prev = 0x20;}
      if (MBdata[8] == 0x0E && MBdata[9] == 0x72) { set_temp = 33; temp_prev = 0x21;}
      */
      if (hdl_control[22] != set_temp_prev){
        set_temp = int(hdl_control[22]);
        temp_prev = hdl_control[22];      
        //!!! Check result
        //!Serial.print("SetTemp = ");
        //!Serial.println(set_temp);
        if (set_temp >= 0 && set_temp <= 89){
          if (mode_prev == 0x00) {
            mode_prev = 0x40;
          }
          send_data[12] = mode_prev + temp_prev;//Mode + set_tmp;
          send_data[11] = power_prev;
          send_data[13] = speed_prev;
          send_data[17] = 0x30;
          send_data[18] = 0x00;
          send_data[20] = 0x00;
          set_temp_prev = set_temp;
        }
      }

  ////////mode
  //Operation mode 0-Cool 1-Heat 2-Auto 3-Dry 4-HAUX 5-Fan
      if (hdl_control[20] != mode_prev) {    
        if (hdl_control[20] == 0x00){ //cool
            //send_data[S_MODE] = 0x40;
            Mode = 0x40;
            send_data[S_FAN_SPD] = speed_prev;
        }
        if (hdl_control[20] == 0x01){ //heat
            //send_data[S_MODE] = 0x80;
            Mode = 0x80; 
            send_data[S_FAN_SPD] = speed_prev;
        }
        if (hdl_control[20] == 0x03){ //auto
          //send_data[S_MODE] = 0x20; 
          Mode = 0x20;
          send_data[S_FAN_SPD] = speed_prev;
        }
        if (hdl_control[20] == 0x04){ //dry
          //send_data[S_MODE] = 0x60;
          Mode = 0x60;
          send_data[S_FAN_SPD] = 0x65;
        }
        if (hdl_control[20] == 0x02){ //fan
          //send_data[S_MODE] = 0xA1;
          Mode = 0xA1;
          send_data[S_FAN_SPD] = speed_prev;
        }
     
        send_data[12] = Mode + temp_prev; //S_MODE
        send_data[11] = power_prev;
        send_data[13] = speed_prev;
        send_data[17] = 0x30;
        send_data[18] = 0x00;
        send_data[20] = 0x00;
        mode_prev = Mode;
      }

  //////////fanSpeed
  //// Fan Speed 0-Low 1-Med 2-High 3-Auto 4-Top 5-Very Lo
      if (hdl_control[21] != fan_speed_prev) {
        if (hdl_control[21] == 0x03){ //"min"){
            send_data[S_FAN_SPD] = 0x28; 
        }
        if (hdl_control[21] == 0x02){ //"mid"){
            send_data[S_FAN_SPD] = 0x3C;
        }
        if (hdl_control[21] == 0x01){ //"max"){
          send_data[13] = 0x50;  //S_FAN_SPD
        }
        if (hdl_control[21] == 0x00){ //"auto"){
            send_data[S_FAN_SPD] = 0x66; 
        }
        send_data[12] = Mode + temp_prev;
        send_data[11] = power_prev;
        send_data[17] = 0x30;
        send_data[18] = 0x00;
        send_data[20] = 0x00;
        speed_prev = send_data[13]; //S_FAN_SPD        
      }
           
  ////////flow
  //// Swing 0-Vertical 1-30 deg 2-45 deg 3-60 deg 4-Horizontal 5-Auto 6-OFF
    if (hdl_control[23] != flow_prev){
      if (hdl_control[23] == 0x00 || hdl_control[23] == 0x01 || hdl_control[23] == 0x11){ 
        //send_data[8] = 0x03;
        //send_data[9] = 0x02;
        send_data[17] = 0x30;
        //return;
      }
      if (hdl_control[23] == 0x10){
        //send_data[8] = 0x03;
        //send_data[9] = 0x02;
        send_data[17] = 0x3C;
        //return;
      }
      
      send_data[12] = mode_prev + temp_prev;
      send_data[11] = power_prev;
      send_data[13] = speed_prev;
      send_data[18] = 0x00;
      send_data[20] = 0x00;

      
    }
  //формируем и отправляем посылку кондиционеру
    send_data[0] = 0xAA;
    send_data[1] = 0x23;
    send_data[2] = 0xAC;
    send_data[3] = 0x00;
    send_data[4] = 0x00;
    send_data[5] = 0x00;
    send_data[6] = 0x00;
    send_data[7] = 0x00;
    send_data[8] = 0x03;
    send_data[9] = 0x02;
    send_data[10] = 0x40;
  //#define S_POWER     11  //on/off  00 - off, 01 - on,    43 - on, 42 - off 
  //#define S_MODE      12  //04 - DRY, 01 - cool, 02 - heat, 00 - smart 03 - вентиляция
  //#define S_FAN_SPD   13  //Скорость 28 - min, 3C - mid, 50 - max, 66 - auto
    send_data[14] = 0x7F;
    send_data[15] = 0x7F;
    send_data[16] = 0x00;
    //send_data[17] = 0x30;
    //send_data[18] = 0x00;
    send_data[19] = 0x00;
    //send_data[20] = 0x00;
    send_data[21] = 0x00;
    send_data[22] = 0x00;
    send_data[23] = 0x00;
    send_data[24] = 0x00;
    send_data[25] = 0x00; 
    send_data[26] = 0x00;
    send_data[27] = 0x00;
    send_data[28] = 0x00;
    send_data[29] = 0x00;
    send_data[30] = 0x00;
    send_data[31] = 0x00;
    send_data[32] = 0x00;
    send_data[33] = ii; //String(ii, HEX); //ii; //HEX(ii);
    
    for (int i = 0; i < 24; i++) {
      temp[i] = send_data[i + 10];
      }
    
    crc1 = calc_crc(temp, len1);
    
    send_data[34] = crc1; //String(crc1, HEX); //HEX(crc1);
    crc2 = (0x100 - getCRC(send_data, len2)) & 0xff;
    send_data[35] = crc2; //String(crc2, HEX); //HEX(crc2);
    
    Serial.write(send_data, sizeof(send_data));
    Serial.write(crc2);
  
    delay(500);
    
    if (ii <= 255) {
        ii = ii + 1;
    } else {
      ii = 0;
    }  
  }
}  
 


void InsertData(byte data[], size_t size){
  
/*
#define R_MODE      12  //0x60(3) - DRY, 0x40(0) - cool, 0x80(1) - heat, 0x20(2) - auto, 0xA0(5) - fan

#define R_POWER     11  //on/off  00 - off, 01 - on,    43 - on, 42 - off 
#define R_SET_TMP   12  //Установленная температура
#define R_FAN_SPD   13  //Скорость 28 - min, 3C - mid, 50 - max, 66 - auto
#define R_CUR_TMP   21  //Текущая температура
*/
// температура  
//// Set Temperature from 0x2000/2 = 16(decimal) to 0x4000/2 = 32(decimal)
    temp_data = data[R_MODE];
    if ((temp_data >= 33) & (temp_data <= 46)) {
      Mode1 = 32;
      set_temp1 = temp_data - 32 + 16;
    }
    if ((temp_data >= 65) & (temp_data <= 78)) {
      Mode1 = 64;
      set_temp1 = temp_data - 64 + 16;
    }
    if ((temp_data >= 97) & (temp_data <= 110)) {
      Mode1 = 96;
      set_temp1 = temp_data - 96 + 16;
    }
    if ((temp_data >= 129) & (temp_data <= 142)) {
      Mode1 = 128;
      set_temp1 = temp_data - 128 + 16;
    }
    //set_tmp1 = (byte(data[R_MODE]) && 0x00001111) + 0x00010000;
    cur_temp = data[R_CUR_TMP];
    //Mode1 = byte(data[R_MODE]) && 0x11110000;
    fan_speed1 = data[R_FAN_SPD];
    power1 = data[R_POWER];
    flow1 = data[17]; 
    //powerfull1 = data[20];

  /////////////////////////////////
  //// On/Off 0-OFF 1-ON
  if (power1 != power1_prev) {
    if (power1 == 0x43){
      //client.publish(mystring4.c_str(), "on", true);
      //HR_40001 = 0x01;
      //send_knx_18[15] = 0x81;
      hdl_responce[19] = 0x01;
    }
    if (power1 == 0x42){
      //client.publish(mystring4.c_str(), "off", true);
      //HR_40001 = 0x00;
      //send_knx_18[15] = 0x80;
      hdl_responce[19] = 0x00;
    }
    
    power1_prev = power1;
  }
  ///////////////////////////////////Скорость 28 - min, 3C - mid, 50 - max, 66 - auto  
  //// Fan Speed 0-Low 1-Med 2-High 3-Auto 4-Top 5-Very Lo
  if (fan_speed1 != fan_speed1_prev) {
    if (fan_speed1 == 0x50){
      //client.publish(mystring3.c_str(), "3", true); //"max");
      //HR_40003 = 0x02;
      //send_knx_20[17] = 0x02;
      hdl_responce[21] = 0x01;
    }
    if (fan_speed1 == 0x3C){
      //client.publish(mystring3.c_str(),  "2", true); //"mid");
      //HR_40003 = 0x01;
      //send_knx_20[17] = 0x01;
      hdl_responce[21] = 0x02;      
    }
    if (fan_speed1 == 0x28){
      //client.publish(mystring3.c_str(), "1", true); //"min");
      //HR_40003 = 0x00;
      //send_knx_20[17] = 0x00;
      hdl_responce[21] = 0x03;
    }
    if (fan_speed1 == 0x66){
      //client.publish(mystring3.c_str(), "0", true); //"auto");
      //HR_40003 = 0x03;
      //send_knx_20[17] = 0x03;
      hdl_responce[21] = 0x00;
    }
    fan_speed1_prev = fan_speed1;
  }
  /////////////////////////////////
  /*
  char b[5]; 
  String char_set_tmp = String(set_tmp1);
  char_set_tmp.toCharArray(b,5);
  */
  /*
      if (MBdata[8] == 0x06 && MBdata[9] == 0x40) { set_temp = 16; temp_prev = 0x10;}
      if (MBdata[8] == 0x06 && MBdata[9] == 0xA4) { set_temp = 17; temp_prev = 0x11;}
      if (MBdata[8] == 0x07 && MBdata[9] == 0x08) { set_temp = 18; temp_prev = 0x12;}
      if (MBdata[8] == 0x07 && MBdata[9] == 0x6C) { set_temp = 19; temp_prev = 0x13;}
      if (MBdata[8] == 0x07 && MBdata[9] == 0xD0) { set_temp = 20; temp_prev = 0x14;}
      if (MBdata[8] == 0x0C && MBdata[9] == 0x1A) { set_temp = 21; temp_prev = 0x15;}
      if (MBdata[8] == 0x0C && MBdata[9] == 0x4C) { set_temp = 22; temp_prev = 0x16;}
      if (MBdata[8] == 0x0C && MBdata[9] == 0x7E) { set_temp = 23; temp_prev = 0x17;}
      if (MBdata[8] == 0x0C && MBdata[9] == 0xB0) { set_temp = 24; temp_prev = 0x18;}
      if (MBdata[8] == 0x0C && MBdata[9] == 0xE2) { set_temp = 25; temp_prev = 0x19;}
      if (MBdata[8] == 0x0D && MBdata[9] == 0x14) { set_temp = 26; temp_prev = 0x1A;}
      if (MBdata[8] == 0x0D && MBdata[9] == 0x46) { set_temp = 27; temp_prev = 0x1B;}
      if (MBdata[8] == 0x0D && MBdata[9] == 0x78) { set_temp = 28; temp_prev = 0x1C;}
      if (MBdata[8] == 0x0D && MBdata[9] == 0xAA) { set_temp = 29; temp_prev = 0x1D;}
      if (MBdata[8] == 0x0D && MBdata[9] == 0xDC) { set_temp = 30; temp_prev = 0x1E;}
      if (MBdata[8] == 0x0E && MBdata[9] == 0x0E) { set_temp = 31; temp_prev = 0x1F;}
      if (MBdata[8] == 0x0E && MBdata[9] == 0x40) { set_temp = 32; temp_prev = 0x20;}
      if (MBdata[8] == 0x0E && MBdata[9] == 0x72) { set_temp = 33; temp_prev = 0x21;}
   */
  if (set_temp1 != set_temp1_prev) {
    //client.publish(mystring2.c_str(), b, true);
    //HR_40004 0  // Set Temperature from 0x2000/2 = 16(decimal) to 0x4000/2 = 32(decimal)
    
    if (set_temp1 == 17) {
      //client.publish(mystring7.c_str(), "17", true);
      //HR_40004 = 0x22;
      hdl_responce[22] = 0x11;
    }
    if (set_temp1 == 18) {
      //client.publish(mystring7.c_str(), "18", true);
      //HR_40004 = 0x24;
      hdl_responce[22] = 0x12;
    }
    if (set_temp1 == 19) {
      //client.publish(mystring7.c_str(), "18", true);
      //HR_40004 = 0x26;
      hdl_responce[22] = 0x13;
    }
    if (set_temp1 == 20) {
      //client.publish(mystring7.c_str(), "20", true);
      //HR_40004 = 0x28;
      hdl_responce[22] = 0x14;
    }
    if (set_temp1 == 21) {
      //client.publish(mystring7.c_str(), "21", true);
      //HR_40004 = 0x2A;
      hdl_responce[22] = 0x15;
    }
    if (set_temp1 == 22) {
      //client.publish(mystring7.c_str(), "22", true);
      //HR_40004 = 0x2C;
      hdl_responce[22] = 0x16;
    }
    if (set_temp1 == 23) {
      //client.publish(mystring7.c_str(), "23", true);
      //HR_40004 = 0x2E;
      hdl_responce[22] = 0x17;
    }
    if (set_temp1 == 24) {
      //client.publish(mystring7.c_str(), "24", true);
      //HR_40004 = 0x30;
      hdl_responce[22] = 0x18;
    }
    if (set_temp1 == 25) {
      //client.publish(mystring7.c_str(), "25", true);
      //HR_40004 = 0x32;
      hdl_responce[22] = 0x19;
    }
    if (set_temp1 == 26) {
      //client.publish(mystring7.c_str(), "26", true);
      //HR_40004 = 0x34;
      hdl_responce[22] = 0x1A;
    }
    if (set_temp1 == 27) {
      //client.publish(mystring7.c_str(), "27", true);
      //HR_40004 = 0x36;
      hdl_responce[22] = 0x1B;
    }
    if (set_temp1 == 28) {
      //client.publish(mystring7.c_str(), "28", true);
      //HR_40004 = 0x38;
      hdl_responce[22] = 0x1C;
    }
    if (set_temp1 == 29) {
      //client.publish(mystring7.c_str(), "29", true);
      //HR_40004 = 0x3A;
      shdl_responce[22] = 0x1D;
    }
    if (set_temp1 == 30) {
      //client.publish(mystring7.c_str(), "30", true);
      //HR_40004 = 0x3C;
      hdl_responce[22] = 0x1E;
    }
    if (set_temp1 == 31) {
      //client.publish(mystring7.c_str(), "31", true);
      //HR_40004 = 0x3C;
      hdl_responce[22] = 0x1F;
    }
    if (set_temp1 == 32) {
      //client.publish(mystring7.c_str(), "32", true);
      //HR_40004 = 0x3C;
      hdl_responce[22] = 0x20;
    }
    if (set_temp1 == 33) {
      //client.publish(mystring7.c_str(), "33", true);
      //HR_40004 = 0x3C;
      hdl_responce[22] = 0x21;
    }
    set_temp1_prev = set_temp1;
  }  
  ////////////////////////////////////
  //define HR_40007 0  // Room Temperature x10 (Read Only)
  if (cur_temp != cur_temp_prev) {
    
    // !!! определить температуры ниже 17 градусов
    
    if (cur_temp == 0x53) {
      //client.publish(mystring7.c_str(), "17", true);
      //HR_40007 = 0x22;
      hdl_responce[13] = 0x11;
    }
    if (cur_temp == 0x54) {
      //client.publish(mystring7.c_str(), "17", true);
      //HR_40007 = 0x22;
      hdl_responce[13] = 0x11;
    }
    if (cur_temp == 0x55) {
      //client.publish(mystring7.c_str(), "18", true);
      //HR_40007 = 0x24;
      hdl_responce[13] = 0x12;
    }
    if (cur_temp == 0x56) {
      //client.publish(mystring7.c_str(), "18", true);
      //HR_40007 = 0x24;
      hdl_responce[13] = 0x12;
    }
    if (cur_temp == 0x57) {
      //client.publish(mystring7.c_str(), "19", true);
      //HR_40007 = 0x26;
      hdl_responce[13] = 0x13;
    }
    if (cur_temp == 0x58) {
      //client.publish(mystring7.c_str(), "19", true);
      //HR_40007 = 0x26;
      hdl_responce[13] = 0x13;
    }
    if (cur_temp == 0x59) {
      //client.publish(mystring7.c_str(), "20", true);
      //HR_40007 = 0x28;
      hdl_responce[13] = 0x14;
    }
    if (cur_temp == 0x5A) {
      //client.publish(mystring7.c_str(), "20", true);
      //HR_40007 = 0x28;
      hdl_responce[13] = 0x14;
    }
    if (cur_temp == 0x5B) {
      //client.publish(mystring7.c_str(), "21", true);
      //HR_40007 = 0x2A;
      hdl_responce[13] = 0x15;
    }
    if (cur_temp == 0x5C) {
      //client.publish(mystring7.c_str(), "21", true);
      //HR_40007 = 0x2A;
      hdl_responce[13] = 0x15;
    }
    if (cur_temp == 0x5D) {
      //client.publish(mystring7.c_str(), "22", true);
      //HR_40007 = 0x2C;
      hdl_responce[13] = 0x16;
    }
    if (cur_temp == 0x5E) {
      //client.publish(mystring7.c_str(), "22", true);
      //HR_40007 = 0x2C;
      hdl_responce[13] = 0x16;
    }
    if (cur_temp == 0x5F) {
      //client.publish(mystring7.c_str(), "23", true);
      //HR_40007 = 0x2E;
      hdl_responce[13] = 0x17;
    }
    if (cur_temp == 0x61) {
      //client.publish(mystring7.c_str(), "23", true);
      //HR_40007 = 0x2E;
      hdl_responce[13] = 0x17;
    }
    if (cur_temp == 0x62) {
      //client.publish(mystring7.c_str(), "24", true);
      //HR_40007 = 0x30;
      hdl_responce[13] = 0x18;
    }
    if (cur_temp == 0x63) {
      //client.publish(mystring7.c_str(), "24", true);
      //HR_40007 = 0x30;
      hdl_responce[13] = 0x18;
    }
    if (cur_temp == 0x64) {
      //client.publish(mystring7.c_str(), "25", true);
      //HR_40007 = 0x32;
      hdl_responce[13] = 0x19;
    }
    if (cur_temp == 0x65) {
      //client.publish(mystring7.c_str(), "25", true);
      //HR_40007 = 0x32;
      hdl_responce[13] = 0x19;
    }
    if (cur_temp == 0x66) {
      //client.publish(mystring7.c_str(), "26", true);
      //HR_40007 = 0x34;
      hdl_responce[13] = 0x1A;
    }
    if (cur_temp == 0x67) {
      //client.publish(mystring7.c_str(), "26", true);
      //HR_40007 = 0x34;
      hdl_responce[13] = 0x1A;
    }
    if (cur_temp == 0x68) {
      //client.publish(mystring7.c_str(), "27", true);
      //HR_40007 = 0x36;
      hdl_responce[13] = 0x1B;
    }
    if (cur_temp == 0x69) {
      //client.publish(mystring7.c_str(), "27", true);
      //HR_40007 = 0x36;
      hdl_responce[13] = 0x1B;
    }
    if (cur_temp == 0x6A) {
      //client.publish(mystring7.c_str(), "28", true);
      //HR_40007 = 0x38;
      hdl_responce[13] = 0x1C;
    }
    if (cur_temp == 0x6B) {
      //client.publish(mystring7.c_str(), "28", true);
      //HR_40007 = 0x38;
      hdl_responce[13] = 0x1C;
    }
    if (cur_temp == 0x6C) {
      //client.publish(mystring7.c_str(), "29", true);
      //HR_40007 = 0x3A;
      hdl_responce[13] = 0x1D;
    }
    if (cur_temp == 0x6D) {
      //client.publish(mystring7.c_str(), "29", true);
      //HR_40007 = 0x3A;
      hdl_responce[13] = 0x1D;
    }
    if (cur_temp == 0x6E) {
      //client.publish(mystring7.c_str(), "30", true);
      //HR_40007 = 0x3C;
      hdl_responce[13] = 0x1E;
    }
    if (cur_temp == 0x6F) {
      //client.publish(mystring7.c_str(), "30", true);
      //HR_40007 = 0x3C;
      hdl_responce[13] = 0x1E;
    }
    //!!! продолжить температуры далее
    
    cur_temp_prev = cur_temp;
  } 
  //String char_cur_tmp = String(cur_tmp);
  //char_cur_tmp.toCharArray(b,5);
  //client.publish("myhome/Conditioner/Current_Temp", b);
  ////////////////////////////////////
  //// Operation mode 0-Cool 1-Heat 2-Auto 3-Dry 4-HAUX 5-Fan
  if (Mode1 != Mode1_prev) {
    if (Mode1 == 0x20){
      //client.publish(mystring1.c_str(), "auto", true);
      //HR_40002 = 0x02;
      hdl_responce[20] = 0x03;
    }
    if (Mode1 == 0x40){
      //client.publish(mystring1.c_str(), "cool", true);
      //HR_40002 = 0x00;
      hdl_responce[20] = 0x00;
    }
    if (Mode1 == 0x80){
      //client.publish(mystring1.c_str(), "heat", true);
      //HR_40002 = 0x01;
      hdl_responce[20] = 0x01;
    }
    if (Mode1 == 0xA1){
      //client.publish(mystring1.c_str(), "vent", true);
      //HR_40002 = 0x05;
      hdl_responce[20] = 0x02;
    }
    if (Mode1 == 0x60){
      //client.publish(mystring1.c_str(), "dry", true);
      //HR_40002 = 0x03;
      hdl_responce[20] = 0x04;
    }
    
  Mode1_prev = Mode1;
  }

  //////
  //// Swing 0-Vertical 1-30 deg 2-45 deg 3-60 deg 4-Horizontal 5-Auto 6-OFF 
  if (flow1 != flow1_prev) {
    if (flow1 == 0x00){
      //client.publish(mystring9.c_str(), "off", true);
      //HR_40005 = 0x06;
      hdl_responce[22] = 0x00;
    } 
    if (flow1 == 0x3C){
      //client.publish(mystring9.c_str(), "vert_on", true);
      //HR_40005 = 0x00;
      hdl_responce[22] = 0x01;
    } 
    if (flow1 == 0x33){
      //client.publish(mystring9.c_str(), "horizont_on", true);
      //HR_40005 = 0x04;
      //send_knx_20[17] = 0x04;
    } 
  
    flow1_prev = flow1;
  }
// формируем отклик и пишем его в программный порт MBSerial
    hdl_responce[0] = 0xAA;
    hdl_responce[1] = 0xAA;
    hdl_responce[2] = 0x18; //Size of Data Packet
    hdl_responce[3] = 0x02; //Subnet ID
    hdl_responce[4] = 0x04; //Device ID
    hdl_responce[5] = 0x03; //Device type
    hdl_responce[6] = 0xB7; //Device type
    hdl_responce[7] = 0x19; //Command
    hdl_responce[8] = 0x3B; //Command
    hdl_responce[9] = 0x02; //Subnet ID
    hdl_responce[10] = 0x02;  //Device ID
    hdl_responce[11] = 0x01;  //ACNo
    hdl_responce[12] = 0x00;  //Temp oC
    //hdl_responce[13] = 0xAA;  //Current Temp
    hdl_responce[14] = hdl_control[14];  //0x14;  //Cooling Temp Point
    hdl_responce[15] = hdl_control[15]; //0x14;  //Heating Temp Point
    hdl_responce[16] = hdl_control[16]; //0x0F;  //Auto Temp Point
    hdl_responce[17] = hdl_control[17]; //0x0F;  //Dry Temp Point
    hdl_responce[18] = hdl_control[18]; //0x01;  //Mode and Fan
    hdl_responce[24] = 0x00;      
    hdl_responce[25] = 0x00;
    //hdl_responce[19] = 0xAA;  //Power
    //hdl_responce[20] = 0xAA;  //Mode
    //hdl_responce[21] = 0xAA;  //Fan Speed
    //hdl_responce[22] = 0xAA;  //Set Temp
    //hdl_responce[23] = 0xAA;  //Flow
    crc_hdl = crc_16(hdl_responce, sizeof(hdl_responce));
    // добавление CRC к массиву данных
    crc_lo = crc_hdl & 0xFF;
    crc_hi = (crc_hdl >> 8) & 0xFF;
    hdl_responce[24] = crc_hi;      //SRC
    hdl_responce[25] = crc_lo;      //SRC     


// просто пишем в порт!!!
    MBSerial.write(hdl_responce, sizeof(hdl_responce));

}
