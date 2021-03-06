//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: rtctimer.cpp,v 1.1.2.11 2009/03/09 02:05:18 terminator356 Exp $
//
//  Most code moved from midiseq.cpp by Werner Schweer.
//
//  (C) Copyright 2004 Robert Jonsson (rj@spamatica.se)
//  (C) Copyright -2004 Werner Schweer (werner@seh.de)
//=========================================================

#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
#include <linux/spinlock.h>
#include <linux/mc146818rtc.h>
#else
#include <linux/rtc.h>
#endif
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <poll.h>


#include "rtctimer.h"
#include "globals.h"
#include "gconfig.h"

RtcTimer::RtcTimer()
{
	timerFd = -1;
}

RtcTimer::~RtcTimer()
{
	if (timerFd != -1)
		close(timerFd);
}

signed int RtcTimer::initTimer()
{
	if (TIMER_DEBUG)
		printf("RtcTimer::initTimer()\n");
	if (timerFd != -1)
	{
		fprintf(stderr, "RtcTimer::initTimer(): called on initialised timer!\n");
		return -1;
	}
	doSetuid();

	timerFd = ::open("/dev/rtc", O_RDONLY);
	if (timerFd == -1)
	{
		fprintf(stderr, "fatal error: open /dev/rtc failed: %s\n", strerror(errno));
		fprintf(stderr, "hint: check if 'rtc' kernel module is loaded, or used by something else\n");
		undoSetuid();
		return timerFd;
	}
	if (!setTimerFreq(config.rtcTicks))
	{
		// unable to set timer frequency
		return -1;
	}
	// check if timer really works, start and stop it once.
	if (!startTimer())
	{
		return -1;
	}
	if (!stopTimer())
	{
		return -1;
	}
	return timerFd;
}

unsigned int RtcTimer::setTimerResolution(unsigned int resolution)
{
	if (TIMER_DEBUG)
		printf("RtcTimer::setTimerResolution(%d)\n", resolution);
	/* The RTC can take power-of-two frequencies from 2 to 8196 Hz.
	 * It doesn't really have a resolution as such.
	 */
	return 0;
}

unsigned int RtcTimer::setTimerFreq(unsigned int freq)
{
	int rc = ioctl(timerFd, RTC_IRQP_SET, freq);
	if (rc == -1)
	{
		fprintf(stderr, "RtcTimer::setTimerFreq(): cannot set tick on /dev/rtc: %s\n",
				strerror(errno));
		fprintf(stderr, "  precise timer not available\n");
		return 0;
	}
	return freq;
}

unsigned int RtcTimer::getTimerResolution()
{
	/* The RTC doesn't really work with a set resolution as such.
	 * Not sure how this fits into things yet.
	 */
	return 0;
}

unsigned int RtcTimer::getTimerFreq()
{
	unsigned int freq;
	int rv = ioctl(timerFd, RTC_IRQP_READ, &freq);
	if (rv < 1)
		return 0;
	return freq;
}

bool RtcTimer::startTimer()
{
	if (TIMER_DEBUG)
		printf("RtcTimer::startTimer()\n");
	if (timerFd == -1)
	{
		fprintf(stderr, "RtcTimer::startTimer(): no timer open to start!\n");
		return false;
	}
	if (ioctl(timerFd, RTC_PIE_ON, 0) == -1)
	{
		perror("MidiThread: start: RTC_PIE_ON failed");
		undoSetuid();
		return false;
	}
	return true;
}

bool RtcTimer::stopTimer()
{
	if (TIMER_DEBUG)
		printf("RtcTimer::stopTimer\n");
	if (timerFd != -1)
	{
		ioctl(timerFd, RTC_PIE_OFF, 0);
	}
	else
	{
		fprintf(stderr, "RtcTimer::stopTimer(): no RTC to stop!\n");
		return false;
	}
	return true;
}

unsigned int RtcTimer::getTimerTicks(bool /*printTicks*/)// prevent compiler warning: unused parameter
{
	if (TIMER_DEBUG)
		printf("getTimerTicks()\n");
	unsigned long int nn;
	if (timerFd == -1)
	{
		fprintf(stderr, "RtcTimer::getTimerTicks(): no RTC open to read!\n");
		return 0;
	}
	if (read(timerFd, &nn, sizeof (unsigned long)) != sizeof (unsigned long))
	{
		fprintf(stderr, "RtcTimer::getTimerTicks(): error reading RTC\n");
		return 0;
	}
	return nn;
}
