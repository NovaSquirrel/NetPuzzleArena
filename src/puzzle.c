/*
 * Net Puzzle Arena
 *
 * Copyright (C) 2016 NovaSquirrel
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "puzzle.h"
int TILE_W = 16, TILE_H = 16;
int ScreenWidth = 320, ScreenHeight = 240;
char *PrefPath;
SDL_Window *window = NULL;
SDL_Renderer *ScreenRenderer = NULL;
SDL_Texture *IconTexture = NULL;
SDL_Texture *TileSheet = NULL;
SDL_Texture *GameFont = NULL;
SDL_Texture *GameBG = NULL;
TTF_Font *ChatFont = NULL;
SDL_Surface *IconSurface = NULL;
SDL_Surface *WindowIcon = NULL;
int quit = 0;
int retraces = 0;
int KeyboardOnly = 0;
struct Playfield Player1;
char TempString[1024];
int FrameAdvanceMode = 0;
int FrameAdvance = 0;
struct JoypadMapping ActiveJoysticks[ACTIVE_JOY_MAX];

// config flags
int ScaleFactor = 1, NoAcceleration = 0;

#ifdef ENABLE_AUDIO
  Mix_Chunk *SampleSwap, *SampleDrop, *SampleDisappear, *SampleMove, *SampleCombo;
#endif

void DrawPlayfield(struct Playfield *P, int DrawX, int DrawY);
void SetGameDefaults(struct Playfield *P, int Game);
void InitPlayfield(struct Playfield *P);
int ShowTitle();
void ShowMainOptions();

void GameplayStart() {
  memset(&Player1, 0, sizeof(struct Playfield));
  Player1.Flags = LIFT_WHILE_CLEARING | MOUSE_CONTROL;
  SetGameDefaults(&Player1, FRENZY);
  InitPlayfield(&Player1);

/*
  struct GarbageSlab *Slab = (struct GarbageSlab*)malloc(sizeof(struct GarbageSlab));
  memset(Slab, 0, sizeof(struct GarbageSlab));
  Slab->X = 1;
  Slab->Y = 1;
  Slab->Width = 2;
  Slab->Height = 3;
  Player1.GarbageSlabs = Slab;
*/

  int FrameAdvanceContinuous = 0;
  while(!quit) {
    FrameAdvance = 0;
    SDL_Event e;
    CombinedUpdateKeys(&Player1);

    while(SDL_PollEvent(&e) != 0) {
      if(e.type == SDL_QUIT)
        quit = 1;
      else if(e.type == SDL_KEYDOWN) {
        if(e.key.keysym.scancode == SDL_SCANCODE_GRAVE)
          FrameAdvance = 1;
        if(e.key.keysym.scancode == SDL_SCANCODE_1)
          FrameAdvanceContinuous = 1;
      } else if(e.type == SDL_KEYUP) {
        if(e.key.keysym.scancode == SDL_SCANCODE_1)
          FrameAdvanceContinuous = 0;
      } else if(e.type == SDL_MOUSEMOTION && (Player1.Flags & MOUSE_CONTROL)) {
        int Buttons = SDL_GetMouseState(NULL, NULL);
        // ignore if button isn't pressed
        if(Buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) {
          int VisualWidth = Player1.Width * TILE_W;
          int VisualHeight = (Player1.Height-1) * TILE_H;
          int X = (ScreenWidth/2)-(Player1.Width*TILE_W)/2;
          int Y = (ScreenHeight/2)-((Player1.Height-1)*TILE_H)/2;
          int Rise = Player1.Rise * (TILE_H / 16);
          // ignore if outside the playfield
          if(e.motion.x > X && e.motion.x < X+VisualWidth && e.motion.y > Y && e.motion.y < Y+VisualHeight) {
            int MouseX = (e.motion.x - X) / TILE_W;
            int MouseY = (e.motion.y - Y + Rise) / TILE_H;
            if(Player1.MouseX != MouseX && Player1.MouseY == MouseY) {
              Player1.KeyNew[KEY_ROTATE_L] = 1;
              Player1.CursorX = MouseX - (MouseX > Player1.MouseX);
              Player1.CursorY = MouseY;
            }
            Player1.MouseX = MouseX;
            Player1.MouseY = MouseY;
          }
        } else {
          Player1.MouseX = -1;
          Player1.MouseY = -1;
        }
      }
    }
    FrameAdvance |= FrameAdvanceContinuous;

    UpdatePlayfield(&Player1);

    SDL_RenderCopy(ScreenRenderer, GameBG, NULL, NULL);
    DrawPlayfield(&Player1, (ScreenWidth/2)-(Player1.Width*TILE_W)/2, (ScreenHeight/2)-((Player1.Height-1)*TILE_H)/2);
    DrawText(GameFont, (ScreenWidth/2), 10*ScaleFactor, TEXT_CENTERED, "%i", Player1.Score);

//    DrawPlayfield(&Player1, 8*ScaleFactor, (ScreenHeight/2)-((Player1.Height-1)*TILE_H)/2);
//    DrawPlayfield(&Player1, ScreenWidth-8*ScaleFactor-(Player1.Width*TILE_W), (ScreenHeight/2)-((Player1.Height-1)*TILE_H)/2);

    SDL_RenderPresent(ScreenRenderer);
    if(retraces % 3)
      SDL_Delay(17);
    else
      SDL_Delay(16);

    retraces++;
  }
}

int main(int argc, char *argv[]) {
  // seed the randomizer
  srand(time(NULL));

  PrefPath = SDL_GetPrefPath("Bushytail Software", "NetPuzzleArena");
  GetConfigPath();
  ParseINI(fopen(TempString, "rb"), INIConfigHandler);

  // read parameters
  for(int i=1; i<argc; i++) {
    if(!strcmp(argv[i], "-scale"))
      ScaleFactor = strtol(argv[i+1], NULL, 10);
    if(!strcmp(argv[i], "-noaccel"))
      NoAcceleration = 1;
    if(!strcmp(argv[i], "-keyboard"))
      KeyboardOnly = 1;
    if(!strcmp(argv[i], "-frameadvance"))
      FrameAdvanceMode = 1;
  }

  TILE_W *= ScaleFactor;
  TILE_H *= ScaleFactor;
  ScreenWidth *= ScaleFactor;
  ScreenHeight *= ScaleFactor;

  if(SDL_Init(SDL_INIT_VIDEO) < 0){
    printf("SDL could not initialize video! SDL_Error: %s\n", SDL_GetError());
    return -1;
  }
  if(!KeyboardOnly && SDL_Init(SDL_INIT_JOYSTICK) < 0){
    printf("SDL could not initialize joystick interface! SDL_Error: %s\n", SDL_GetError());
  }

#ifdef ENABLE_AUDIO
  if(SDL_Init(SDL_INIT_AUDIO) < 0){
    printf("SDL could not initialize audio! SDL_Error: %s\n", SDL_GetError());
    return -1;
  }
#endif

  window = SDL_CreateWindow("Net Puzzle Arena", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, ScreenWidth, ScreenHeight, SDL_WINDOW_SHOWN);
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

  ScreenRenderer = SDL_CreateRenderer(window, -1, NoAcceleration?0:SDL_RENDERER_ACCELERATED);
  SDL_RendererInfo RendererInfo;
  SDL_GetRendererInfo(ScreenRenderer, &RendererInfo);
  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Software? %s, Accelerated? %s, Rendererer: %s",
    (RendererInfo.flags&SDL_RENDERER_SOFTWARE)?"Yes":"No", (RendererInfo.flags&SDL_RENDERER_ACCELERATED)?"Yes":"No", RendererInfo.name);

  // set window icon
  GameFont = LoadTexture("data/font.png", 0);
  WindowIcon = SDL_LoadImage("data/icon.png", 0);
  SDL_SetWindowIcon(window, WindowIcon);

  TileSheet = LoadTexture("data/gfx.png", 0);
  GameBG = LoadTexture("data/gamebg.png", 0);
  SDL_SetRenderDrawColor(ScreenRenderer, 0, 0, 0, 255);
  SDL_RenderClear(ScreenRenderer); 
  DrawText(GameFont, ScreenWidth/2, ScreenHeight/2, TEXT_CENTERED, "Initializing");
  SDL_RenderPresent(ScreenRenderer);

  ChatFont = TTF_OpenFont("data/font.ttf", 8*ScaleFactor);
#ifdef ENABLE_AUDIO
  if( (Mix_Init(MIX_INIT_MODPLUG) & MIX_INIT_MODPLUG) != MIX_INIT_MODPLUG ) {
    SDL_MessageBox(SDL_MESSAGEBOX_ERROR, "Error", NULL, "SDL_mixer could not initialize! SDL_mixer Error: %s", Mix_GetError());
  }
  if(Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 1024)==-1) {
    SDL_MessageBox(SDL_MESSAGEBOX_ERROR, "Error", NULL, "Mix_OpenAudio: %s\n", Mix_GetError());
  }
  SampleMove = Mix_LoadWAV("data/move.wav");
  SampleSwap = Mix_LoadWAV("data/swap.wav");
  SampleDrop = Mix_LoadWAV("data/drop.wav");
  SampleDisappear = Mix_LoadWAV("data/disappear.wav");
  SampleCombo = Mix_LoadWAV("data/combo.wav");
  UpdateVolumes();

  memset(&ActiveJoysticks, 0, sizeof(ActiveJoysticks));
  while(!quit)
    switch(ShowTitle()) {
      case 0:
        GameplayStart();
        break;
      case 1:
        break;
      case 2:
        ShowMainOptions();
        break;
      case 3:
        quit = 1;
        break;
    }

#ifdef ENABLE_MUSIC
  Mix_Music *music;
  music = Mix_LoadMUS("data/bgm.xm");
  if(music)
    Mix_PlayMusic(music, -1);
#endif
#endif

  SaveConfigINI();
  // close resources
  TTF_CloseFont(ChatFont);
  if(PrefPath) SDL_free(PrefPath);
#ifdef ENABLE_AUDIO
  Mix_CloseAudio();
  Mix_Quit();
#endif
  IMG_Quit();
  TTF_Quit();
  SDL_Quit();

  return 0;
}
