/*
** Copyright (C) 2001 Erik de Castro Lopo <erikd AT mega-nerd DOT com>
**
** Permission to use, copy, modify, distribute, and sell this file for any
** purpose is hereby granted without fee, provided that the above copyright
** and this permission notice appear in all copies.  No representations are
** made about the suitability of this software for any purpose.  It is
** provided "as is" without express or implied warranty.
*/

/* Version 1.1 */

/*
** Originally this header provided x86 inline-assembly fallbacks for lrint /
** lrintf on Windows, which do not compile on x64. Every compiler we target
** (MSVC 2015+, GCC, Clang) ships the C99 functions in <math.h>, so the
** fallbacks are gone.
*/

#pragma once

#include <math.h>
