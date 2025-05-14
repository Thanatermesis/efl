#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */

#include <strings.h>
#include <inttypes.h>
#include <ctype.h>
#include <time.h>

#include "evil_private.h"

/*
 * gettimeofday
 * based on https://github.com/Alexpux/mingw-w64/blob/master/mingw-w64-crt/misc/gettimeofday.c
 * public domain
 */

#define FILETIME_1970 116444736000000000ull /* seconds between 1/1/1601 and 1/1/1970 */

/**
 * @brief Get the current time and timezone information.
 * @param tv Pointer to a timeval struct to store the time. Can be NULL.
 * @param tz Pointer to a timezone struct to store timezone information. Can be NULL.
 * @return Always returns 0 (success).
 *
 * This function mimics the POSIX gettimeofday function.
 * If tv is not NULL, it fills the timeval struct with the current time,
 * converting from Windows FILETIME. It uses GetSystemTimePreciseAsFileTime
 * if available (_WIN32_WINNT >= 0x0602), otherwise GetSystemTimeAsFileTime.
 * If tz is not NULL, it fills the timezone struct with information obtained
 * from GetTimeZoneInformation.
 */
int evil_gettimeofday(struct timeval *tv, struct timezone *tz)
{
   int res = 0; /* evil_gettimeofday always returns 0, success. */
   union
   {
      unsigned long long ns100; /* time since 1 Jan 1601 in 100ns units */
      FILETIME ft;
   } _now;
   TIME_ZONE_INFORMATION time_zone_information;
   DWORD tzi;

   if (tz != NULL)
     {
        tzi = GetTimeZoneInformation(&time_zone_information);
        if (tzi != TIME_ZONE_ID_INVALID)
          {
             tz->tz_minuteswest = time_zone_information.Bias;
             if (tzi == TIME_ZONE_ID_DAYLIGHT)
               tz->tz_dsttime = 1;
             else
               tz->tz_dsttime = 0;
          }
        else
          {
             tz->tz_minuteswest = 0;
             tz->tz_dsttime = 0;
          }
     }

   if (tv != NULL)
     {
#if _WIN32_WINNT < 0x0602
        GetSystemTimeAsFileTime(&_now.ft);
#else
        GetSystemTimePreciseAsFileTime(&_now.ft);
#endif
        _now.ns100 -= FILETIME_1970;	/* 100 nano-seconds since 1-1-1970 */
        tv->tv_sec = _now.ns100 / 10000000ull;	/* seconds since 1-1-1970 */
        tv->tv_usec = (long) (_now.ns100 % 10000000ull) /10; /* nanoseconds */
     }

   return res;
}

/*
 * strptime
 * based on http://cvsweb.netbsd.org/bsdweb.cgi/src/lib/libc/time/strptime.c?rev=HEAD
 * BSD licence
 */

#define TM_YEAR_BASE 1900

/*
 * We do not implement alternate representations. However, we always
 * check whether a given modifier is allowed for a certain conversion.
 */
#define ALT_E			0x01
#define ALT_O			0x02
#define	LEGAL_ALT(x)		{ if (alt_format & ~(x)) return NULL; }


static const char *day[7] =
{
   "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
};

static const char *abday[7] =
{
   "Sun","Mon","Tue","Wed","Thu","Fri","Sat"
};

/**< Array of full month names. */
static const char *mon[12] =
{
   "January", "February", "March", "April", "May", "June", "July",
   "August", "September", "October", "November", "December"
};

static const char *abmon[12] =
{
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

/**< Array of AM/PM indicators. */
static const char *am_pm[2] =
{
   "AM", "PM"
};

/**< String for GMT timezone. */
static char gmt[] = { "GMT" };

#ifdef TM_ZONE
/**< String for UTC timezone. */
static char utc[] = { "UTC" };
#endif

/* RFC-822/RFC-2822 North American timezones */
/**< Array of North American standard time zone names. */
static const char * const nast[5] = {
   "EST",    "CST",    "MST",    "PST",    "\0\0\0" /* Terminator */
};

/**< Array of North American daylight saving time zone names. */
static const char * const nadt[5] = {
   "EDT",    "CDT",    "MDT",    "PDT",    "\0\0\0" /* Terminator */
};

/**
 * @brief Finds a string in a list of strings (case-insensitive).
 * @param bp Pointer to the current position in the input buffer.
 * @param tgt Pointer to an integer where the index of the found string will be stored.
 * @param n1 Pointer to an array of full string names.
 * @param n2 Pointer to an array of abbreviated string names (can be NULL).
 * @param c The number of items in the n1 (and n2 if provided) arrays.
 * @return Pointer to the character in bp after the matched string, or NULL if no match.
 *
 * This function attempts to match the string at bp with strings from n1,
 * and then from n2 if n1 fails and n2 is provided. The match is case-insensitive.
 * If a match is found, *tgt is set to the index of the matched string.
 */
static const unsigned char *
find_string(const unsigned char *bp, int *tgt,
            const char *const *n1, const char *const *n2,
            int c)
{
   size_t len;
   int i;

   /* check full name - then abbreviated ones */
   for (; n1 != NULL; n1 = n2, n2 = NULL)
     {
        for (i = 0; i < c; i++, n1++)
          {
             len = strlen(*n1);
             if (strncasecmp(*n1, (const char *)bp, len) == 0)
               {
                  *tgt = i;
                  return bp + len;
               }
          }
     }

   /* Nothing matched */
   /* Nothing matched */
   return NULL;
}

/**
 * @brief Converts a sequence of digits from a string to an integer.
 * @param buf Pointer to the string buffer containing digits.
 * @param dest Pointer to an integer where the converted number will be stored.
 * @param llim The lower limit (inclusive) for the converted number.
 * @param ulim The upper limit (inclusive) for the converted number.
 * @return Pointer to the character in buf after the consumed digits, or NULL if
 *         no digits are found, or if the number is out of bounds.
 *
 * This function reads digits from buf, converts them to an unsigned integer,
 * and stores the result in *dest. It stops when a non-digit character is
 * encountered or when adding another digit would exceed ulim.
 * It checks if the result is within [llim, ulim].
 */
static const unsigned char *
conv_num(const unsigned char *buf, int *dest, unsigned int llim, unsigned int ulim)
{
   unsigned int result = 0;
   unsigned char ch;

   /* The limit also determines the number of valid digits. */
   unsigned int rulim = ulim;

   ch = *buf;
   if (ch < '0' || ch > '9')
     return NULL;

   do {
      result *= 10;
      result += ch - '0';
      rulim /= 10;
      ch = *++buf;
   } while ((result * 10 <= ulim) && rulim && (ch >= '0') && (ch <= '9'));

   if ((result < llim) || (result > ulim))
     return NULL;

   *dest = result;
   return buf;
}

EVIL_API char *
strptime(const char *buf, const char *fmt, struct tm *tm)
{
   unsigned char c;
   const unsigned char *bp, *ep;
   int alt_format, i, split_year = 0, neg = 0, offs; // offs for timezone offset
   const char *new_fmt;

   bp = (const unsigned char *)buf;

   while (bp != NULL && (c = *fmt++) != '\0')
     {
        /* Clear `alternate' modifier prior to new conversion. */
        alt_format = 0;
        i = 0; /* General purpose integer. */

        /* Eat up white-space in format string and input buffer. */
        if (isspace(c))
          {
             while (isspace(*bp))
               bp++;
             continue;
          }

        if (c != '%')
          goto literal;

     again:
        switch (c = *fmt++)
          {
           case '%':	/* "%%" is converted to "%". */
           literal:
              if (c != *bp++)
                return NULL;
              LEGAL_ALT(0); /* No modifiers allowed for literal characters. */
              continue;

              /*
               * "Alternative" modifiers. These flags modify the behavior of
               * subsequent conversion specifiers.
               */
           case 'E':	/* "%E?" alternative conversion modifier. */
              LEGAL_ALT(0); /* No modifiers allowed for 'E' itself. */
              alt_format |= ALT_E;
              goto again; /* Process next format character. */

           case 'O':	/* "%O?" alternative conversion modifier. */
              LEGAL_ALT(0); /* No modifiers allowed for 'O' itself. */
              alt_format |= ALT_O;
              goto again; /* Process next format character. */

              /*
               * "Complex" conversion rules are implemented by recursively
               * calling strptime with a new format string.
               */
           /* case 'c':	/\* Locale's appropriate date and time representation. *\/ */
           /*    new_fmt = _TIME_LOCALE(loc)->d_t_fmt; */
           /*    goto recurse; */

           case 'D':	/* Equivalent to "%m/%d/%y". */
              new_fmt = "%m/%d/%y";
              LEGAL_ALT(0); /* No modifiers for %D. */
              goto recurse;

           case 'F':	/* Equivalent to "%Y-%m-%d" (ISO 8601 date format). */
              new_fmt = "%Y-%m-%d";
              LEGAL_ALT(0); /* No modifiers for %F. */
              goto recurse;

           case 'R':	/* Equivalent to "%H:%M". */
              new_fmt = "%H:%M";
              LEGAL_ALT(0); /* No modifiers for %R. */
              goto recurse;

           /* case 'r':	/\* Locale's 12-hour clock time format. *\/ */
           /*    new_fmt = _TIME_LOCALE(loc)->t_fmt_ampm; */
           /*    LEGAL_ALT(0); */
           /*    goto recurse; */

           case 'T':	/* Equivalent to "%H:%M:%S". */
              new_fmt = "%H:%M:%S";
              LEGAL_ALT(0); /* No modifiers for %T. */
              goto recurse;

           /* case 'X':	/\* Locale's appropriate time representation. *\/ */
           /*    new_fmt = _TIME_LOCALE(loc)->t_fmt; */
           /*    goto recurse; */

           /* case 'x':	/\* Locale's appropriate date representation. *\/ */
           /*    new_fmt = _TIME_LOCALE(loc)->d_fmt; */
           recurse: /* Label for recursive calls. */
              bp = (const unsigned char *)strptime((const char *)bp,
                                                   new_fmt, tm);
              LEGAL_ALT(ALT_E); /* %E is allowed for some recursive formats (e.g. %Ec). */
              continue;

              /*
               * "Elementary" conversion rules.
               */
           case 'A':	/* Full weekday name. */
           case 'a': /* Abbreviated weekday name. */
              bp = find_string(bp, &tm->tm_wday, day, abday, 7);
              LEGAL_ALT(0); /* No modifiers. */
              continue;

           case 'B':	/* Full month name. */
           case 'b': /* Abbreviated month name. */
           case 'h': /* Equivalent to %b. */
              bp = find_string(bp, &tm->tm_mon, mon, abmon, 12);
              LEGAL_ALT(0); /* No modifiers. */
              continue;

           case 'C':	/* Century (year divided by 100 and truncated to an integer). */
              i = 20; /* Default century. */
              bp = conv_num(bp, &i, 0, 99);

              i = i * 100 - TM_YEAR_BASE;
              if (split_year) /* If tm_year was already set by %y */
                i += tm->tm_year % 100; /* Add the year within the century. */
              split_year = 1; /* Indicates that century is now set. */
              tm->tm_year = i;
              LEGAL_ALT(ALT_E); /* %EC is alternative representation. */
              continue;

           case 'd':	/* Day of the month as a decimal number [01,31]. */
           case 'e': /* Like %d, the day of the month as a decimal number, but a leading zero is replaced by a space. */
              bp = conv_num(bp, &tm->tm_mday, 1, 31);
              LEGAL_ALT(ALT_O); /* %Od or %Oe for alternative numeric symbols. */
              continue;

           case 'k':	/* Hour (24-hour clock) as a decimal number [ 0,23]; single digits are preceded by a blank. */
              LEGAL_ALT(0); /* No modifiers. */
              /* FALLTHROUGH */
           case 'H': /* Hour (24-hour clock) as a decimal number [00,23]. */
              bp = conv_num(bp, &tm->tm_hour, 0, 23);
              LEGAL_ALT(ALT_O); /* %OH for alternative numeric symbols. */
              continue;

           case 'l':	/* Hour (12-hour clock) as a decimal number [ 1,12]; single digits are preceded by a blank. */
              LEGAL_ALT(0); /* No modifiers. */
              /* FALLTHROUGH */
           case 'I': /* Hour (12-hour clock) as a decimal number [01,12]. */
              bp = conv_num(bp, &tm->tm_hour, 1, 12);
              if (tm->tm_hour == 12) /* 12 AM is 00 hours, 12 PM is 12 hours. Handled by %p. */
                tm->tm_hour = 0;
              LEGAL_ALT(ALT_O); /* %OI for alternative numeric symbols. */
              continue;

           case 'j':	/* Day of the year as a decimal number [001,366]. */
              i = 1;
              bp = conv_num(bp, &i, 1, 366);
              tm->tm_yday = i - 1; /* tm_yday is 0-indexed. */
              LEGAL_ALT(0); /* No modifiers. */
              continue;

           case 'M':	/* Minute as a decimal number [00,59]. */
              bp = conv_num(bp, &tm->tm_min, 0, 59);
              LEGAL_ALT(ALT_O); /* %OM for alternative numeric symbols. */
              continue;

           case 'm':	/* Month as a decimal number [01,12]. */
              i = 1;
              bp = conv_num(bp, &i, 1, 12);
              tm->tm_mon = i - 1; /* tm_mon is 0-indexed. */
              LEGAL_ALT(ALT_O); /* %Om for alternative numeric symbols. */
              continue;

           case 'p':	/* Locale's equivalent of either AM or PM. */
              bp = find_string(bp, &i, am_pm, NULL, 2); /* i will be 0 for AM, 1 for PM. */
              if (tm->tm_hour > 11) /* %I sets hour in [0,11] (12 is 0) */
                return NULL; /* Invalid hour for AM/PM. */
              tm->tm_hour += i * 12; /* Add 12 hours if PM. */
              LEGAL_ALT(0); /* No modifiers. */
              continue;

           case 'S':	/* Second as a decimal number [00,60]. (60 is for leap second) */
              bp = conv_num(bp, &tm->tm_sec, 0, 61); /* Range up to 61 for leap seconds. */
              LEGAL_ALT(ALT_O); /* %OS for alternative numeric symbols. */
              continue;

#ifndef TIME_MAX
# ifdef _WIN64
#  define TIME_MAX INT64_MAX
# else
#  define TIME_MAX INT32_MAX
# endif
#endif
           case 's':	/* The number of seconds since the Epoch, 1970-01-01 00:00:00 +0000 (UTC). */
             {
                time_t sse = 0; /* seconds since epoch */
                __int64 rulim = TIME_MAX; /* Remaining upper limit for parsing. */

                if (*bp < '0' || *bp > '9') /* Must start with a digit. */
                  {
                     bp = NULL;
                     continue;
                  }

                /* Parse the number of seconds. */
                do
                  {
                     sse *= 10;
                     sse += *bp++ - '0';
                     rulim /= 10;
                  } while ((sse * 10 <= TIME_MAX) && /* Check for overflow before multiplication. */
                           rulim && *bp >= '0' && *bp <= '9');

                if (sse < 0 || sse > TIME_MAX) /* Final check for overflow or invalid value. */
                  {
                     bp = NULL;
                     continue;
                  }

                /* Convert seconds since epoch to struct tm. */
                if (localtime_r(&sse, tm) == NULL)
                  bp = NULL;
             }
             continue;

           case 'U':	/* Week number of the year as a decimal number [00,53], Sunday as the first day of the week. */
           case 'W':	/* Week number of the year as a decimal number [00,53], Monday as the first day of the week. */
			/*
			 * XXX This is bogus, as we can not assume any valid
			 * information present in the tm structure at this
			 * point to calculate a real value for tm_yday, tm_wday etc.
			 * So, we just parse the number and check its range.
			 * The actual calculation of week number would require mktime.
			 */
              bp = conv_num(bp, &i, 0, 53);
              LEGAL_ALT(ALT_O); /* %OU or %OW for alternative numeric symbols. */
              continue;

           case 'w':	/* Day of the week as a decimal number [0(Sunday),6]. */
              bp = conv_num(bp, &tm->tm_wday, 0, 6);
              LEGAL_ALT(ALT_O); /* %Ow for alternative numeric symbols. */
              continue;

           case 'u':	/* Day of the week as a decimal number [1(Monday),7]. */
              bp = conv_num(bp, &i, 1, 7);
              tm->tm_wday = i % 7; /* Convert to 0 (Sunday) - 6 (Saturday) range. */
              LEGAL_ALT(ALT_O); /* %Ou for alternative numeric symbols. */
              continue;

           case 'g':	/* Like %G, but without century, i.e., with a 2-digit year (00-99). */
              /* This is related to ISO 8601 week date. Parsing it correctly
               * without full date context is complex. Here, we just consume digits.
               */
              bp = conv_num(bp, &i, 0, 99);
              continue;

           case 'G':	/* Year of the ISO week number, which may differ from the calendar year. */
              /* This is related to ISO 8601 week date. Parsing it correctly
               * without full date context is complex. Here, we just consume digits.
               */
              do /* Consume all digits for the year. */
                bp++;
              while (isdigit(*bp));
              continue;

           case 'V':	/* ISO 8601 week number of the year as a decimal number [01,53]. */
              /* This is related to ISO 8601 week date. Parsing it correctly
               * without full date context is complex. Here, we just parse the number.
               */
              bp = conv_num(bp, &i, 0, 53); /* Should be 1-53, but 0 is accepted by some. */
              continue;

           case 'Y':	/* Year as a decimal number including the century. */
              i = TM_YEAR_BASE;	/* Initialize for sanity, though overwritten. */
              bp = conv_num(bp, &i, 0, 9999); /* Year 0-9999. */
              tm->tm_year = i - TM_YEAR_BASE; /* tm_year is years since 1900. */
              LEGAL_ALT(ALT_E); /* %EY for alternative representation. */
              continue;

           case 'y':	/* Year as a decimal number without a century [00,99]. */
			/* LEGAL_ALT(ALT_E | ALT_O); %Ey and %Oy are for alternative representations. */
              bp = conv_num(bp, &i, 0, 99);

              if (split_year) /* If century was already set by %C */
                /* preserve century */
                i += (tm->tm_year / 100) * 100; /* Add the existing century. */
              else {
                 split_year = 1; /* Mark that year (and implicitly century) is now being set. */
                 /* POSIX interpretation: values 69-99 refer to 1969-1999,
                  * and values 00-68 refer to 2000-2068.
                  */
                 if (i <= 68)
                   i = i + 2000 - TM_YEAR_BASE;
                 else
                   i = i + 1900 - TM_YEAR_BASE;
              }
              tm->tm_year = i;
              continue;

           case 'Z': /* Time zone name or abbreviation. */
              tzset(); /* Initialize timezone information from the environment. */
              if (strncmp((const char *)bp, gmt, 3) == 0) { /* Check for "GMT" */
                 tm->tm_isdst = 0;
#ifdef TM_GMTOFF
                 tm->TM_GMTOFF = 0; /* GMT offset is 0. */
#endif
#ifdef TM_ZONE
                 tm->TM_ZONE = gmt; /* Timezone name is "GMT". */
#endif
                 bp += 3;
              }
              else /* Try matching against local timezone names. */
                {
                   ep = find_string(bp, &i,
                                    (const char * const *)tzname, /* System's timezone names (e.g., "PST", "PDT"). */
                                    NULL, 2); /* tzname usually has two entries: standard and daylight. */
                   if (ep != NULL)
                     {
                        tm->tm_isdst = i; /* 0 for standard, 1 for daylight. */
#ifdef TM_GMTOFF
                        /* This is a simplification; 'timezone' variable gives offset for standard time.
                         * A more accurate approach would involve checking 'daylight' variable too.
                         */
                        tm->TM_GMTOFF = -(timezone); /* 'timezone' is seconds west of UTC. */
#endif
#ifdef TM_ZONE
                        tm->TM_ZONE = tzname[i];
#endif
                     }
                   bp = ep; /* Advance buffer pointer past matched timezone name. */
                }
              continue;

           case 'z': /* Offset from UTC in the form +HHMM or -HHMM. */
              /*
               * This implementation handles various timezone offset formats:
               * ISO 8601 formats:
               *   Z (Zulu time/UTC)
               *   [+-]hhmm
               *   [+-]hh:mm
               *   [+-]hh
               * RFC-822/RFC-2822 formats:
               *   UT|GMT
               *   North American timezone abbreviations (EST, CDT, etc.)
               *   Military timezone single letters (A-I, L-M, N-Y, excluding J)
               */
              while (isspace(*bp)) /* Skip leading whitespace. */
                bp++;

              switch (*bp++) /* Check the first character of the offset. */
                {
                 case 'G': /* GMT */
                    if (*bp++ != 'M') return NULL;
                    /*FALLTHROUGH*/
                 case 'U': /* UT */
                    if (*bp++ != 'T') return NULL;
                    /*FALLTHROUGH*/
                 case 'Z': /* Zulu/UTC */
                    tm->tm_isdst = 0;
#ifdef TM_GMTOFF
                    tm->TM_GMTOFF = 0;
#endif
#ifdef TM_ZONE
                    tm->TM_ZONE = utc;
#endif
                    continue;
                 case '+': /* Positive offset. */
                    neg = 0;
                    break;
                 case '-': /* Negative offset. */
                    neg = 1;
                    break;
                 default: /* Potentially a named timezone or military. */
                    --bp; /* Put back the character. */
                    /* Check for North American standard timezones. */
                    ep = find_string(bp, &i, nast, NULL, 4);
                    if (ep != NULL) {
#ifdef TM_GMTOFF
                       tm->TM_GMTOFF = (-5 - i) * 3600; /* Offsets from -5 (EST) to -8 (PST) hours. */
#endif
#ifdef TM_ZONE
                       tm->TM_ZONE = __UNCONST(nast[i]);
#endif
                       bp = ep;
                       continue;
                    }
                    /* Check for North American daylight timezones. */
                    ep = find_string(bp, &i, nadt, NULL, 4);
                    if (ep != NULL)
                      {
                         tm->tm_isdst = 1;
#ifdef TM_GMTOFF
                         tm->TM_GMTOFF = (-4 - i) * 3600; /* Offsets from -4 (EDT) to -7 (PDT) hours. */
#endif
#ifdef TM_ZONE
                         tm->TM_ZONE = __UNCONST(nadt[i]);
#endif
                         bp = ep;
                         continue;
                      }
                    /* Check for military timezones. */
                    if ((*bp >= 'A' && *bp <= 'I') || /* A-I: -1 to -9 hours */
                        (*bp >= 'L' && *bp <= 'M'))   /* L-M: -10 to -12 hours */
                      {
#ifdef TM_GMTOFF
                         if (*bp >= 'A' && *bp <= 'I')
                           tm->TM_GMTOFF = (('A' - 1) - (int)*bp) * 3600;
                         else /* L or M */
                           tm->TM_GMTOFF = ('A' - (int)*bp) * 3600; /* This seems off, should be ('A' - 1 - *bp) or similar logic for L,M */
#endif
#ifdef TM_ZONE
                         /* tm->TM_ZONE = ... ; could store single letter */
#endif
                         bp++;
                         continue;
                      }
                    else if (*bp >= 'N' && *bp <= 'Y') /* N-Y: +1 to +12 hours */
                      {
#ifdef TM_GMTOFF
                        tm->TM_GMTOFF = ((int)*bp - 'M') * 3600;
#endif
#ifdef TM_ZONE
                        /* tm->TM_ZONE = ... ; */
#endif
                        bp++;
                        continue;
                      }
                    return NULL; /* Unrecognized format. */
                }
              /* Parse numeric offset [+-]HHMM or [+-]HH:MM or [+-]HH */
              offs = 0;
              for (i = 0; i < 4; ) /* Read up to 4 digits for HHMM. */
                {
                   if (isdigit(*bp))
                     {
                        offs = offs * 10 + (*bp++ - '0');
                        i++;
                        continue;
                     }
                   if (i == 2 && *bp == ':') /* Allow ':' separator after HH. */
                     {
                        bp++;
                        continue;
                     }
                   break; /* End of digits or invalid char. */
                }
              switch (i) /* Based on number of digits read for offset. */
                {
                 case 2: /* HH format */
                    offs *= 100; /* Convert HH to HH00. */
                    break;
                 case 4: /* HHMM format */
                    /* offs already in HHMM format. Validate minutes. */
                    if ((offs % 100) >= 60) return NULL; /* Invalid minutes. */
                    break;
                 default: /* Invalid number of digits. */
                    return NULL;
                }

              /* Convert HHMM to seconds offset. */
              /* offs / 100 = hours, offs % 100 = minutes. */
              offs = (offs / 100) * 3600 + (offs % 100) * 60;
              if (neg)
                offs = -offs;
              tm->tm_isdst = 0;	/* Explicit offset usually means not DST by local rule, but this is not strictly true. */
#ifdef TM_GMTOFF
              tm->TM_GMTOFF = offs;
#endif
#ifdef TM_ZONE
              tm->TM_ZONE = NULL;	/* No specific zone name for numeric offset. */
#endif
              continue;

              /*
               * Miscellaneous conversions.
               */
           case 'n':	/* A newline character. */
           case 't': /* A tab character. */
              while (isspace(*bp)) /* Consume all whitespace. */
                bp++;
              LEGAL_ALT(0); /* No modifiers. */
              continue;

           default:	/* Unknown or unsupported conversion specifier. */
              return NULL;
          }
     }

   return (char *)bp;
}
