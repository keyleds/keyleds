/* Keyleds -- Gaming keyboard tool
 * Copyright (C) 2017 Julien Hartmann, juli1.hartmann@gmail.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef KEYLEDSCTL_H
#define KEYLEDSCTL_H

int main_list(int argc, char * argv[]);
int main_info(int argc, char * argv[]);
int main_gkeys(int argc, char * argv[]);
int main_get_leds(int argc, char * argv[]);
int main_set_leds(int argc, char * argv[]);
int main_gamemode(int argc, char * argv[]);

void reset_getopt(int argc, char * const argv[], const char * optstring);

#endif
