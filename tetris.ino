#include <FastLED.h>
#include<Wire.h>
#define NUM_LEDS 200
#define DATA_PIN 6

#define R_BUTTON 9
#define L_BUTTON 8

CRGB leds[NUM_LEDS];

struct Vector2 {
  int x;
  int y;
};

struct Block {
   Vector2 point[3];
};

struct State {
  byte x;
  byte y;
  byte type;
  Block block;
};

byte board[25][12];
byte displayBoard[25][12];

State current;

float rotateCount=0;

Block block[8] {
  {{{0, 0}, {0, 0}, { 0, 0}}},
  {{{0, -1}, {0, 1}, { 0, 1}}},
  {{{0, -1}, {0, 1}, { 1, 1}}},
  {{{0, -1}, {0, 1}, { 1, 1}}},
  {{{0, -1}, {1, 0}, { 1, 1}}},
  {{{0, -1}, { -1, 0}, { -1, 1}}},
  {{{0, 1}, {1, 0}, { 1, 1}}},
  {{{0, -1}, {1, 0}, { -1, 0}}}
};

void setup() {
  pinMode(R_BUTTON,INPUT);
  pinMode(L_BUTTON,INPUT);
  Serial.begin(9600);
  FastLED.setBrightness(20); 
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  SensorSetup();
  for (int y = 0; y < 25; y++) {
    for (int x = 0; x < 12; x++) {
      if (x == 0 || x == 11 || y == 0) {
        board[y][x] = 255;
        displayBoard[y][x] = 255;
      } else {
        board[y][x] = 0;
        displayBoard[y][x] = 0;
      }
    }
  }
  WriteDisplay();

  current.x = 5;
  current.y = 21;
  
  current.type = (byte)random(1, 7);
  current.block = block[current.type];
}

void SensorSetup(){
  Wire.begin();
  Wire.beginTransmission(0x53);  
    Wire.write(0x31);
    Wire.write(0x0B);  
  Wire.endTransmission();
  Wire.beginTransmission(0x53);  
    Wire.write(0x2d);
    Wire.write(0x08);  
  Wire.endTransmission();
}


Vector2 under = {0, -1};
Vector2 right = {1,0};
Vector2 left = {-1,0};

void loop() {
    if (CheckMoveable(under))Move(under);
    else Next();
    
    WriteDisplay();

    int time=0;
    int interval=200;
    while(time<500){
      SensorUpdate();
      if(CheckRightRotateable() && digitalRead(R_BUTTON) && interval<=0){
        Rotate();
        interval=200;
      }
      if(CheckLeftRotateable() &&digitalRead(L_BUTTON) && interval<=0){
      LeftRotate();
      interval=200;
      }
    WriteDisplay();
      delay(50);
      time+=50;
      interval-=50;
    }
    if(rotateCount>=10){
      Serial.print("right\n");
        if (CheckMoveable(right))Move(right);
        rotateCount=0;
      }else if(rotateCount<=-10){
      Serial.print("left\n");
        if(CheckMoveable(left))Move(left);
        rotateCount=0;
      }
}

void SensorUpdate(){
  
  byte RegTbl[6]; 
  // XYZの先頭アドレスに移動する
  Wire.beginTransmission(0x53);
  Wire.write(0x32);
  Wire.endTransmission();
  
  // デバイスへ6byteのレジスタデータを要求する
  Wire.requestFrom(0x53, 6);
   
  // 6byteのデータを取得する
  byte i;
  for (i=0; i< 6; i++){
    while (Wire.available() == 0 ){}
    RegTbl[i] = Wire.read();
  }
 
  // データを各XYZの値に変換する(LSB単位)
  int16_t x = (((int16_t)RegTbl[1]) << 8) | RegTbl[0];
  int16_t y = (((int16_t)RegTbl[3]) << 8) | RegTbl[2];
  int16_t z = (((int16_t)RegTbl[5]) << 8) | RegTbl[4];  
 /*
  // 各XYZ軸の加速度(m/s^2)を出力する
  Serial.print("X : ");
  Serial.print( x * 0.0392266 );
  Serial.print(" Y : ");
  Serial.print( y * 0.0392266 );
  Serial.print(" Z : ");
  Serial.print( z * 0.0392266 );
  Serial.println(" m/s^2");
 */
 float p=x* 0.0392266;
 if(p<=1 && -1<=p)p=0;
  rotateCount+=(p);
  
}


void Rotate() {
  Serial.print("rightrotate");
  SetDisplayBoardState(0);
  for (byte i = 0; i < 3; i++)
  {
    int buff = current.block.point[i].y;
    current.block.point[i].y = -current.block.point[i].x;
    current.block.point[i].x = buff;
  }
  SetDisplayBoardState(current.type);
}

void LeftRotate(){
  Serial.print("leftrotate");
  SetDisplayBoardState(0);
  for (byte i = 0; i < 3; i++)
  {
    int buff = current.block.point[i].y;
    current.block.point[i].y = current.block.point[i].x;
    current.block.point[i].x = -buff;
  }
  SetDisplayBoardState(current.type);
}

void Move(Vector2 point) {
  SetDisplayBoardState(0);
  current.y += point.y;
  current.x += point.x;
  SetDisplayBoardState(current.type);
}

void SetDisplayBoardState(byte s) {
  displayBoard[current.y][current.x] = s;
  for (byte i = 0; i < 3; i++)
  {
    int pointY = current.y + current.block.point[i].y;
    int pointX = current.x + current.block.point[i].x;
    displayBoard[pointY][pointX] = s;
  }
}

void SetBoardState(byte s) {
  board[current.y][current.x] = s;
  for (byte i = 0; i < 3; i++)
  {
    int pointY = current.y + current.block.point[i].y;
    int pointX = current.x + current.block.point[i].x;
    board[pointY][pointX] = s;
  }
}

void Next() {
  SetBoardState(current.type);
  if (current.y == 21) GameOver();
  else
  {
    current.x = 5;
    current.y = 21;
    current.type = (byte)random(1, 8);
    current.block = block[current.type];
  }
  LineProcess();
}

void LineProcess() {
  for (byte y = 1; y < 21; y++)
  {
    for (byte x = 1; x < 11; x++)
    {
      if (board[y][x] == 0) break;
      if (x == 10) RemoveLine(y);
    }
  }
  WriteDisplay();
}

void RemoveLine(int line) {
  for (byte x = 1; x < 11; x++)
  {
    board[line][x] = 0;
    displayBoard[line][x] = 0;
  }

  for (byte y = line + 1; y < 21; y++)
  {
    for (byte x = 1; x < 11; x++)
    {
      board[y - 1][x] = board[y][x];
      displayBoard[y - 1][x] = displayBoard[y][x];
    }
  }
  LineProcess();
}

void GameOver() {

}

bool CheckMoveable(Vector2 diff) {
  if (board[(int)current.y + diff.y][(int)current.x + diff.x] != 0)return false;
  for (int i = 0; i < 3; i++) {
    int pointY = current.y + diff.y + current.block.point[i].y;
    int pointX = current.x + diff.x + current.block.point[i].x;
    if (board[pointY][pointX] != 0)return false;
  }
  return true;
}

bool CheckRightRotateable(){
  for (int i = 0; i < 3; i++)
  {
    if(board[-current.block.point[i].x+current.y][current.block.point[i].y+current.x]!=0){
      return false;
    }
  }
  return true;
}

bool CheckLeftRotateable(){
  for (byte i = 0; i < 3; i++)
  {
    if(board[current.block.point[i].x+current.y][-current.block.point[i].y+current.x]!=0)return false;
  }
  return true;
}


void WriteDisplay(){


  CRGB color[8]{
    CRGB::Black,
    CRGB::Aqua, //tetris
    CRGB::Orange, //L
    CRGB::Blue, //J
    CRGB::Red, //S
    CRGB::Green, //Z
    CRGB::Yellow, //square
    CRGB::Purple, //T
  };

  for(byte x=1;x<11;x++){
    for(byte y=1;y<21;y++){
      byte pos;
      if(x%2==0) pos = 20*x-y;
      else pos = (x-1)*20+(y-1);
      
      leds[pos] = color[displayBoard[y][x]];
    }
  }
    FastLED.show();
}
