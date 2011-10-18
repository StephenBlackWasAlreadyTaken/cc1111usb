#include "global.h"

void sleepMillis(int ms)
{
	int j;
	while (--ms > 0)
	{
		for (j=0; j<SLEEPTIMER;j++); // about 1 millisecond
	}
}

void sleepMicros(int us)
{
    while (--us > 0);
}
