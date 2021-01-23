
#include "Adafruit_NeoTrellis.h"

#define Y_DIM 8 //number of rows of key
#define X_DIM 8 //number of columns of keys

#define BRIGHTNESS 10

// Create a matrix of trellis panels
Adafruit_NeoTrellis trellis_array[Y_DIM/4][X_DIM/4] = {
  { Adafruit_NeoTrellis(0x2E), Adafruit_NeoTrellis(0x2F) },
  { Adafruit_NeoTrellis(0x30), Adafruit_NeoTrellis(0x32) }
};

// Pass this matrix to the multitrellis object
Adafruit_MultiTrellis trellis((Adafruit_NeoTrellis *)trellis_array, Y_DIM/4, X_DIM/4);

// Input a value 0 to 255 to get a color value.
// The colors are a transition r - g - b - back to r.
uint32_t wheel(byte wheel_pos) {
  if(wheel_pos < 85) {
    return seesaw_NeoPixel::Color(wheel_pos * 3, 255 - wheel_pos * 3, 0);
  } else if(wheel_pos < 170) {
    wheel_pos -= 85;
    return seesaw_NeoPixel::Color(255 - wheel_pos * 3, 0, wheel_pos * 3);
  } else {
    wheel_pos -= 170;
    return seesaw_NeoPixel::Color(0, wheel_pos * 3, 255 - wheel_pos * 3);
  }
  return 0;
}

int currentLed[X_DIM * Y_DIM];

int board[X_DIM][Y_DIM] = {
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 1, 2, 0, 0, 0 },
  { 0, 0, 0, 2, 1, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0 }
};

int current_player = 1;

// qsort requires you to create a sort function
int sort_desc(const void *cmp1, const void *cmp2) {
  int a = *((int *)cmp1);
  int b = *((int *)cmp2);
  return b - a;
}

int sort_board() {
  int player1_count = 0;
  int player2_count = 0;
  int blank_count   = 0;

  for(int x = 0; x < 8; x++) {
    for(int y = 0; y < 8; y++) {
      if(board[x][y] == 0) blank_count++;
      if(board[x][y] == 1) player1_count++;
      if(board[x][y] == 2) player2_count++;
    }
  }
  for(int x = 0; x < 8; x++) {
    for(int y = 0; y < 8; y++) {
      if(player1_count > 0) {
        board[x][y] = 1;
        player1_count--;
      } else if(blank_count > 0) {
        board[x][y] = 0;
        blank_count--;
      } else if(player2_count > 0) {
        board[x][y] = 2;
        player2_count--;
      }
    }
  }

  // sorting now makes things a bit more even
  for(int x = 0; x < 8; x++) {
    qsort(board[x], 8, sizeof(board[x][0]), sort_desc);
  }
}

void set_brightness(int level) {
	for (int x = 0; x < X_DIM / 4; x++) {
		for (int y = 0; y < Y_DIM / 4; y++) {
		  trellis_array[y][x].pixels.setBrightness(level);
		}
	}
}

int other_player(int player) {
  return player % 2 + 1;
}

int ends_with_player(int player, int x, int y, int dx, int dy) {
  if(x < 0) return 0;
  if(x > 7) return 0;
  if(y < 0) return 0;
  if(y > 7) return 0;

  if(board[x][y] == player) {
    return 1;
  }

  if(board[x][y] == other_player(player)) {
    return ends_with_player(player, x+dx, y+dy, dx, dy);
  }

  return 0;
}

int flip_until_player(int player, int x, int y, int dx, int dy) {
  Serial.print("Flip move until player ");
  Serial.print(player);
  Serial.print(" @ ");
  Serial.print(x);
  Serial.print(",");
  Serial.print(y);
  Serial.print(" | ");
  Serial.print(dx);
  Serial.print(",");
  Serial.println(dy);

  // Done when we fall off the board
  if(x < 0) return 0;
  if(x > 7) return 0;
  if(y < 0) return 0;
  if(y > 7) return 0;

  // Done when we get to the player
  if(board[x][y] == player) {
    return 1;
  }

  if(board[x][y] == other_player(player)) {
    if(flip_until_player(player, x+dx, y+dy, dx, dy)) {
      board[x][y] = player;
      return 1;
    }
  }
  return 0;
}

int valid_move_for(int player, int x, int y) {
  if(board[x][y] != 0) {
    return 0;
  }

  for(int dx = -1; dx <= 1; dx++) {
    for(int dy = -1; dy <= 1; dy++) {
      // Must be a direction
      if(dx == 0 && dy == 0) continue;

      // Not edge of board
      if(x + dx < 0) continue;
      if(x + dx > 7) continue;
      if(y + dy < 0) continue;
      if(y + dy > 7) continue;

      // Neighbor is other player
      if(board[x + dx][y + dy] != other_player(current_player))
        continue;

      if(ends_with_player(current_player, x+dx, y+dy, dx, dy)) {
        return 1; // shortcut
      }
    }
  }

  return 0;
}

void do_move_for(int player, int x, int y) {
  Serial.print("Doing move for ");
  Serial.print(player);
  Serial.print(" @ ");
  Serial.print(x);
  Serial.print(",");
  Serial.println(y);

  board[x][y] = player;

  for(int dx = -1; dx <= 1; dx++) {
    for(int dy = -1; dy <= 1; dy++) {
      // Must be a direction
      if(dx == 0 && dy == 0) continue;

      // Not edge of board
      if(x + dx < 0) continue;
      if(x + dx > 7) continue;
      if(y + dy < 0) continue;
      if(y + dy > 7) continue;

      // Neighbor is other player
      if(board[x + dx][y + dy] != other_player(current_player))
        continue;

      flip_until_player(current_player, x+dx, y+dy, dx, dy);
    }
  }
}

void draw_board() {
  int valid_move_count = 0;
  int empty_space_count = 0;
  for(int board_x = 0; board_x < 8; board_x++) {
    for(int board_y = 0; board_y < 8; board_y++) {
      empty_space_count++;
      Serial.print(board[board_x][board_y]);
      Serial.print(" ");
      if(board[board_x][board_y] == 0) {
        // If is a valid move for the current player, let's show it but dim?
        if(valid_move_for(current_player, board_x, board_y)) {
          valid_move_count++;
          set_brightness(2);
          trellis.setPixelColor(board_x, board_y, wheel( current_player * 100 - 20));
        } else {
          trellis.setPixelColor(board_x, board_y, 0x000000);
        }
      } else {
        set_brightness(20);
        trellis.setPixelColor(board_x, board_y, wheel( board[board_x][board_y] * 100 ));
      }
    }
    Serial.println("");
  }
  trellis.show();

  // Game over!
  if(valid_move_count == 0) {
    sort_board();
  }

  //return valid_move_count;
}

void mutate_neighbors(int current_player, int x, int y) {
  for(int i = x - 1; i <= x + 1; i++) {
    if(i < 0) continue;
    if(i > 7) continue;
    for(int j = y - 1; j <= y + 1; j++) {
      if(j < 0) continue;
      if(j > 7) continue;
      if(board[i][j] > 0) {
        board[i][j] = current_player;
      }
    }
  }
}


// Define a callback for key presses
TrellisCallback blink(keyEvent evt) {

  // Rising == push, Falling == release
  if(evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING) {
    Serial.print("Player: ");
    Serial.print(current_player);

    int x = evt.bit.NUM % 8;
    int y = evt.bit.NUM / 8;

    Serial.print(" x: ");
    Serial.print(x);
    Serial.print(" y: ");
    Serial.print(y);

    Serial.print(" current: ");
    Serial.println(board[x][y]);

    if(valid_move_for(current_player, x, y)) {
      do_move_for(current_player, x, y);
      current_player = other_player(current_player);
    }

    draw_board();

  } else if(evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING) {
    // trellis.setPixelColor(evt.bit.NUM, 0); // Off falling
  }

  return 0;
}

void setup() {
  Serial.begin(9600);

  if(!trellis.begin()){
    Serial.println("Failed to begin trellis!");
    while(1);
  } else {
    Serial.println("Trellis started!");
  }

  set_brightness(BRIGHTNESS);

  // Fancy random initialization display
  for(int n = 0; n < 10; n++) {
    for(int i=0; i < Y_DIM * X_DIM; i++) {
      trellis.setPixelColor(i, random(0x1000000));
    }
    trellis.show();
    delay(50);
  }

  // Hook up callbacks for every button
  for(int y=0; y<Y_DIM; y++){
    for(int x=0; x<X_DIM; x++){
      // activate rising and falling edges on all keys
      trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_RISING, true);
      trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_FALLING, true);
      trellis.registerCallback(x, y, blink);
      trellis.setPixelColor(x, y, 0x000000); // Addressed with x,y
      trellis.show(); // Show all LEDs
    }
    delay(5);
  }

  draw_board();
}

void loop() {
  trellis.read();
  delay(20);
}
