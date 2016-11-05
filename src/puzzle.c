#include "puzzle.h"
#define TILE_W 16
#define TILE_H 16
int ScreenWidth = 320, ScreenHeight = 240;

SDL_Window *window = NULL;
SDL_Renderer *ScreenRenderer = NULL;
SDL_Texture *IconTexture = NULL;
SDL_Texture *TileSheet = NULL;
SDL_Surface *IconSurface = NULL;
SDL_Surface *WindowIcon = NULL;
int quit = 0;
int retraces = 0;


void DrawPlayfield(struct Playfield *P, int DrawX, int DrawY) {
  int VisualWidth = P->Width * TILE_W;
  int VisualHeight = (P->Height-1) * TILE_H;

  // border
  SDL_SetRenderDrawColor(ScreenRenderer, 0, 0, 0, 255);
  SDL_Rect BorderRectangle = {DrawX-1, DrawY-1, VisualWidth + 2, VisualHeight + 2};
  SDL_RenderDrawRect(ScreenRenderer, &BorderRectangle);

  SDL_Rect ClipRectangle = {DrawX, DrawY, P->Width * TILE_W, (P->Height-1) * TILE_H};
  SDL_RenderSetClipRect(ScreenRenderer, &ClipRectangle);

  for(int y=0; y<VisualHeight; y++) {
    int Value = (float)y / VisualHeight * 255;
    SDL_SetRenderDrawColor(ScreenRenderer, Value, Value, Value, 255);
    SDL_RenderDrawLine(ScreenRenderer, DrawX, DrawY+y, DrawX+VisualWidth-1, DrawY+y);
  }

/*
  // fill the inside
  SDL_SetRenderDrawColor(ScreenRenderer, 255, 255, 255, 255);
  SDL_Rect FillRectangle = {DrawX, DrawY, P->Width * TILE_W, P->Height * TILE_H};
  SDL_RenderFillRect(ScreenRenderer, &FillRectangle);
*/

  // Draw tiles
  for(int x=0; x<P->Width; x++)
    for(int y=0; y<P->Height; y++) {
      int Tile = P->Playfield[P->Width * y + x];
      blit(TileSheet, ScreenRenderer, TILE_W*Tile, 0, DrawX+x*TILE_W, DrawY+y*TILE_H-P->Rise, 16, 16);
    }
  // Draw exploding blocks
  for(struct MatchRow *Heads = P->Match; Heads; Heads=Heads->Next) {
    int SourceY = Heads->Timer1?TILE_H:TILE_H*2;
    for(struct MatchRow *Match = Heads; Match; Match=Match->Child) {
      for(int i=0; i<Match->DisplayWidth; i++)
        blit(TileSheet, ScreenRenderer, TILE_W*Match->Color, SourceY, DrawX+(Match->DisplayX+i)*TILE_W, DrawY+Match->Y*TILE_H-P->Rise, 16, 16);
    }
  }

  // Draw the cursor
  blit(TileSheet, ScreenRenderer, 0, TILE_H, DrawX+P->CursorX*TILE_W, DrawY+P->CursorY*TILE_H-P->Rise, TILE_W, TILE_H);
  blit(TileSheet, ScreenRenderer, 0, TILE_H, DrawX+(P->CursorX+1)*TILE_W, DrawY+P->CursorY*TILE_H-P->Rise, TILE_W, TILE_H);

  SDL_RenderSetClipRect(ScreenRenderer, NULL);
}

int main(int argc, char *argv[]) {
  if(SDL_Init(SDL_INIT_VIDEO) < 0){
    printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    return -1;
  }

  window = SDL_CreateWindow("puzzle", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, ScreenWidth, ScreenHeight, SDL_WINDOW_SHOWN);
  if(!window) {
     SDL_MessageBox(SDL_MESSAGEBOX_ERROR, "Error", NULL, "Window could not be created! SDL_Error: %s", SDL_GetError());
     return -1;
  }
  if(!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)){
    SDL_MessageBox(SDL_MESSAGEBOX_ERROR, "Error", NULL, "SDL_image could not initialize! SDL_image Error: %s", IMG_GetError());
    return -1;
  }
  if( TTF_Init() == -1 ) {
    SDL_MessageBox(SDL_MESSAGEBOX_ERROR, "Error", NULL, "SDL_ttf could not initialize! SDL_ttf Error: %s", TTF_GetError());
    return -1;
  }
  ScreenRenderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_TARGETTEXTURE);
  if(!SDL_RenderTargetSupported(ScreenRenderer)) {
    SDL_MessageBox(SDL_MESSAGEBOX_ERROR, "Error", NULL, "Rendering to a texture isn't supported");
    return -1;
  }

  // set window icon
  WindowIcon = SDL_LoadImage("data/icon.png", 0);
  SDL_SetWindowIcon(window, WindowIcon);

  TileSheet = LoadTexture("data/gfx.png", 0);

  struct Playfield Player1;
  memset(&Player1, 0, sizeof(struct Playfield));
  Player1.Width = 6;
  Player1.Height = 13;
  Player1.Playfield = calloc(Player1.Width * Player1.Height, sizeof(int));
  Player1.FallingColumns = calloc(Player1.Width, sizeof(struct FallingData));
  for(int i=0; i<6; i++)
    for(int j=8; j<13; j++)
      Player1.Playfield[j*Player1.Width + i] = (rand()%(BLOCK_BLUE-1))+1;

  SDL_SetRenderDrawColor(ScreenRenderer, 128, 128, 128, 255);
  SDL_RenderClear(ScreenRenderer); 

  while(!quit) {
    SDL_Event e;
    while(SDL_PollEvent(&e) != 0) {
      if(e.type == SDL_QUIT)
        quit = 1;
    }

    const Uint8 *Keyboard = SDL_GetKeyboardState(NULL);
    Player1.KeyDown[KEY_LEFT] = Keyboard[SDL_SCANCODE_LEFT];
    Player1.KeyDown[KEY_DOWN] = Keyboard[SDL_SCANCODE_DOWN];
    Player1.KeyDown[KEY_UP] = Keyboard[SDL_SCANCODE_UP];
    Player1.KeyDown[KEY_RIGHT] = Keyboard[SDL_SCANCODE_RIGHT];
    Player1.KeyDown[KEY_OK] = Keyboard[SDL_SCANCODE_RETURN];
    Player1.KeyDown[KEY_BACK] = Keyboard[SDL_SCANCODE_BACKSPACE];
    Player1.KeyDown[KEY_SWAP] = Keyboard[SDL_SCANCODE_SPACE];
    Player1.KeyDown[KEY_LIFT] = Keyboard[SDL_SCANCODE_Z];
    Player1.KeyDown[KEY_PAUSE] = Keyboard[SDL_SCANCODE_P];

    UpdatePlayfield(&Player1);
    DrawPlayfield(&Player1, 112, 24);

    SDL_RenderPresent(ScreenRenderer);
    SDL_Delay(17);
    retraces++;
  }
  SDL_Quit();

  return 0;
}
