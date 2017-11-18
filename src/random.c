/*
 * Net Puzzle Arena
 *
 * Copyright (C) 2017 NovaSquirrel
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
#include "pcg_variants.h"
#include <stdint.h>

pcg32_random_t RandomState;

void RandomSeed() {
  pcg32_srandom_r(&RandomState, time(NULL), (intptr_t)&RandomState);
}

uint32_t RandomRaw() {
  return pcg32_random_r(&RandomState);
}

uint32_t Random(uint32_t Bound) {
  return pcg32_boundedrand_r(&RandomState, Bound);
}

uint32_t RandomMinMax(uint32_t Min, uint32_t Max) {
  return pcg32_boundedrand_r(&RandomState, Max-Min)+Min;
}
