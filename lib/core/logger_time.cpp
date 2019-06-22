//
// Created by Giulio Luzzati on 06/08/18.
//

#include <chrono>
#include <sstream>
#include <iomanip>
#include <sys/time.h>
#include "logger.h"

#define TIME_FMT_STR    "%02d/%02d %02d:%02d:%02d.%03d"
#define TIME_STR_LEN    19

#define USEC_PER_SEC uint64_t(1000000)

/**
 * a structure similar to ANSI struct tm with the following differences:
 *  - tm_usec isn't an ANSI field
 *  - tm_gmtoff isn't an ANSI field (it's a bsdism)
 */
struct time_exp_t {
    /** microseconds past tm_sec */
    int32_t tm_usec;
    /** (0-61) seconds past tm_min */
    int32_t tm_sec;
    /** (0-59) minutes past tm_hour */
    int32_t tm_min;
    /** (0-23) hours past midnight */
    int32_t tm_hour;
    /** (1-31) day of the month */
    int32_t tm_mday;
    /** (0-11) month of the year */
    int32_t tm_mon;
    /** year since 1900 */
    int32_t tm_year;
    /** (0-6) days since sunday */
    int32_t tm_wday;
    /** (0-365) days since jan 1 */
    int32_t tm_yday;
    /** daylight saving time */
    int32_t tm_isdst;
    /** seconds east of UTC */
    int32_t tm_gmtoff;
};

c_time_t time_now(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * USEC_PER_SEC + tv.tv_usec;
}

static void explode_time(time_exp_t *xt, c_time_t t, int use_localtime) {
    struct tm tm;
    time_t tt = (t / USEC_PER_SEC);
    xt->tm_usec = t % USEC_PER_SEC;

#if defined (_POSIX_THREAD_SAFE_FUNCTIONS)
    if (use_localtime)
        localtime_r(&tt, &tm);
    else
        gmtime_r(&tt, &tm);
#else
    if (use_localtime)
        tm = *localtime(&tt);
    else
        tm = *gmtime(&tt);
#endif

    xt->tm_sec = tm.tm_sec;
    xt->tm_min = tm.tm_min;
    xt->tm_hour = tm.tm_hour;
    xt->tm_mday = tm.tm_mday;
    xt->tm_mon = tm.tm_mon;
    xt->tm_year = tm.tm_year;
    xt->tm_wday = tm.tm_wday;
    xt->tm_yday = tm.tm_yday;
    xt->tm_isdst = tm.tm_isdst;
    xt->tm_gmtoff = 0;
}

std::string time_string() {

    time_exp_t te;
    explode_time(&te, time_now(), 1);

    char temp[TIME_STR_LEN];
    sprintf(temp, TIME_FMT_STR,
            te.tm_mon + 1, te.tm_mday, te.tm_hour,
            te.tm_min, te.tm_sec, te.tm_usec / 1000);
    return std::string(temp);
}