/*
HappyDays - A Birthdate displayer for the PalmPilot
Copyright (C) 1999-2001 JaeMok Jeong

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

#include <PalmOS.h>
#include "calendar.h"

//  lunayDay : the lunar day to be converted
//  leapyes  : leap yes or no(0/1)
//
int lun2sol(int lyear, int lmonth, int lday, int leapyes,
                  DateTimeType *solar)
{
    int syear, smonth, sday;    /* calculated solar date */
    int m1, m2, n2;
    int i, j, leap;
    long td;
      
    if ( lyear < 1881 || lyear > 2043 ) return -1;
    if ( lmonth < 1 || lmonth > 12 ) return -1;
    if ( lday < 1 || lday > 30 ) return -1;
                                              
    if(leapyes == 1)
    {
        if( convertIndex[lyear-1881][12] < 1 ) return -1;
        if( convertIndex[lyear-1881][lmonth] < 3 ) return -1;
        if( lday > convertIndex[lyear-1881][lmonth]+26 ) return -1;
    }

    j = lmonth-1;

    if( leapyes == 0 )
    {
        for(i = 1; i <= 12; i++) if(convertIndex[lyear-1881][i-1] > 2 ) j++;
        if(lday > convertIndex[lyear-1881][j]+28) return -1;
    }

    m1 = -1;
    td = 0L;

    if (lyear != 1881) {
        m1 = lyear - 1882;
        for (i=0; i<=m1; i++) {
            for (j=0; j<13; j++) td = td + (long)convertIndex[i][j];
            if (convertIndex[i][12]==0 ) td = td + 336L;
            else               td = td + 362L;
        }
    }

    m1++;
    n2 = lmonth-1;
    m2 = -1;

    do {
        m2++;
        if ( convertIndex[m1][m2] > 2 ) {
            td = td + 26L + (long)convertIndex[m1][m2];
            n2++;
        }
        else if (m2==n2) break;
        else td = td + 28L + (long)convertIndex[m1][m2];
    } while(1);

    if (leapyes != 0) td = td + 28L + (long)convertIndex[m1][m2];
    td = td + (long)lday + 29L;
    m1 = 1880;

    do {
        m1++;
        leap = m1%400==0 || (m1%100!=0 && m1%4==0);
        if (leap) m2=366;
        else      m2=365;
        if (td <= (long)m2) break;
        td = td - (long)m2;
    } while(1);

    syear = m1;
    month[1] = m2 - 337;
    m1 = 0;

    do {
        m1++;
        if ( td <= (long)month[m1-1] ) break;
        td = td - (long)month[m1-1];
    } while(1);

    smonth = m1;
    sday = (int)td;
    
    solar->year = syear; solar->month = smonth;
    solar->day =  sday;

    return 0;
}
