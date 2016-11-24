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
SDL_Window *window = NULL;
SDL_Renderer *ScreenRenderer = NULL;
SDL_Texture *IconTexture = NULL;
SDL_Texture *TileSheet = NULL;
SDL_Texture *GameFont = NULL;
TTF_Font *ChatFont = NULL;
SDL_Surface *IconSurface = NULL;
SDL_Surface *WindowIcon = NULL;
int quit = 0;
int retraces = 0;
struct Playfield Player1;

// config flags
int ScaleFactor = 1, NoAcceleration = 0;

#ifdef ENABLE_AUDIO
  Mix_Chunk *SampleSwap, *SampleDrop, *SampleDisappear, *SampleMove;
#endif

void DrawPlayfield(struct Playfield *P, int DrawX, int DrawY);
void SetGameDefaults(struct Playfield *P, int Game);
void InitPlayfield(struct Playfield *P);
int ShowTitle();
void MainMenu();

int main(int argc, char *argv[]) {
  // read parameters
  for(int i=1; i<argc; i++) {
    if(!strcmp(argv[i], "-scale") && ScaleFactor == 1) {
      ScaleFactor = strtol(argv[i+1], NULL, 10);
      TILE_W *= ScaleFactor;
      TILE_H *= ScaleFactor;
      ScreenWidth *= ScaleFactor;
      ScreenHeight *= ScaleFactor;
    }
    if(!strcmp(argv[i], "-noaccel"))
      NoAcceleration = 1;
  }

  // seed the randomizer
  srand(time(NULL));

  if(SDL_Init(SDL_INIT_VIDEO) < 0){
    printf("SDL could not initialize video! SDL_Error: %s\n", SDL_GetError());
    return -1;
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

  // set window icon
  GameFont = LoadTexture("data/font.png", 0);
  WindowIcon = SDL_LoadImage("data/icon.png", 0);
  SDL_SetWindowIcon(window, WindowIcon);

  TileSheet = LoadTexture("data/gfx.png", 0);
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
  Mix_VolumeChunk(SampleMove, MIX_MAX_VOLUME/2);
  Mix_VolumeChunk(SampleSwap, MIX_MAX_VOLUME/2);
  Mix_VolumeChunk(SampleDrop, MIX_MAX_VOLUME/2);
  Mix_VolumeChunk(SampleDisappear, MIX_MAX_VOLUME/2);

  ShowTitle();

#ifdef ENABLE_MUSIC
  Mix_Music *music;
  music = Mix_LoadMUS("data/bgm.xm");
  if(music)
    Mix_PlayMusic(music, -1);
#endif
#endif

  memset(&Player1, 0, sizeof(struct Playfield));
  Player1.Flags = LIFT_WHILE_CLEARING;
  SetGameDefaults(&Player1, FRENZY);
  InitPlayfield(&Player1);

  SDL_SetRenderDrawColor(ScreenRenderer, 128, 128, 128, 255);
  SDL_RenderClear(ScreenRenderer); 

//  DrawTextTTF(ChatFont, 0, 1*ScaleFactor, TEXT_WHITE|TEXT_WRAPPED, "Is this a pretty decent amount of text for chat messages? With word wrap you've got two lines of text.");
//  DrawTextTTF(ChatFont, 0, ScreenHeight-(1*ScaleFactor), TEXT_FROM_BOTTOM|TEXT_WHITE|TEXT_WRAPPED, "The second player's messages can go on the bottom or something.");

  while(!quit) {
    SDL_Event e;
    while(SDL_PollEvent(&e) != 0) {
      if(e.type == SDL_QUIT)
        quit = 1;
    }

    UpdateKeys(&Player1);

    UpdatePlayfield(&Player1);
    DrawPlayfield(&Player1, (ScreenWidth/2)-(Player1.Width*TILE_W)/2, (ScreenHeight/2)-((Player1.Height-1)*TILE_H)/2);

//    DrawPlayfield(&Player1, 8*ScaleFactor, (ScreenHeight/2)-((Player1.Height-1)*TILE_H)/2);
//    DrawPlayfield(&Player1, ScreenWidth-8*ScaleFactor-(Player1.Width*TILE_W), (ScreenHeight/2)-((Player1.Height-1)*TILE_H)/2);

    SDL_RenderPresent(ScreenRenderer);
    if(retraces % 3)
      SDL_Delay(17);
    else
      SDL_Delay(16);

    retraces++;
  }

  // close resources
  TTF_CloseFont(ChatFont);
	
#ifdef ENABLE_AUDIO
  Mix_CloseAudio();
  Mix_Quit();
#endif
  IMG_Quit();
  TTF_Quit();
  SDL_Quit();

  return 0;
}
