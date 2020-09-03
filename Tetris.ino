//Copyright (C) 2013 Valentin Ivanov http://www.breakcontinue.com.
//
//This work is licensed under a Creative Commons 
//Attribution-NonCommercial-ShareAlike 3.0 Unported License.
//See http://creativecommons.org/licenses/by-nc-sa/3.0/
//
//All other rights reserved.
//
//THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, 
//EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED 
//WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

#include <Adafruit_NeoPixel.h>
#include <NintendoExtensionCtrl.h>
#include <Adafruit_GFX.h>
#include <gfxfont.h>
#include <LiquidCrystal_I2C.h>
#include <avr/wdt.h>
#include <EEPROM.h>


#define UP 1
#define LEFT 2
#define RIGHT 3
#define DOWN 4
#include "GamePiece.h"
#include "WS2812_Definitions.h"
#include "Wire.h"
//LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address

#define PIN 4

#define LED_ROWS 16
#define LED_COLUMNS 16
#define LED_COUNT 256 //LED_ROWS * LED_COLUMNS
#define GAME_COLUMNS 10
#define GAME_ROWS 16


#define ROWS 16 // snake
#define COLS 16

boolean lastButtonStateA = LOW;
boolean buttonStateA = LOW;
boolean lastButtonStateB = LOW;
boolean buttonStateB = LOW;
boolean lastButtonStateUp = LOW;
boolean buttonStateUp = LOW;
boolean lastButtonStateDown = LOW;
boolean buttonStateDown = LOW;
boolean lastButtonStateLeft = LOW;
boolean buttonStateLeft = LOW;
boolean lastButtonStateRight = LOW;
boolean buttonStateRight = LOW;
boolean lastButtonStateStart = LOW;
boolean buttonStateStart = LOW;
int gameState = 2;//0 for tetris , 1 for snake , 2 for picking
int gameScore;

byte gameField[GAME_ROWS * GAME_COLUMNS];
NESMiniController nes;
Adafruit_NeoPixel leds = Adafruit_NeoPixel(LED_COUNT, PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel matrix = Adafruit_NeoPixel(256, PIN, NEO_GRB + NEO_KHZ800);
LiquidCrystal_I2C lcd(0x27,20,4);  // Set the LCD I2C address

//uint32_t RED = matrix.Color(255, 0, 0);
//uint32_t GREEN = matrix.Color(0, 255, 0);
//uint32_t BLUE = matrix.Color(0, 0, 255);
uint32_t CLEAR = matrix.Color(0, 0, 0); 
//uint32_t PURPLE = matrix.Color(255, 0, 255); 
//uint32_t YELLOW = matrix.Color(255, 255, 0);

byte p1[4] = { 1, 1, 1, 1};  
byte p2[6] = { 2, 2, 2, 0, 2, 0};
byte p3[6] = { 3, 0, 3, 0, 3, 3};
byte p4[6] = { 0, 4, 0, 4, 4, 4};
byte p5[6] = { 5, 5, 0, 0, 5, 5};
byte p6[6] = { 0, 6, 6, 6, 6, 0};
byte p7[4] = { 7, 7, 7, 7 };

struct point {
  int x;
  int y;
};

point player[ROWS * COLS];

int playerDirection;
int playerLength;
point playerHead;
point apple;

int board[ROWS][COLS];

unsigned long lastClockTick;
int gameRate;
int numApplesEaten = 0;

GamePiece  _gamePieces[7] = 
{
	GamePiece(2, 2, p1 ),
	GamePiece(3, 2, p2 ),
	GamePiece(3, 2, p3 ),
	GamePiece(2, 3, p4 ),
	GamePiece(2, 3, p5 ),
	GamePiece(2, 3, p6 ),
	GamePiece(4, 1, p7 )
};

unsigned long colors[7] =
{
	RED,
	BLUE,
	GREEN,
	YELLOW,
	MAGENTA,
	SALMON,
	INDIGO  
};

GamePiece * fallingPiece = NULL;
GamePiece * nextPiece = NULL;

byte gameLevel = 1;
byte currentRow = 0;
byte currentColumn = 0;
byte gameLines = 0;
boolean gameOver = false;
int cursorState = 0;
unsigned long loopStartTime = 0;

float Normalize(int min, int max, int value)
{
	float result = -1.0 + (float)((value - min) << 1) / (max - min);
	return result < -1 ? -1 : result > 1 ? 1 : result;
}
void controllerBegin(){
  nes.begin();

  while (!nes.connect()) {
    Serial.println("NES Classic Controller not detected!");

  }

  if (nes.isKnockoff()) {  // Uh oh, looks like your controller isn't genuine?
    nes.setRequestSize(8);  // Requires 8 or more bytes for knockoff controllers
  }
  
}
void clearLEDs()
{
	for (int i=0; i<LED_COUNT; i++)
	{
		leds.setPixelColor(i, 0);
	}
}

void reboot() {
  wdt_disable();
  wdt_enable(WDTO_15MS);
  while (1) {}
}

void displayScoreTetris(int gover)
{
  //lcd.clear();
  
  updateScore();
  int tetrisHS = EEPROM.read(1);
  lcd.setCursor(2,0);
  lcd.print("**UCONN Tetris**");
  lcd.setCursor(2,1);
  lcd.print("High Score is ");
  lcd.print(tetrisHS, DEC);
  lcd.setCursor(2,2);
  lcd.print("Your Score is ");
  lcd.print(gameScore ,DEC);
  if (tetrisHS < gameScore) EEPROM.write(1,gameScore);
  
  if(gover){
    lcd.setCursor(2,3);
    lcd.print("Game Over");
  }
  else
  {
    lcd.setCursor(2,3);
    lcd.print("          ");
    
  }
  
}
void displayScoreSnake(int gover)
{
  //lcd.clear();
  int snakeHS = EEPROM.read(0);
  lcd.setCursor(2,0);
  lcd.print("**UCONN Snake**");
  lcd.setCursor(2,1);
  lcd.print("High Score is ");
  lcd.print(snakeHS, DEC);
  lcd.setCursor(2,2);
  lcd.print("Your Score is ");
  lcd.print(numApplesEaten ,DEC);
  if (snakeHS < numApplesEaten) EEPROM.write(0,numApplesEaten);
  if(gover){
    lcd.setCursor(2,3);
    lcd.print("Game Over");
    
  }
}
void setup() {
	randomSeed(analogRead(0));

	Serial.begin(9600);

	//init the game
  lcd.init();
  lcd.backlight();
//    for(int i = 0; i< 3; i++)
//  {
//    lcd.backlight();
//    delay(250);
//    lcd.noBacklight();
//    delay(250);
//  }
   lcd.backlight();
	controllerBegin();
	updateController();
 

  lcd.setCursor(2,0);
  lcd.print("UCONN ECE Dept.");
  lcd.setCursor(2,1);
  lcd.print("Please pick a game");
  lcd.setCursor(2,2);
  lcd.print("Tetris");
  lcd.setCursor(2,3);
  lcd.print("Snake");
  

//  if (nes.buttonA())
//  {
//    
//    gameState = cursorState;
//    if (gameState == 1)
//    {
//    matrix.begin();  
//    matrix.setBrightness(15);
//    matrix.show();
//    randomSeed(analogRead(0));
//    Serial.begin(9600);
//    defineBoard();  
//    startGame();  //snake
//    }
//    else
//    {
//        leds.begin();
//        clearLEDs();
//        startGame();
//        leds.show();
//    }
//    
//
//    
//  }


}



void loop() {
  
  if ( gameState == 2)
  {
    updateController();
    buttonStateStart = (nes.dpadDown() || nes.dpadUp());
    
    if (buttonStateStart != lastButtonStateStart){
      if (buttonStateStart == HIGH){
        if (cursorState == 1)
        {
          cursorState = 0;
          lcd.setCursor(0,2);
          lcd.print("X");
          lcd.setCursor(0,3);
          lcd.print(" ");
        }
        else 
        {
          cursorState = 1;
          lcd.setCursor(0,2);
          lcd.print(" ");
          lcd.setCursor(0,3);
          lcd.print("X");
        }
      Serial.println("flipped");
      delay(50);
      }
    }  
  
 lastButtonStateStart = buttonStateStart;
    if (nes.buttonA())
    {
    lcd.clear();
    gameState = cursorState;
    if (gameState == 1)
    {
    matrix.begin();  
    matrix.setBrightness(30);
    matrix.show();
    randomSeed(analogRead(0));
    defineBoard();  
    startGameS();  //snake
    }
    else
    {
        leds.begin();
        clearLEDs();
        startGame();
        leds.show();
    } 
  }
  }

if (gameState == 0){	
	readButtons();   
  displayScoreTetris(gameOver);

	//check if enough time passed to move the piece
	//with each level this time is geting shorter
	if( millis() - loopStartTime > (800 / (gameLevel * 0.80)) )
	{
		if( !gameOver )
		{        
			moveDown();
			gameOver = !isValidLocation(*fallingPiece, currentColumn, currentRow);       
		}
		loopStartTime = millis();

		render();
	}
}
else if (gameState == 1)
{
  updateController();
  displayScoreSnake(gameOver); 

  if(nes.dpadUp()) {
    playerDirection = UP;
  }
  else if(nes.dpadDown()) {
      playerDirection = DOWN;
    }
  else if(nes.dpadLeft()) {
      playerDirection = LEFT;
    } 
  else if(nes.dpadRight()) {
      playerDirection = RIGHT;
      Serial.println("hi");
    }
  else 
  
      
  if(millis() - lastClockTick > gameRate) {
    advancePlayer();    
    detectCollision();   
    detectAppleEaten();
    updateBoard();
    lastClockTick = millis();
  }
}
}


int leftXState = 0, newXState = 0;
int leftYState = 3, newYState = 3;

/// <summary>
/// Read buttons of the Wii classic controller and apply the actions
/// </summary>
void updateController() {
  boolean success = nes.update();  // Get new data from the controller
  nes.fixKnockoffData();
}


void readButtons()
{
	updateController();
  buttonStateA = nes.buttonA();
  buttonStateB = nes.buttonB();
  buttonStateUp = nes.dpadUp();
  buttonStateDown = nes.dpadDown();
  buttonStateLeft = nes.dpadLeft();
  buttonStateRight = nes.dpadRight();
  
	if (buttonStateB != lastButtonStateB){
    if (buttonStateB == HIGH){
		  if( gameOver )
	  	{
			  startGame();
		  	return;
		  }
		  else{
			rotateLeft();
      Serial.println("RotateLeft");
      delay(50);
		  }
    }  
	}
 lastButtonStateB = buttonStateB;

  if (buttonStateA != lastButtonStateA){
    if (buttonStateA == HIGH){
      if( gameOver )
      {
        startGame();
        return;
      }
      else{
      rotateRight();
      Serial.println("RotateRight");
      delay(50);
      }
    }  
  }
 lastButtonStateA = buttonStateA;


  if (buttonStateDown != lastButtonStateDown){
    if (buttonStateDown == HIGH){
      if( gameOver )
      {
        startGame();
        return;
      }
      else{
      drop();
      Serial.println("Drop");
      delay(50);
      }
    }  
  }
 lastButtonStateDown = buttonStateDown;

	if( !gameOver )
	{

  if (buttonStateRight != lastButtonStateRight){
    if (buttonStateRight == HIGH){
      if( gameOver )
      {
        startGame();
        return;
      }
      else{
      newXState =-1;
      Serial.println("right");
      delay(50);
      }
    }  
  }
 
 
		else if (buttonStateLeft != lastButtonStateLeft){
    if (buttonStateLeft == HIGH){
      if( gameOver )
      {
        startGame();
        return;
      }
      else{
      newXState =1;
      Serial.println("Left");
      delay(50);
      }
    }  
  }
		else
			newXState = 0;
    lastButtonStateRight = buttonStateRight;
    lastButtonStateLeft = buttonStateLeft;
		if( newXState != leftXState )
		{
			leftXState = newXState;

			if( leftXState == -1 )
				moveLeft();
			else if( leftXState == 1 )
				moveRight();
		}
	}}

	//   if( leftY < -0.5 )
	//    newYState = -1;
	//   else if( leftY > 0.5 )
	//    newYState =1;
	//   else
	//    newYState = 0;
	//    
	//    if( newYState != leftYState )
	//    {
	//      leftYState = newYState;
	//      if( leftYState != 0 )
	//      {
	//      int odd = (CurLed / 16) % 2;
	//      
	//      Serial.println(CurLed);
	//      Serial.println(odd);
	//      
	//      if( leftYState == -1 )
	//      {
	//         if( !odd )
	//           CurLed=(CurLed & 0xFFFFFFE0) - 1 - (CurLed%16);
	//         else
	//           CurLed=(((CurLed>>4)-1)<<4) + 15-(CurLed%16);
	//      }
	//       else if( leftYState == 1 )
	//       {
	//         if( !odd )
	//           CurLed=(CurLed & 0xFFFFFFE0) + 32 - 1 - CurLed%16;
	//         else
	//           CurLed=((1+(CurLed>>4))<<4) + 15-(CurLed%16);
	//       }
	//       
	//       Serial.println(CurLed);
	//      }
	//    }      


void setColor( int row, int column, unsigned long color)
{
	int odd = row%2;
	int led = 0;

	if( !odd )
		led = (row<<4)+column; 
	else
		led= ((row+1)<<4)-1 - column;

	leds.setPixelColor(led,color);
}

/// <summary>
/// Main rendering routine
/// </summary>
void render()
{
	int value = 0;
	unsigned long color = 0;

	//render game field first
	for( int row = 0; row < GAME_ROWS; row++)
	{
		for( int col = 0; col < GAME_COLUMNS; col++)
		{
			color = 0;
			value = gameField[row * GAME_COLUMNS+col];
			if( value > 0)
				color = colors[value-1];
			setColor(row,col, color);
		}
	}

	//render falling piece
	for( int row = 0; row < fallingPiece->Rows; row++)
	{
		for( int col = 0; col < fallingPiece->Columns; col++)
		{
			value = (*fallingPiece)(row,col);
			if( value > 0)    
				setColor(currentRow+row,currentColumn+col, colors[value-1]);
		}
	}

	//render divider line
	for( int row = 0; row < LED_ROWS; row++)
	{
		for( int col = GAME_COLUMNS; col < LED_COLUMNS; col ++)
		{
			if( col == GAME_COLUMNS )
				setColor(row,col, WHITE);
			else
				setColor(row,col, 0);
		}
	}

	//render next piece
	for( int row = 0; row < nextPiece->Rows; row++)
	{
		for( int col = 0; col < nextPiece->Columns; col++)
		{
			value = (*nextPiece)(row,col);
			if( value > 0)    
				setColor(7+row,12+col, colors[value-1]);
			else
				setColor(7+row,12+col, 0);
		}
	}  

	leds.show();
}

/// <summary>
/// Start new game
/// </summary>

void advancePlayer() {
  if(playerDirection == LEFT) {
    playerHead.y -= 1;
  } else if(playerDirection == RIGHT) {    
    playerHead.y += 1;    
  } else if(playerDirection == UP) {    
    playerHead.x -= 1;    
  } else if(playerDirection == DOWN) {        
    playerHead.x += 1;
  }
  // see if this point already exists in the player's matrix
  for(int i = 0; i < playerLength; i++) {
    if(player[i].x == playerHead.x && player[i].y == playerHead.y) {
      gameOverS();
    }
  }
  for(int i = playerLength - 1; i > 0; i--) {
    player[i] = player[i - 1];
  }
  player[0].x = playerHead.x;
  player[0].y = playerHead.y;
}

void detectCollision() {
  if(board[playerHead.x][playerHead.y] == 1) {
    gameOverS();
  }
}
void detectAppleEaten() {
  if(playerHead.x == apple.x && playerHead.y == apple.y) {
    numApplesEaten++;
    playerLength += 1;
    player[playerLength - 1].x = playerHead.x;
    player[playerLength - 1].y = playerHead.y;
    if(numApplesEaten % 5 == 0 && gameRate > 100) {
      gameRate -= 20;
    }
    generateApple();
  }
}

void updateBoard() {  
  drawBoard();
  drawPlayer();
  drawApple();
  matrix.show();
}
void gameOverS() {
  matrix.clear();
  for(int i = 0; i < ROWS; i++) {
    for(int j = 0; j < COLS; j++) {
      matrix.setPixelColor(convertToMatrixPoint(i, j), GREEN);      
    }    
  }
  matrix.show();
  delay(3000);
  startGameS();
}

int convertToMatrixPoint(int i, int j) {
  if(i % 2 == 0) {
    return (COLS * i) + (COLS - 1) - j;
  } else {
    return (COLS * i) + j;
  }
}

void defineBoard() {  
  // draw the outer board
  for(int i = 0; i < COLS; i++) {
    board[0][i] = 1;    
    board[ROWS - 1][i] = 1;
  }  
  generateRandomBoard();
}

void generateRandomBoard() {
  // clear the existing board first
  for(int i = 1; i < ROWS - 1; i++) {
    for(int j = 0; j < COLS; j++) {
      if(j == 0 || j == (COLS - 1)) {
        board[i][j] = 1;         
      } else {
        board[i][j] = 0;
      }
    }
  }
  // 20 random pixels set, but make sure that it won't cause the player to lose (diagonal pixels)
//  for(int i = 0; i < 20; i++) {
//    bool found = false;
//    int x = random(1, ROWS - 1);
//    int y = random(1, COLS - 1);
//    while(!found) {          
//      if(!boardContainsCoordinates(x, y)
//         && !boardContainsCoordinates(x - 1, y - 1)         
//         && !boardContainsCoordinates(x + 1, y - 1)
//         && !boardContainsCoordinates(x - 1, y + 1) 
//         && !boardContainsCoordinates(x + 1, y + 1)){
//         found = true;
//      } else {
//        x = random(1, ROWS - 1);
//        y = random(1, COLS - 1);
//      }
//    }
//    board[x][y] = 1;
//  }
}

void startGameS() {
  matrix.clear();
  resetGameVariables();
  drawBoard();  
  drawPlayer();  
  drawApple();
  matrix.show();  
}

void resetGameVariables() {  
  generateRandomBoard();
  bool found = false;  
  // generate a random spot for the user to start, and a random direction
  while(!found) {
    // start the player in a random spot
    playerHead.x = random(1, ROWS - 1);
    playerHead.y = random(1, COLS - 1);
    int startDirection = random(0, 2);
    
    if(startDirection == 0) {
      if(playerHead.y < COLS / 2) {
        playerDirection = RIGHT;
      } else {
        playerDirection = LEFT;
      }
    } else {
      if(playerHead.x < ROWS / 2) {
        playerDirection = DOWN;
      } else {
        playerDirection = UP;
      }  
    }

    if(playerHas5Moves()) {
      found = true;
    }
  }

  generateApple();

  playerLength = 1;
  player[0].x = playerHead.x;
  player[0].y = playerHead.y;

  lastClockTick = millis();
  gameRate = 200;  
  numApplesEaten = 0;
}
// make sure with the random start that the player has a few moves to react
bool playerHas5Moves() {
  for(int i = 0; i < 5; i++) {
    switch(playerDirection) {
      case RIGHT:
        if(board[playerHead.x][playerHead.y + i] == 1) {
          return false;
        }
        break;
      case LEFT:
        if(board[playerHead.x][playerHead.y - i] == 1) {
          return false;
        }
        break;
      case UP:
        if(board[playerHead.x - i][playerHead.y] == 1) {
          return false;
        }
        break;
      case DOWN:
        if(board[playerHead.x + i][playerHead.y] == 1) {
          return false;
        }
        break;
    }
  }
  return true;
}
void generateApple() {
  bool found = false;
  // make sure that the apple doesn't end up on a board coordinate or on top of the player
  while(!found) {    
    apple.x = random(1, ROWS - 1);
    apple.y = random(1, COLS - 1);
    if(!playerContainsCoordinates(apple.x, apple.y) && !boardContainsCoordinates(apple.x, apple.y)) {       
      found = true;
    }
  }  
}
bool playerContainsCoordinates(int x, int y) {  
  for(int i = 0; i < playerLength; i++) {
    if(player[i].x == x && player[i].y == y) {      
      return true;
    }
  }
  return false;
}
bool boardContainsCoordinates(int x, int y) {
  return board[x][y] == 1;
}

void drawPlayer() {  
  for(int i = 0; i < playerLength; i++) {        
    matrix.setPixelColor(convertToMatrixPoint(player[i].x, player[i].y), BLUE);
  }  
}
void drawBoard() {
  for(int i = 0; i < ROWS; i++) {
    for(int j = 0; j < COLS; j++) {
      if(board[i][j] == 1) {        
        matrix.setPixelColor(convertToMatrixPoint(i, j), GREEN);     
      } else {
        matrix.setPixelColor(convertToMatrixPoint(i, j), CLEAR);
      }
    }
  }  
  // identify the bottom left pixel by painting it blue
  //matrix.setPixelColor(convertToMatrixPoint(0, 0), BLUE);   
  // identify the matrix start pixels and direction
  //matrix.setPixelColor(0, PURPLE);
  // matrix.setPixelColor(1, YELLOW);
}
void drawApple() {
  matrix.setPixelColor(convertToMatrixPoint(apple.x, apple.y), RED);
}


void startGame()
{
	//Serial.println("Start game");
	nextPiece=NULL;
	gameLines = 0;
	loopStartTime = 0;
	newLevel(1);
	gameOver = false;
	render();
}

/// <summary>
/// Start a level
/// </summary>
void newLevel(uint8_t level)
{
	gameLevel = level;
	emptyField();
	newPiece();
}

/// <summary>
/// Empty game field (only part where game piece can be located
/// </summary>
void emptyField()
{
	for(int i = 0; i < GAME_ROWS * GAME_COLUMNS; i++ )
	{
		gameField[i] = 0;
	}
}

/// <summary>
/// Get new piece
/// </summary>
void newPiece()
{
	int next;

	currentColumn = 4;
	currentRow = 0;


	if (nextPiece == NULL)
	{
		next = random(100) % 7; 
		nextPiece = &_gamePieces[next];
	}

	if( fallingPiece != NULL )
		delete fallingPiece;

	fallingPiece = new GamePiece(*nextPiece);
	next = random(100) % 7;
	nextPiece = &_gamePieces[next];  
}

/// <summary>
/// Check if the piece can be placed at the given location
/// </summary>
boolean isValidLocation(GamePiece & piece, byte column, byte row)
{
	for (int i = 0; i < piece.Rows; i++)
		for (int j = 0; j < piece.Columns; j++)
		{
			int newRow = i + row;
			int newColumn = j + column;                    

			//location is outside of the fieled
			if (newColumn < 0 || newColumn > GAME_COLUMNS - 1 || newRow < 0 || newRow > GAME_ROWS - 1)
			{
				//piece part in that location has a valid square - not good
				if (piece(i, j) != 0)
					return false;
			}else
			{
				//location is in the field but is already taken, pice part for that location has non-empty square 
				if (gameField[newRow*GAME_COLUMNS + newColumn] != 0 && piece(i, j) != 0)
					return false;
			}
		}

		return true;  
}

/// <summary>
/// Move the piece down
/// </summary>
void moveDown()
{
	if (isValidLocation(*fallingPiece, currentColumn, currentRow + 1))
	{
		currentRow +=1;
		return;
	}


	//The piece can't be moved anymore, merge it into the game field
	for (int i = 0; i < fallingPiece->Rows; i++)
	{
		for (int j = 0; j < fallingPiece->Columns; j++)
		{
			byte value = (*fallingPiece)(i, j);
			if (value != 0)
				gameField[(i + currentRow) * GAME_COLUMNS + (j + currentColumn)] = value;
		}
	}

	//Piece is merged update the score and get a new pice
	updateScore();            
	newPiece();  
}

/// <summary>
/// Drop it all the way down
/// </summary>
void drop()
{
	while (isValidLocation(*fallingPiece, currentColumn, currentRow + 1))
		moveDown();
}

/// <summary>
/// Move falling game piece to the left
/// </summary>
void moveLeft()
{
	if (isValidLocation(*fallingPiece, currentColumn - 1, currentRow))
		currentColumn--;
}


/// <summary>
/// Move falling game piece to the right
/// </summary>
void moveRight()
{
	if (isValidLocation(*fallingPiece, currentColumn + 1, currentRow))
		currentColumn++;
}

/// <summary>
/// Rotate falling game piece CW
/// </summary>
void rotateRight()
{
	GamePiece * rotated = fallingPiece->rotateRight();

	if (isValidLocation(*rotated, currentColumn, currentRow))
	{
		delete fallingPiece;
		fallingPiece = rotated;
	}else
        {
          delete rotated;
        }
}

/// <summary>
/// Rotate falling game piece CCW
/// </summary>
void rotateLeft()
{
	GamePiece * rotated = fallingPiece->rotateLeft();

	if (isValidLocation(*rotated, currentColumn, currentRow))
	{
		delete fallingPiece;
		fallingPiece = rotated;
	}else
        {
          delete rotated;
        }
}

/// <summary>
/// Clean good line(s), shift the rest down and update the score if required
/// </summary>
void updateScore()
{
	int count = 0;
	for (int row = 1; row < GAME_ROWS; row++)
	{
		boolean goodLine = true;
		for (int col = 0; col < GAME_COLUMNS; col++)
		{
			if (gameField[row *GAME_COLUMNS + col] == 0)
				goodLine = false;
		}

		if (goodLine)
		{
			count++;
			for (int i = row; i > 0; i--)
				for (int j = 0; j < GAME_COLUMNS; j++)
					gameField[i *GAME_COLUMNS +j] = gameField[(i - 1)*GAME_COLUMNS+ j];
		}
	}


	if (count > 0)
	{
		gameScore += count * (gameLevel * 10);
		gameLines += count;


		int nextLevel = (gameLines / GAME_ROWS) + 1;
		if (nextLevel > gameLevel)
		{
			gameLevel = nextLevel;
			newLevel(gameLevel);
      displayScoreTetris(gameOver);
		}
	}
}
