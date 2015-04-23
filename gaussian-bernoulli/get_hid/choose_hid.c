#include <stdio.h>
#include "sm_hid.h"

#define MAX_HID_TYPE	16
#define NUM_HID_TYPE	5
#define RESTRICTED_NUM	3
#define MAX_BUF	128

static fun_get_hid get_hid[MAX_HID_TYPE] = {sm_hid_random, sm_hid_sa, sm_hid_sa_thr, rsm_hid, rsm_hid_thr};//, sm_hid_nag, classify_get_hid};

fun_get_hid choose_hid(char *name, int flag)
{
	//if (flag == 0) // restricted network
	//	return get_hid[RESTRICTED_NUM];
	int i;
	char hid_type_name[MAX_HID_TYPE][MAX_BUF] = {"rand", "sa", "sa_thread", "restricted", "restricted_thread", "nag"};
	for (i = 0; i < NUM_HID_TYPE; i++) {
		if (strncmp(hid_type_name[i], name, MAX_BUF) == 0)
			return get_hid[i];
	}
	fprintf(stderr, "for function get_hid, you have following choices:\n\t|");
	for (i = 0; i < NUM_HID_TYPE; i++)
		fprintf(stderr, "%s|", hid_type_name[i]);
	fprintf(stderr, "\nbut your parameter is %s\n", name);
	return NULL;
}
