#ifndef __GB_HID_H

#define __GB_HID_H
#include "../model/gb.h"

void gb_sa(const gb_info_t *gb, void *ip, const double *V, double *H, int numcase, int numsample, void *reserved);
void gb_rand(const gb_info_t *gb, void *ip, const double *V, double *H, int numcase, int numsample, void *reserved);

#endif
