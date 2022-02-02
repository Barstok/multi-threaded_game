#ifndef _CONST_
#define _CONST_
#define MAX_MAP_SIZE 128
#define MAX_PLAYERS 4
#define SIGHT 5
#define ROUND_TIME_MS 50


//extended colors
#define COLOR_DARK_GREEN 22
#define COLOR_BRIGHT_GREEN 46
#define COLOR_GOLDEN 226
#define COLOR_ORANGE 202

enum map_elements{
    PLAYER=1,
    WALL,
    COIN,
    CAMPSITE,
    BUSH,
    BEAST,
    DEFAULT
};

enum type{
    HUMAN,
    CPU
};


#endif