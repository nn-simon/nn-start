#ifndef __SM_HID_H
#define __SM_HID_H

#include <nag.h>
#include <nagh02.h>
#include "sm.h"
#include "classify.h"

typedef struct {
	int num_variables;
	int num_constraints;
	double *bl;
	double *bu;
	double *a;
	double *c;
	double *h;
	double *x0;
	Nag_Boolean *intvar;
	Nag_Boolean *all_real;
	Nag_H02_Opt options;
	NagError fail;
} hid_nag_ip_t;

void init_hid_nag_ip_struct(hid_nag_ip_t *ip, int num_var, int num_cons);
void free_hid_nag_ip_struct(hid_nag_ip_t *ip);
void classify_get_hid(const sm_info_t *sm, hid_nag_ip_t *ip, hid_nag_ip_t *ip_constraints, const int *V, double *H, int numcase, void *reserved);
void sm_hid_nag(const sm_info_t *sm, hid_nag_ip_t *ip, const int *V, double *H, int numcase, void *reserved);

#endif 
