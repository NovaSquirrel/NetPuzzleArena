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

int MenuKeysPressed(struct Playfield *P) {
  return P->KeyNew[KEY_UP] || P->KeyNew[KEY_DOWN] || P->KeyNew[KEY_LEFT] || P->KeyNew[KEY_RIGHT] || P->KeyNew[KEY_OK] || P->KeyNew[KEY_BACK];
}

void KeymapPathForJoystick(SDL_Joystick *joy) {
  sprintf(TempString, "%skeymap_", PrefPath);
  if(!joy) {
    strcat(TempString, "keyboard.txt");
  } else {
    char *End = strrchr(TempString, 0);
    SDL_JoystickGUID guid = SDL_JoystickGetGUID(joy);
    SDL_JoystickGetGUIDString(guid, End, 100);
    strcat(TempString, ".txt");
  }
}

char *JoypadKeyString(struct JoypadKey *Key, char *Output) {
  strcpy(Output, "?");
  switch(Key->Type) {
    case 'k':
      sprintf(Output, "Keyboard (%s)", SDL_GetScancodeName(Key->Which));
      break;
    case 'a':
      if(Key->Value > 0)
        sprintf(Output, "Axis %i +", Key->Which);
      else
        sprintf(Output, "Axis %i -", Key->Which);
      break;
    case 'b':
      sprintf(Output, "Button %i", Key->Which+1);
      break;
    case 'h':
      if(Key->Value == SDL_HAT_UP)
        sprintf(Output, "Hat %i Up", Key->Which);
      else if(Key->Value == SDL_HAT_DOWN)
        sprintf(Output, "Hat %i Down", Key->Which);
      else if(Key->Value == SDL_HAT_LEFT)
        sprintf(Output, "Hat %i Left", Key->Which);
      else if(Key->Value == SDL_HAT_RIGHT)
        sprintf(Output, "Hat %i Right", Key->Which);
      break;
  }
  return Output;
}

void JoypadConfig(int Id, SDL_Joystick *joy) {
  const char *KeyAssignmentNames[KEY_COUNT] = {"Left", "Down", "Up", "Right", "OK", "Cancel", "Pause", "Action", "Item", "Rotate Counter-clockwise/Swap", "Rotate Clockwise/alternate Swap", "Lift"};
  int SuggestedX[] = {27, 38, 38, 49, 178, 161, 107, 141, 158, 161, 178, 160};
  int SuggestedY[] = {45, 56, 34, 45, 46,  63,  53,  44,  28,  63,  46,  2};
  struct JoypadKey KeyList[KEY_COUNT];

  const char *JoypadName;
  if(joy)
    JoypadName = SDL_JoystickName(joy);
  else
    JoypadName = "Keyboard";

  SDL_Texture *GameController = LoadTexture("data/controller.png", 0);
  int ControllerWidth, ControllerHeight;
  SDL_QueryTexture(GameController, NULL, NULL, &ControllerWidth, &ControllerHeight);

  int CurrentKey = 0;
  int AxeHoldCount = 0;

  struct JoypadKey LastKey = {0, 0, 0};

  while(!quit) {
    struct JoypadKey TheKey = {0, 0, 0};
    SDL_Event e;
    while(SDL_PollEvent(&e) != 0) {
      if(e.type == SDL_QUIT)
        quit = 1;
      else if(e.type == SDL_KEYDOWN && Id == -1) {
        TheKey.Type = 'k';
        TheKey.Which = e.key.keysym.scancode;
        TheKey.Value = 0;
      }
      else if(e.type == SDL_JOYBUTTONDOWN && e.jbutton.which == Id) {
        TheKey.Type = 'b';
        TheKey.Which = e.jbutton.button;
        TheKey.Value = 0;
      }
      else if(e.type == SDL_JOYAXISMOTION && e.jaxis.which == Id) {
        if(abs(e.jaxis.value) > (32767/2)) {
          TheKey.Type = 'a';
          TheKey.Which = e.jaxis.axis;
          TheKey.Value = (e.jaxis.value > 0) ? 1 : -1;
          if(TheKey.Type != LastKey.Type || TheKey.Which != LastKey.Which || TheKey.Value != LastKey.Value)
            AxeHoldCount = 0;
        }
      }
      else if(e.type == SDL_JOYHATMOTION && e.jhat.which == Id) {
        if(e.jhat.value == SDL_HAT_UP || e.jhat.value == SDL_HAT_LEFT || e.jhat.value == SDL_HAT_DOWN || e.jhat.value == SDL_HAT_RIGHT) {
          TheKey.Type = 'h';
          TheKey.Which = e.jhat.hat;
          TheKey.Value = e.jhat.value;
        }
      }
      else if(e.type == SDL_JOYDEVICEREMOVED && e.jdevice.which == Id)
        goto Exit;
    }
    // Keep axis key pressed even if it's not continually moving
    if(!TheKey.Type && LastKey.Type == 'a') {
      int Value = SDL_JoystickGetAxis(joy, LastKey.Which);
      if((LastKey.Value > 0 && Value >  (32767/2))
       ||(LastKey.Value < 0 && Value < -(32767/2)))
        TheKey = LastKey;
      else
        memset(&LastKey, 0, sizeof(LastKey));
    }

    if(TheKey.Type) {
      if(LastKey.Type && LastKey.Which == TheKey.Which && LastKey.Value == TheKey.Value) {
        if(TheKey.Type == 'a')
          AxeHoldCount++;
        if(TheKey.Type != 'a' || AxeHoldCount > 60) {
          KeyList[CurrentKey] = TheKey;
          CurrentKey++;
          if(CurrentKey >= KEY_COUNT)
            break;
          LastKey.Type = 0;
          AxeHoldCount = 0;
        }
      } else {
        LastKey = TheKey;
      }
    }

    const Uint8 *Keyboard = SDL_GetKeyboardState(NULL);
    if(Keyboard[SDL_SCANCODE_ESCAPE])
      quit = 1;

    SDL_SetRenderDrawColor(ScreenRenderer, 255, 255, 255, 255);
    SDL_RenderClear(ScreenRenderer);
    DrawText(GameFont, ScreenWidth/2, 5*8*ScaleFactor, TEXT_WHITE|TEXT_CENTERED, "Set up controller:");
    DrawText(GameFont, ScreenWidth/2, 7*8*ScaleFactor, TEXT_WHITE|TEXT_CENTERED, JoypadName);
    DrawTextTTF(ChatFont, ScreenWidth/2, ScreenHeight/2-8*3*ScaleFactor, TEXT_WHITE|TEXT_CENTERED, "Press \"%s\"", KeyAssignmentNames[CurrentKey]);
    if(LastKey.Type) {
      DrawTextTTF(ChatFont, ScreenWidth/2, ScreenHeight/2-8*2*ScaleFactor, TEXT_WHITE|TEXT_CENTERED, "%s", JoypadKeyString(&LastKey, TempString));
      if(LastKey.Type == 'a')
        DrawTextTTF(ChatFont, ScreenWidth/2, ScreenHeight/2-8*1*ScaleFactor, TEXT_WHITE|TEXT_CENTERED, "Hold to confirm (%i)", AxeHoldCount);
      else
        DrawTextTTF(ChatFont, ScreenWidth/2, ScreenHeight/2-8*1*ScaleFactor, TEXT_WHITE|TEXT_CENTERED, "Press again to confirm");
    }

    int ControllerX = ScreenWidth/2-ControllerWidth/2;
    int ControllerY = ScreenHeight/2+10*ScaleFactor;
    blitfull(GameController, ScreenRenderer, ControllerX, ControllerY);

    SDL_SetRenderDrawColor(ScreenRenderer, 255, 0, 0, 255);
    SDL_Rect Rect = {ControllerX+(SuggestedX[CurrentKey]-3)*ScaleFactor, ControllerY+(SuggestedY[CurrentKey]-3)*ScaleFactor, 6*ScaleFactor, 6*ScaleFactor};
    SDL_RenderFillRect(ScreenRenderer, &Rect);

    SDL_RenderPresent(ScreenRenderer);
    SDL_Delay(17);
  }

  if(CurrentKey >= KEY_COUNT) {
    KeymapPathForJoystick(joy);
    FILE *File = fopen(TempString, "wb");
    if(!File)
      LogMessage("Couldn't open %s to write key config", TempString);
    else {
      fprintf(File, "1\r\n%s\r\n", JoypadName);
      for(int i=0; i<KEY_COUNT; i++)
        fprintf(File, "%c %i %i\r\n", KeyList[i].Type, KeyList[i].Which, KeyList[i].Value);
      fclose(File);
    }
  }
Exit:
  SDL_DestroyTexture(GameController);
}

struct JoypadMapping ReadControllerConfig(FILE *File) {
  char Line[200];
  struct JoypadMapping Out;
  memset(&Out, 0, sizeof(Out));

  fgets(Line, sizeof(Line), File); // read and discard the version number
  fgets(Line, sizeof(Line), File); // read and discard the name

  for(int i=0; i<KEY_COUNT; i++) {
    fgets(Line, sizeof(Line), File);
    char Type = Line[0];
    char *Pointer = Line + 2;
    int Which = strtol(Pointer, &Pointer, 10);
    int Value = strtol(Pointer, &Pointer, 10);
    Out.Keys[i][0].Type = Type;
    Out.Keys[i][0].Which = Which;
    Out.Keys[i][0].Value = Value;
  }
  Out.Active = 1;

  return Out;
}

extern int KeyboardOnly;

void DiscoverJoysticks() {
  if(!KeyboardOnly) {
    // Close all open joysticks
    for(int i=0; i<ACTIVE_JOY_MAX; i++)
      if(ActiveJoysticks[i].Joy && SDL_JoystickGetAttached(ActiveJoysticks[i].Joy))
        SDL_JoystickClose(ActiveJoysticks[i].Joy);
  }
  memset(ActiveJoysticks, 0, sizeof(ActiveJoysticks));

  KeymapPathForJoystick(NULL);
  FILE *Test = fopen(TempString, "rb");
  if(!Test) {
    JoypadConfig(-1, NULL);
    Test = fopen(TempString, "rb");
  }
  if(Test) {
    ActiveJoysticks[0] = ReadControllerConfig(Test);
    ActiveJoysticks[0].Active = 1;
    fclose(Test);
  }

  int JoystickNum = 1;

  if(KeyboardOnly)
    return;

  // look through the list of plugged-in controllers
  for(int i=0; i<SDL_NumJoysticks(); i++) {
    // Open joystick
    SDL_Joystick *joy = SDL_JoystickOpen(i);
    if(joy) {
      KeymapPathForJoystick(joy);
      Test = fopen(TempString, "rb");
      if(!Test) {
        JoypadConfig(SDL_JoystickInstanceID(joy), joy);
        Test = fopen(TempString, "rb");
      }
      if(Test) {
        ActiveJoysticks[JoystickNum] = ReadControllerConfig(Test);
        ActiveJoysticks[JoystickNum].Active = 1;
        ActiveJoysticks[JoystickNum].Joy = joy;
        JoystickNum++;
        fclose(Test);
        if(JoystickNum >= ACTIVE_JOY_MAX) {
          LogMessage("Too many joysticks!");
          return;
        }
      }
    }
  }
}

int ShowTitle() {
  DiscoverJoysticks();

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

    CombinedUpdateKeys(&Player1);
    SDL_Delay(17);
    if(Player1.KeyNew[KEY_OK])
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

void ShowMainOptions() {
  int Choice = 0, Initial = 1;
  Player1.Flags |= NO_AUTO_REPEAT;
  while(!quit) {
    SDL_Event e;
    while(SDL_PollEvent(&e) != 0) {
      if(e.type == SDL_QUIT)
        quit = 1;
    }

    int WhichJoystick = CombinedUpdateKeys(&Player1);

    if(!Initial && !MenuKeysPressed(&Player1))
      continue;
    Initial = 0;

    if(Player1.KeyNew[KEY_UP]) {
      Choice--;
      if(Choice == -1)
        Choice = 4;
    }
    if(Player1.KeyNew[KEY_DOWN]) {
      Choice++;
      if(Choice == 5)
        Choice = 0;
    }

    if(Player1.KeyNew[KEY_OK]) {
      if(Choice == 0) {
        int Id = -1;
        // 0 is keyboard
        if(WhichJoystick != 0) {
          Id = SDL_JoystickInstanceID(ActiveJoysticks[WhichJoystick].Joy);
        }
        JoypadConfig(Id, ActiveJoysticks[WhichJoystick].Joy);
      }
      if(Choice == 1)
        ;
      if(Choice == 2)
        Mix_PlayChannel(-1, SampleCombo, 0);
      if(Choice == 4)
        break;
    }
    if(Player1.KeyNew[KEY_BACK])
      break;

    if(Player1.KeyNew[KEY_LEFT] && Choice == 2) {
      if(SFXVolume > 0) {
        SFXVolume -= 10;
        UpdateVolumes();
        Mix_PlayChannel(-1, SampleCombo, 0);
      }
    }
    if(Player1.KeyNew[KEY_RIGHT] && Choice == 2) {
      if(SFXVolume < 100) {
        SFXVolume += 10;
        UpdateVolumes();
        Mix_PlayChannel(-1, SampleCombo, 0);
      }
    }
    if(Player1.KeyNew[KEY_LEFT] && Choice == 3) {
      if(BGMVolume > 0) {
        BGMVolume -= 10;
      }
    }
    if(Player1.KeyNew[KEY_RIGHT] && Choice == 3) {
      if(BGMVolume < 100) {
        BGMVolume += 10;
      }
    }

    SDL_SetRenderDrawColor(ScreenRenderer, 255, 255, 255, 255);
    SDL_RenderClear(ScreenRenderer);
    int BaseX = ScreenWidth/2 - 50*ScaleFactor;
    DrawText(GameFont, ScreenWidth/2, 2*8*ScaleFactor, TEXT_WHITE|TEXT_CENTERED, "Options");
    DrawTextTTF(ChatFont, BaseX, 2*16*ScaleFactor, TEXT_WHITE, "Key config");
    DrawTextTTF(ChatFont, BaseX, 3*16*ScaleFactor, TEXT_WHITE, "Themes");
    DrawTextTTF(ChatFont, BaseX, 4*16*ScaleFactor, TEXT_WHITE, "SFX Volume: %.0f%%", SFXVolume);
    DrawTextTTF(ChatFont, BaseX, 5*16*ScaleFactor, TEXT_WHITE, "BGM Volume: %.0f%%", BGMVolume);
    DrawTextTTF(ChatFont, BaseX, 6*16*ScaleFactor, TEXT_WHITE, "Back");
    blit(TileSheet, ScreenRenderer, 0, TILE_H, BaseX-20*ScaleFactor, 16*ScaleFactor*Choice+(28*ScaleFactor), TILE_W, TILE_H);
    SDL_RenderPresent(ScreenRenderer);
    SDL_Delay(17);
  }
  Player1.Flags &= ~NO_AUTO_REPEAT;
}

