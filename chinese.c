/*-
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2019 The DragonFly Project.  All rights reserved.
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

/*
 * Implement the Chinese calendar of the 1645 version, established in the
 * second year of the Qīng dynasty.
 *
 * The winter solstice (dōngzhì; 冬至) always occurs during the eleventh
 * month of the year.  The winter-solstice-to-winter-solstice period is
 * called a suì (岁).
 *
 * The leap month of a 13-month winter-solstice-to-winter-solstice period
 * is the first month that does not contain a major solar term --- that is,
 * the first lunar month that is wholly within a solar month.
 */

#include <err.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#include "basics.h"
#include "chinese.h"
#include "gregorian.h"
#include "moon.h"
#include "sun.h"
#include "utils.h"

/*
 * Fixed date of the start of the Chinese calendar.
 * Ref: Sec.(19.3), Eq.(19.15)
 */
static const int epoch = -963099;  /* Gregorian: -2636, February, 15 */

/*
 * Timezone (in fraction of days) of Beijing adopted in Chinese calendar
 * calculations.
 * Ref: Sec.(19.1), Eq.(19.2)
 */
static double
chinese_zone(int rd)
{
	if (gregorian_year_from_fixed(rd) < 1929)
		return (1397.0 / 180.0 / 24.0);
	else
		return (8.0 / 24.0);
}

/*
 * Location and timezone (in fraction of days) of Beijing adopted in Chinese
 * calendar calculations.
 * Ref: Sec.(19.1), Eq.(19.2)
 */
static struct location
chinese_location(int rd)
{
	struct location loc = {
		.latitude = angle2deg(116, 25, 0),
		.longitude = angle2deg(39, 55, 0),
		.elevation = 43.5,
		.zone = chinese_zone(rd),
	};

	return loc;
}

/*
 * The universal time (UT) of (clock) midnight at the start of the fixed date
 * $rd in China.
 * Ref: Sec.(19.1), Eq.(19.7)
 */
static double
midnight_in_china(int rd)
{
	return (double)rd - chinese_zone(rd);
}

/*
 * Calculate the last Chinese major solar term (zhōngqì) in range of [1, 12]
 * before the fixed date $rd.
 * Ref: Sec.(19.1), Eq.(19.1)
 */
int
current_major_solar_term(int rd)
{
	double ut = midnight_in_china(rd);
	double lon = solar_longitude(ut);
	return mod1(2 + div_floor(lon, 30), 12);
}

/*
 * Calculate the fixed date (in China) of winter solstice on or before the
 * given fixed date $rd.
 * Ref: Sec.(19.1), Eq.(19.8)
 */
int
chinese_winter_solstice_onbefore(int rd)
{
	/* longitude of Sun at winter solstice */
	const double winter = 270.0;

	/* approximate the time of winter solstice */
	double t = midnight_in_china(rd + 1);
	double approx = estimate_prior_solar_longitude(winter, t);

	/* search for the date of winter solstice */
	int day = (int)floor(approx) - 1;
	while (winter >= solar_longitude(midnight_in_china(day+1)))
		day++;

	return day;
}

/*
 * Calculate the fixed date (in China) of the first new moon on or after the
 * given fixed date $rd.
 * Ref: Sec.(19.2), Eq.(19.9)
 */
int
chinese_new_moon_onafter(int rd)
{
	double t = new_moon_atafter(midnight_in_china(rd));
	double st = t + chinese_zone(rd);  /* in standard time */

	return (int)floor(st);
}

/*
 * Calculate the fixed date (in China) of the first new moon before the
 * given fixed date $rd.
 * Ref: Sec.(19.2), Eq.(19.10)
 */
int
chinese_new_moon_before(int rd)
{
	double t = new_moon_before(midnight_in_china(rd));
	double st = t + chinese_zone(rd);  /* in standard time */

	return (int)floor(st);
}

/*
 * Determine whether the Chinese lunar month starting on the given fixed
 * date $rd has no major solar term.
 * Ref: Sec.(19.2), Eq.(19.11)
 */
bool
chinese_no_major_solar_term(int rd)
{
	int rd2 = chinese_new_moon_onafter(rd + 1);
	return (current_major_solar_term(rd) ==
		current_major_solar_term(rd2));
}

/*
 * Recursively determine whether there is a Chinese leap month on or
 * after the lunar month starting on fixed date $m1 and at or before
 * the lunar month starting on fixed date $m2.
 * Ref: Sec.(19.2), Eq.(19.12)
 */
bool
chinese_prior_leap_month(int m1, int m2)
{
	int m2_prev = chinese_new_moon_before(m2);
	return ((m2 >= m1) &&
		(chinese_no_major_solar_term(m2) ||
		 chinese_prior_leap_month(m1, m2_prev)));
}

/*
 * Calculate the fixed date of Chinese New Year in suì containing the
 * given date $rd.
 * Ref: Sec.(19.2), Eq.(19.13)
 */
int
chinese_new_year_in_sui(int rd)
{
	/* prior and following winter solstice */
	int s1 = chinese_winter_solstice_onbefore(rd);
	int s2 = chinese_winter_solstice_onbefore(s1 + 370);

	/* month after the 11th month: either 12 or leap 11 */
	int m12 = chinese_new_moon_onafter(s1 + 1);
	/* month after m12: either 12 (or leap 12) or 1 */
	int m13 = chinese_new_moon_onafter(m12 + 1);
	/* the next 11th month */
	int m11_next = chinese_new_moon_before(s2 + 1);

	bool leap_year = lround((m11_next - m12) / mean_synodic_month) == 12;
	if (leap_year && (chinese_no_major_solar_term(m12) ||
			  chinese_no_major_solar_term(m13))) {
		/* either m12 or m13 is a leap month */
		return chinese_new_moon_onafter(m13 + 1);
	} else {
		return m13;
	}
}

/*
 * Calculate the fixed date of Chinese New Year on or before the given
 * fixed date $rd.
 * Ref: Sec.(19.2), Eq.(19.14)
 */
int
chinese_new_year_onbefore(int rd)
{
	int newyear = chinese_new_year_in_sui(rd);
	if (rd >= newyear)
		return newyear;
	else
		return chinese_new_year_in_sui(rd - 180);
}

/*
 * Calculate the fixed date of Chinese New Year in Gregorian year $year.
 * Ref: Sec.(19.6), Eq.(19.26)
 */
int
chinese_new_year(int year)
{
	struct g_date date = { year, 7, 1 };
	int july1 = fixed_from_gregorian(&date);
	return chinese_new_year_onbefore(july1);
}

/*
 * Calculate the Chinese date (cycle, year, month, leap, day) corresponding
 * to the fixed date $rd.
 * Ref: Sec.(19.3), Eq.(19.16)
 */
void
chinese_from_fixed(int rd, struct chinese_date *date)
{
	/* prior and following winter solstice */
	int s1 = chinese_winter_solstice_onbefore(rd);
	int s2 = chinese_winter_solstice_onbefore(s1 + 370);

	/* start of month containing the given date */
	int m = chinese_new_moon_before(rd + 1);
	/* start of the previous month */
	int m_prev = chinese_new_moon_before(m);
	/* month after the last 11th month: either 12 or leap 11 */
	int m12 = chinese_new_moon_onafter(s1 + 1);
	/* the next 11th month */
	int m11_next = chinese_new_moon_before(s2 + 1);

	bool leap_year = lround((m11_next - m12) / mean_synodic_month) == 12;
	int month = (int)lround((m - m12) / mean_synodic_month);
	if (leap_year && chinese_prior_leap_month(m12, m))
		month--;
	month = mod1(month, 12);
	bool leap_month = (leap_year &&
			   chinese_no_major_solar_term(m) &&
			   !chinese_prior_leap_month(m12, m_prev));
	int elapsed_years = (int)floor(1.5 - month/12.0 +
				       (rd - epoch) / mean_tropical_year);

	date->cycle = div_floor(elapsed_years - 1, 60) + 1;
	date->year = mod1(elapsed_years, 60);
	date->month = month;
	date->leap = leap_month;
	date->day = rd - m + 1;
}

/*
 * Calculate the fixed date corresponding to the given Chinese date $date
 * (cycle, year, month, leap, day).
 * Ref: Sec.(19.3), Eq.(19.17)
 */
int
fixed_from_chinese(const struct chinese_date *date)
{
	int midyear = (int)floor(epoch + mean_tropical_year *
				 ((date->cycle - 1) * 60 + date->year - 0.5));
	int newyear = chinese_new_year_onbefore(midyear);

	/* new moon before the given date */
	int newmoon = chinese_new_moon_onafter(
			newyear + (date->month - 1) * 29);
	struct chinese_date date2;
	chinese_from_fixed(newmoon, &date2);
	if (date->month != date2.month || date->leap != date2.leap) {
		/* there was a prior leap month, so get the next month */
		newmoon = chinese_new_moon_onafter(newmoon + 1);
	}

	return newmoon + date->day - 1;
}

/**************************************************************************/

/* celestial stem, tiāngān (天干) */
static const struct stem {
	const char *name;
	const char *zhname;
} STEMS[] = {
	{ "Jiǎ",  "甲" },
	{ "Yǐ",   "乙" },
	{ "Bǐng", "丙" },
	{ "Dīng", "丁" },
	{ "Wù",   "戊" },
	{ "Jǐ",   "己" },
	{ "Gēng", "庚" },
	{ "Xīn",  "辛" },
	{ "Rén",  "壬" },
	{ "Guǐ",  "癸" },
};

/* terrestrial branch, dìzhī (地支) */
static const struct branch {
	const char *name;
	const char *zhname;
	const char *zodiac;
	const char *zhzodiac;
} BRANCHES[] = {
	{ "Zǐ",   "子", "Rat",     "鼠" },
	{ "Chǒu", "丑", "Ox",      "牛" },
	{ "Yín",  "寅", "Tiger",   "虎" },
	{ "Mǎo",  "卯", "Rabbit",  "兔" },
	{ "Chén", "辰", "Dragon",  "龙" },
	{ "Sì",   "巳", "Snake",   "蛇" },
	{ "Wǔ",   "午", "Horse",   "马" },
	{ "Wèi",  "未", "Goat",    "羊" },
	{ "Shēn", "申", "Monkey",  "猴" },
	{ "Yǒu",  "酉", "Rooster", "鸡" },
	{ "Xū",   "戌", "Dog",     "狗" },
	{ "Hài",  "亥", "Pig",     "猪" },
};

/* Chinese names of months and days */
static const char *months[] = {
	"正", "二", "三", "四", "五", "六",
	"七", "八", "九", "十", "冬", "腊",
};
static const char *mdays[] = {
	"初一", "初二", "初三", "初四", "初五", "初六",
	"初七", "初八", "初九", "初十", "十一", "十二",
	"十三", "十四", "十五", "十六", "十七", "十八",
	"十九", "二十", "廿一", "廿二", "廿三", "廿四",
	"廿五", "廿六", "廿七", "廿八", "廿九", "三十",
};

/* 24 major and minor solar terms (节气) */
static const struct solar_term {
	const char	*name;
	const char	*zhname;
	bool		major;  /* whether a major solar term */
	int		longitude;  /* longitude of Sun */
} SOLAR_TERMS[] = {
	{ "Lìchūn",      "立春", false, 315 },
	{ "Yǔshuǐ",      "雨水", true,  330 },
	{ "Jīngzhé",     "惊蛰", false, 345 },
	{ "Chūnfēn",     "春分", true,  0   },
	{ "Qīngmíng",    "清明", false, 15  },
	{ "Gǔyǔ",        "谷雨", true,  30  },
	{ "Lìxià",       "立夏", false, 45  },
	{ "Xiǎomǎn",     "小满", true,  60  },
	{ "Mángzhòng",   "芒种", false, 75  },
	{ "Xiàzhì",      "夏至", true,  90  },
	{ "Xiǎoshǔ",     "小暑", false, 105 },
	{ "Dàshǔ",       "大暑", true,  120 },
	{ "Lìqiū",       "立秋", false, 135 },
	{ "Chǔshǔ",      "处暑", true,  150 },
	{ "Báilù",       "白露", false, 165 },
	{ "Qiūfēn",      "秋分", true,  180 },
	{ "Hánlù",       "寒露", false, 195 },
	{ "Shuāngjiàng", "霜降", true,  210 },
	{ "Lìdōng",      "立冬", false, 225 },
	{ "Xiǎoxuě",     "小雪", true,  240 },
	{ "Dàxuě",       "大雪", false, 255 },
	{ "Dōngzhì",     "冬至", true,  270 },
	{ "Xiǎohán",     "小寒", false, 285 },
	{ "Dàhán",       "大寒", true,  300 },
};

/*
 * Print the Chinese calendar of the year containing the given fixed date
 * $rd.
 */
void
show_chinese_calendar(int rd)
{
	struct chinese_date date;
	chinese_from_fixed(rd, &date);
	struct stem stem = STEMS[mod1(date.year, 10) - 1];
	struct branch branch = BRANCHES[mod1(date.year, 12) - 1];

	struct g_date gdate;
	gregorian_from_fixed(rd, &gdate);
	printf("公历 (Gregorian): %4d-%02d-%02d\n",
	       gdate.year, gdate.month, gdate.day);

	printf("农历: %s%s年 [%s], %s%s月%s\n",
	       stem.zhname, branch.zhname, branch.zhzodiac,
	       date.leap ? "闰" : "", months[date.month - 1],
	       mdays[date.day - 1]);
	printf("Chinese calendar: year %s%s [%s], %smonth %d, day %d\n",
	       stem.name, branch.name, branch.zodiac,
	       date.leap ? "leap " : "", date.month, date.day);

	/* the following Chinese New Year */
	int newyear = chinese_new_year_onbefore(
			chinese_new_year_onbefore(rd) + 370);
	gregorian_from_fixed(newyear, &gdate);
	printf("春节 (Chinese New Year): %4d-%02d-%02d\n",
	       gdate.year, gdate.month, gdate.day);

	/* 1st solar term (Lìchūn) is generally around February 4 */
	gregorian_from_fixed(rd, &gdate);
	gdate.month = 2;
	gdate.day = 1;
	int feb1 = fixed_from_gregorian(&gdate);

	printf("------------------------------------------\n");
	const double zone = chinese_zone(rd);
	const struct solar_term *term;
	double t_term;
	int lambda, n;
	char buf[128];

	for (size_t i = 0; i < nitems(SOLAR_TERMS); i++) {
		term = &SOLAR_TERMS[i];
		lambda = term->longitude;
		t_term = solar_longitude_atafter(lambda, feb1) + zone;
		gregorian_from_fixed((int)floor(t_term), &gdate);
		n = snprintf(buf, sizeof(buf),
			     "%s (%-13s): %3d°, %4d-%02d-%02d ",
			     term->zhname, term->name, lambda,
			     gdate.year, gdate.month, gdate.day);
		n += format_time(buf + n, sizeof(buf) - n, t_term);
		n += snprintf(buf + n, sizeof(buf) - n, "%s", " ");
		n += format_zone(buf + n, sizeof(buf) - n, zone);
		printf("%s\n", buf);
	}
}
