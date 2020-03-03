/*
 * Net Puzzle Arena
 *
 * Copyright (C) 2017, 2020 NovaSquirrel
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

pcg32_random_t random_state;

void random_seed() {
	pcg32_srandom_r(&random_state, time(NULL), (intptr_t)&random_state);
}

uint32_t random_raw() {
	return pcg32_random_r(&random_state);
}

uint32_t random(uint32_t bound) {
	return pcg32_boundedrand_r(&random_state, bound);
}

uint32_t random_min_max(uint32_t min, uint32_t max) {
	return pcg32_boundedrand_r(&random_state, max-min)+min;
}
