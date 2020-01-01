/*-
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 1989, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: head/usr.bin/calendar/day.c 326025 2017-11-20 19:49:47Z pfg $
 */

#include <err.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "calendar.h"

static time_t		time1, time2;
char			dayname[100];
int			year1, year2;


void
settimes(time_t now, int before, int after, int friday,
	 struct tm *tp1, struct tm *tp2)
{
	struct tm tp;

	localtime_r(&now, &tp);

	/* Friday displays Monday's events */
	if (after == 0 && before == 0 && friday != -1)
		after = tp.tm_wday == friday ? 3 : 1;

	time1 = now - SECSPERDAY * before;
	localtime_r(&time1, tp1);
	year1 = 1900 + tp1->tm_year;
	time2 = now + SECSPERDAY * after;
	localtime_r(&time2, tp2);
	year2 = 1900 + tp2->tm_year;

	setlocale(LC_TIME, "C");
	header[5].iov_base = dayname;
	header[5].iov_len = strftime(dayname, sizeof(dayname),
				     "%A, %d %B %Y", tp1);
	setlocale(LC_TIME, "");

	setnnames();
}

/* convert Day[/Month][/Year] into unix time (since 1970)
 * Day: two digits, Month: two digits, Year: digits
 */
time_t
Mktime(char *dp)
{
	time_t t;
	struct tm tm;
	int d, m, y;

	time(&t);
	localtime_r(&t, &tm);

	switch (sscanf(dp, "%d.%d.%d", &d, &m, &y)) {
	case 3:
		if (y > 1900)
			y -= 1900;
		tm.tm_year = y;
		/* FALLTHROUGH */
	case 2:
		tm.tm_mon = m - 1;
		/* FALLTHROUGH */
	case 1:
		tm.tm_mday = d;
	}

#ifdef DEBUG
	fprintf(stderr, "%s: %ld %ld %s\n", __func__,
			(long)mktime(&tm), (long)t, asctime(&tm));
#endif
	return (mktime(&tm));
}
