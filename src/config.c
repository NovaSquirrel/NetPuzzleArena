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

enum ConfigTypes {
  CONFIG_STRING,
  CONFIG_BOOLEAN,
  CONFIG_FLOAT,
  CONFIG_INTEGER,
};

char OnlineName[64] = "Guest";
char OnlineServer[150] = "puzzle.novasquirrel.com:28780";

struct ConfigItem {
  char *Group;
  char *Item;
  void *Data;
  char Type;
  short Len;
} ConfigOptions[] = {
  {"Graphics", "Scale",  &ScaleFactor, CONFIG_INTEGER, 0},
  {"Sound", "SFXVolume", &SFXVolume,   CONFIG_FLOAT, 0},
  {"Sound", "BGMVolume", &BGMVolume,   CONFIG_FLOAT, 0},
  {"Online", "Nick",     OnlineName,   CONFIG_STRING, sizeof(OnlineName)},
  {"Online", "Server",   OnlineServer, CONFIG_STRING, sizeof(OnlineServer)},
  {NULL}, // <-- end marker
};

// removes \n or \r if found on the end of a string
void RemoveLineEndings(char *buffer) {
  buffer[strcspn(buffer, "\r\n")] = 0;
}

void SaveConfigINI() {
   GetConfigPath();
   FILE *Output = fopen(TempString, "wb");
   if(!Output) {
     LogMessage("Can't open preferences file for writing\n");
     return;
   }
   int i;
   const char *LastGroup = "";
   for(i=0;ConfigOptions[i].Group;i++) {
     if(strcmp(LastGroup, ConfigOptions[i].Group)) {
       if(*LastGroup)
         fprintf(Output, "\n");
       fprintf(Output, "[%s]\n", ConfigOptions[i].Group);
    }
    LastGroup = ConfigOptions[i].Group;

    if(ConfigOptions[i].Type == CONFIG_STRING) {
      fprintf(Output, "%s=%s\n", ConfigOptions[i].Item, (char*)ConfigOptions[i].Data);
    } else if(ConfigOptions[i].Type == CONFIG_BOOLEAN) {
      fprintf(Output, "%s=%s\n", ConfigOptions[i].Item, (*(int*)ConfigOptions[i].Data)?"yes":"no");
    } else if(ConfigOptions[i].Type == CONFIG_FLOAT){
      fprintf(Output, "%s=%f\n", ConfigOptions[i].Item, *(float*)ConfigOptions[i].Data);
    } else {
      fprintf(Output, "%s=%i\n", ConfigOptions[i].Item, *(int*)ConfigOptions[i].Data);
    } 
  }
  fclose(Output);
}

void INIConfigHandler(const char *Group, const char *Item, const char *Value) {
  int *Int;
  float *Float;
  char *String;

  for(int i=0;ConfigOptions[i].Group!=NULL;i++)
    if(!strcmp(ConfigOptions[i].Group, Group) && !strcmp(ConfigOptions[i].Item, Item)) {
      switch(ConfigOptions[i].Type) {
        case CONFIG_INTEGER:
          Int = ConfigOptions[i].Data;
          if(isdigit(*Value) || *Value == '-')
            *Int = strtol(Value, NULL, 10);
          else
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Item \"[%s] %s\" requires a numeric value\n", Group, Item);
          return;
        case CONFIG_FLOAT:
          Float = ConfigOptions[i].Data;
          if(isdigit(*Value) || *Value == '-')
            *Float = strtod(Value, NULL);
          else
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Item \"[%s] %s\" requires a numeric value\n", Group, Item);
          return;
        case CONFIG_BOOLEAN:
          Int = ConfigOptions[i].Data;
          if(!strcmp(Value, "on") || !strcmp(Value, "yes"))
            *Int = 1;
          else if(!strcmp(Value, "off") || !strcmp(Value, "no"))
            *Int = 0;
          else
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Item \"[%s] %s\" requires a value of on/yes or off/no (You put %s)\n", Group, Item, Value);
          return;
        case CONFIG_STRING:
          String = ConfigOptions[i].Data;
          if(strlen(Value)<ConfigOptions[i].Len)
            strcpy(String, Value);
          else
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Item \"[%s] %s\" requires a smaller string\n", Group, Item);            
          return;
      }
      break;
    }
}

int ParseINI(FILE *File, void (*Handler)(const char *Group, const char *Item, const char *Value)) {
  char Group[512]="", *Item, *Value, Line[512]="", *Poke = NULL;
  if(!File)
    return 0;

  while(!feof(File)) {
    fgets(Line, sizeof(Line), File);
    if(!*Line)
      break;
    RemoveLineEndings(Line);

    if(*Line == ';'); // comment
    else if(*Line == '[') { // group
      Poke = strchr(Line, ']');
      if(Poke) *Poke = 0;
      strcpy(Group, Line+1);
    } else { // item
      Poke = strchr(Line, '=');
      if(Poke) {
        *Poke = 0;
        Item = Line;
        Value = Poke+1;
        Handler(Group, Item, Value);
      }
    }
  }
  fclose(File);
  return 1;
}
