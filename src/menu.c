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

SDL_Texture *GameLogo = NULL;

int ShowTitle() {
  GameLogo = LoadTexture("data/logo.png", 0);
  int LogoWidth, LogoHeight;
  SDL_QueryTexture(GameLogo, NULL, NULL, &LogoWidth, &LogoHeight);

  SDL_SetRenderDrawColor(ScreenRenderer, 255, 255, 255, 255);
  SDL_RenderClear(ScreenRenderer); 
  int ThirdHeight = ScreenHeight/3;
  blitfull(GameLogo, ScreenRenderer, ScreenWidth/2-LogoWidth/2, ThirdHeight-LogoHeight/2);
  int MenuOptionsBase = ThirdHeight*2;

  // main menu loop
  Player1.Flags |= NO_AUTO_REPEAT;
  int Initial = 1, Choice = 0;
  const char *OptionNames[] = {"Local", "Online", "Options", "Exit"};
  while(!quit) {
    SDL_Event e;
    while(SDL_PollEvent(&e) != 0) {
      if(e.type == SDL_QUIT)
        quit = 1;
    }

    UpdateKeys(&Player1);
    SDL_Delay(17);
    if(Player1.KeyNew[KEY_OK] || Player1.KeyNew[KEY_SWAP])
      break;

    if((!Player1.KeyNew[KEY_UP] && !Player1.KeyNew[KEY_DOWN]) && !Initial)
      continue;
    if(Player1.KeyNew[KEY_UP])
      Choice = (Choice-1)&3;
    if(Player1.KeyNew[KEY_DOWN])
      Choice = (Choice+1)&3;
    Initial = 0;

    for(int i = 0; i < 4; i++)
      DrawText(GameFont, ScreenWidth/2, MenuOptionsBase+10*i*ScaleFactor, (Choice==i)?TEXT_CENTERED:(TEXT_CENTERED|TEXT_WHITE), OptionNames[i]);
    SDL_RenderPresent(ScreenRenderer);
  }
  Player1.Flags &= ~NO_AUTO_REPEAT;

  SDL_DestroyTexture(GameLogo);
  return Choice;
}

void MainMenu() {

}

