/*-
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2019-2020 The DragonFly Project.  All rights reserved.
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
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#ifndef nitems
#define nitems(x)	(sizeof(x) / sizeof((x)[0]))
#endif

static inline bool
string_eqn(const char *s1, const char *s2)
{
	return (s1 && s2 && strncmp(s1, s2, strlen(s2)) == 0);
}

static inline void
swap(int *a, int *b)
{
	int tmp = *a;
	*a = *b;
	*b = tmp;
}


int	div_floor(int x, int y);

int	mod(int x, int y);
double	mod_f(double x, double y);
int	mod1(int x, int y);
int	mod3(int x, int a, int b);
double	mod3_f(double x, double a, double b);

double	poly(double x, const double *coefs, size_t n);

double	sin_deg(double deg);
double	cos_deg(double deg);
double	tan_deg(double deg);
double	arcsin_deg(double x);
double	arccos_deg(double x);
double	arctan_deg(double y, double x);

double	angle2deg(int deg, int min, double sec);

double	invert_angular(double (*f)(double), double y, double a, double b);

void *	xcalloc(size_t number, size_t size);
char *	xstrdup(const char *str);

size_t	count_char(const char *s, int ch);

struct node;
struct node *	list_newnode(char *name, void *data);
struct node *	list_addfront(struct node *listp, struct node *newp);
bool		list_lookup(struct node *listp, const char *name,
			    int (*cmp)(const char *, const char *),
			    void **data_out);
void		list_freeall(struct node *listp, void (*free_name)(void *),
			     void (*free_data)(void *));

#endif