/*
HappyDays - A Birthdate displayer for the PalmPilot
Copyright (C) 1999-2000 JaeMok Jeong

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef CALENDAR_H_
#define CALENDAR_H_

extern Char convertIndex[163][13];

extern int sol2lun(int syear, int smonth, int sday,
                   DateTimeType* lunar, int *leapyes);
extern int lun2sol(int lyear, int lmonth, int lday, int leapyes,
                   DateTimeType* solar);
extern Int month[12];

#endif
