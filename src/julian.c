/*-
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2020 The DragonFly Project.  All rights reserved.
 *
 * This code is derived from software contributed to The DragonFly Project
 * by Aaron LI <aly@aaronly.me>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of The DragonFly Project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific, prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Reference:
 * Calendrical Calculations, The Ultimate Edition (4th Edition)
 * by Edward M. Reingold and Nachum Dershowitz
 * 2018, Cambridge University Press
 */

#include <stdbool.h>
#include <stdio.h>

#include "basics.h"
#include "gregorian.h"
#include "julian.h"
#include "utils.h"

/*
 * Fixed date of the start of the Julian calendar.
 * Ref: Sec.(3.1), Eq.(3.2)
 */
static const int epoch = -1;  /* Gregorian: 0, December, 30 */

/*
 * Return true if $year is a leap year on the Julian calendar,
 * otherwise return false.
 * Ref: Sec.(3.1), Eq.(3.1)
 */
bool
julian_leap_year(int year)
{
	int i = (year > 0) ? 0 : 3;
	return (mod(year, 4) == i);
}

/*
 * Calculate the fixed date (RD) equivalent to the Julian date $date.
 * Ref: Sec.(3.1), Eq.(3.3)
 */
int
fixed_from_julian(const struct date *date)
{
	int y = (date->year >= 0) ? date->year : (date->year + 1);
	int rd = ((epoch - 1) + 365 * (y - 1) +
		  div_floor(y - 1, 4) +
		  div_floor(date->month * 367 - 362, 12));
	/* correct for the assumption that February always has 30 days */
	if (date->month <= 2)
		return rd + date->day;
	else if (julian_leap_year(date->year))
		return rd + date->day - 1;
	else
		return rd + date->day - 2;
}

/*
 * Calculate the Julian date (year, month, day) corresponding to the
 * fixed date $rd.
 * Ref: Sec.(3.1), Eq.(3.4)
 */
void
julian_from_fixed(int rd, struct date *date)
{
	int correction, pdays;

	date->year = div_floor(4 * (rd - epoch) + 1464, 1461);
	if (date->year <= 0)
		date->year--;

	struct date d = { date->year, 3, 1 };
	if (rd < fixed_from_julian(&d))
		correction = 0;
	else if (julian_leap_year(date->year))
		correction = 1;
	else
		correction = 2;

	d.month = 1;
	pdays = rd - fixed_from_julian(&d);
	date->month = div_floor(12 * (pdays + correction) + 373, 367);

	d.month = date->month;
	date->day = rd - fixed_from_julian(&d) + 1;
}

/**************************************************************************/

/*
 * Determine the day of week of the Julian date $jdate,
 * using the Zeller's congruence.
 * Ref: https://en.wikipedia.org/wiki/Zeller%27s_congruence
 */
int
julian_dayofweek_from_fixed(const struct date *jdate)
{
	/* January/February counted as month 13/14 of previous year */
	int y = (jdate->month > 2) ? jdate->year : (jdate->year - 1);
	int m = (jdate->month > 2) ? jdate->month : (jdate->month + 12);
	int d = jdate->day;
	int h = d + div_floor(13 * (m+1), 5) + div_floor(y, 4) + y + 5;
		/* 0 = Sat, 1 = Sun, ... , 5 = Thu, 6 = Fri */
	return mod(h - 1, 7);  /* 0 = Sun, 1 = Mon, ... , 5 = Fri, 6 = Sat */
}

/*
 * Print the Julian calendar of the given date $rd.
 */
void
show_julian_calendar(int rd)
{
	static const char *dow_names[] = {
		"Sunday", "Monday", "Tuesday", "Wednesday",
		"Thursday", "Friday", "Saturday",
	};

	struct date gdate, jdate;
	bool leap;
	int gdow, jdow;

	gregorian_from_fixed(rd, &gdate);
	julian_from_fixed(rd, &jdate);
	leap = julian_leap_year(jdate.year);
	gdow = dayofweek_from_fixed(rd);
	jdow = julian_dayofweek_from_fixed(&jdate);

	printf("Gregorian date: %d-%02d-%02d, %s\n",
	       gdate.year, gdate.month, gdate.day, dow_names[gdow]);
	printf("Julian date: %d-%02d-%02d, %s\n",
	       jdate.year, jdate.month, jdate.day, dow_names[jdow]);
	printf("Leap year: %s\n", leap ? "yes" : "no");
}
