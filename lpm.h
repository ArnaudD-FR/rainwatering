#ifndef _LPM_H
#define _LPM_H

void timer0_acquire();
void timer0_release();

void lpm_lock_acquire();
void lpm_lock_release();

void lpm_setup();
void lpm_sleep();

#endif // _LPM_H
