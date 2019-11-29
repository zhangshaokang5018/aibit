#ifndef __NTP_H
#define __NTP_H

#ifndef mktime
extern inline unsigned long mktime(unsigned int year, unsigned int mon, unsigned int day, unsigned int hour, unsigned int min, unsigned int sec)
{
    if (0 >= (int) (mon -= 2)) {    /* 1..12 -> 11,12,1..10 */
        mon += 12;      /* Puts Feb last since it has leap day */
        year -= 1;
    }

    /* see include/linux/time.h */
    return (((
                (unsigned long) (year/4 - year/100 + year/400 + 367*mon/12 + day) +
                year*365 - 719499
            )*24 + hour /* now have hours */
        )*60 + min /* now have minutes */
    )*60 + sec; /* finally seconds */
}
#endif

#endif
