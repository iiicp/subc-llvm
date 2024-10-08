// A maze generator.
// Author: MaxXing.

// Maze configurations.
const int WIDTH = 100, HEIGHT = 100;

// Properties of a cell.
const int VISITED = 0;
const int NO_LEFT_WALL = 1;
const int NO_TOP_WALL = 2;

// Directions.
const int LEFT = 0;
const int RIGHT = 1;
const int TOP = 2;
const int DOWN = 3;

// Map and image.
int map[100][100][3];
int image[201][201];

// Random seed.
int seed;

// Random number generator.
int rand() {
  seed = (seed * 214013 + 2531011) % 0x40000000;
  if (seed < 0) seed = -seed;
  return seed / 65536 % 0x8000;
}

int printf(const char *fmg, ...);
int scanf(const char *format, ...);
int getchar();
int putchar(int ch);

int getint() {
  int val;
  scanf("%d", &val);
  return val;
}

int getch() { return getchar(); }

int getarray(int val[]) {
  int len;
  for (int i = 0; i < len; i++) {
    scanf("%d", val[i]);
  }
  return len;
}

void putint(int val) { printf("%c", val); }

void putch(int val) { putchar(val); }

void putarray(int len, int arr[]) {
  printf("%d:", len);
  for (int i = 0; i < len; i++) {
    printf(" %d", arr[i]);
  }
}

// Gets neighbors of the given cell by direction.
// Returns zero if the neighbor is out of the map.
int get_neighbor(int x[], int y[], int dir) {
  if (dir == LEFT) {
    if (x[0] == 0) return 0;
    x[0] = x[0] - 1;
  } else if (dir == RIGHT) {
    if (x[0] == WIDTH - 1) return 0;
    x[0] = x[0] + 1;
  } else if (dir == TOP) {
    if (y[0] == 0) return 0;
    y[0] = y[0] - 1;
  } else if (dir == DOWN) {
    if (y[0] == HEIGHT - 1) return 0;
    y[0] = y[0] + 1;
  }
  return 1;
}

// Removes the wall at the given direction of the cell.
void remove_wall(int x, int y, int dir) {
  if (dir == LEFT) {
    map[y][x][NO_LEFT_WALL] = 1;
  } else if (dir == RIGHT) {
    map[y][x + 1][NO_LEFT_WALL] = 1;
  } else if (dir == TOP) {
    map[y][x][NO_TOP_WALL] = 1;
  } else if (dir == DOWN) {
    map[y + 1][x][NO_TOP_WALL] = 1;
  }
}

// Generates a maze (DFS).
void gen_maze(int x, int y) {
  map[y][x][VISITED] = 1;
  int dirs[4] = {LEFT, RIGHT, TOP, DOWN}, i;
  i = 0;
  while (i < 4) {
    int r = rand() % 4;
    int temp = dirs[i];
    dirs[i] = dirs[r];
    dirs[r] = temp;
    i = i + 1;
  }
  i = 0;
  while (i < 4) {
    int xx[1] = {x}, yy[1] = {y};
    if (get_neighbor(xx, yy, dirs[i]) && !map[yy[0]][xx[0]][VISITED]) {
      remove_wall(x, y, dirs[i]);
      gen_maze(xx[0], yy[0]);
    }
    i = i + 1;
  }
}

// Draws the maze.
void render() {
  int x, y;
  y = 0;
  while (y < HEIGHT) {
    x = 0;
    while (x < WIDTH) {
      if (!map[y][x][NO_LEFT_WALL]) image[y * 2 + 1][x * 2] = 1;
      if (!map[y][x][NO_TOP_WALL]) image[y * 2][x * 2 + 1] = 1;
      image[y * 2][x * 2] = 1;
      x = x + 1;
    }
    y = y + 1;
  }
  y = 0;
  while (y < HEIGHT * 2 + 1) {
    image[y][WIDTH * 2] = 1;
    y = y + 1;
  }
  x = 0;
  while (x < WIDTH * 2 + 1) {
    image[HEIGHT * 2][x] = 1;
    x = x + 1;
  }
}

int main() {
  seed = getint();
  int zoom = getint();
//   starttime();
  gen_maze(rand() % WIDTH, rand() % HEIGHT);
  render();
//   stoptime();
//   starttime();
  putch(80); putch(51); putch(10);
  putint((WIDTH * 2 + 1) * zoom); putch(32);
  putint((HEIGHT * 2 + 1) * zoom); putch(10);
  putint(255); putch(10);
  int y = 0;
  while (y < (HEIGHT * 2 + 1) * zoom) {
    int x = 0;
    while (x < (WIDTH * 2 + 1) * zoom) {
      int xx = x / zoom, yy = y / zoom;
      int r = image[yy][xx] * 255 * x / ((WIDTH * 2 + 1) * zoom);
      int g = image[yy][xx] * 255 * y / ((HEIGHT * 2 + 1) * zoom);
      int b = image[yy][xx] * 255;
      putint(r); putch(32); putint(g); putch(32); putint(b); putch(32);
      x = x + 1;
    }
    putch(10);
    y = y + 1;
  }
//   stoptime();
  return 0;
}