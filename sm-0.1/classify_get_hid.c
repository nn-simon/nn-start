#include <math.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "rsm.h"
#include "classify.h"
#include <nag.h>
#include <nagh02.h>
#include <nag_string.h>
#include <nag_stdlib.h>

typedef struct {
	int num_variables;
	int num_constraints;
	double *bl;
	double *bu;
	double *a;
	double *c;
	Nag_Boolean *intvar;
	Nag_H02_Opt options;
	NagError fail;
} hid_nag_ip_t;

static hid_nag_ip_t *ip;

void init_hid_nag_ip_struct(int num_var, int num_cons)
{
	ip = (hid_nag_ip_t*) malloc(sizeof(hid_nag_ip_t));
	if (ip == NULL) {
		fprintf(stderr,"malloc in init_hid_nag_ip_struct!\n");
		exit(0);
	}
	ip->num_variables = num_var;
	ip->num_constraints = num_cons;
	ip->bl = (double*) malloc(sizeof(double) * (num_var + num_cons));
	ip->bu = (double*) malloc(sizeof(double) * (num_var + num_cons));
	ip->a = (double*) malloc(sizeof(double) * num_var * num_cons);
	ip->c = (double*) malloc(sizeof(double) * num_var);
	ip->intvar = (Nag_Boolean*) malloc(sizeof(Nag_Boolean) * num_var);
	if (!(ip->bl && ip->bu && ip->a && ip->c && ip->intvar)) {
		fprintf(stderr,"malloc in init_hid_nag_ip_struct!\n");
		exit(0);
	}
	int nvar, ncons;
	for (nvar = 0; nvar < num_var; nvar++) {
		ip->bl[nvar] = 0.0;
		ip->bu[nvar] = 1.0;
		ip->intvar[nvar] = Nag_TRUE;
	}
	for (ncons = 0; ncons < num_cons; ncons++) {
		ip->bl[ncons + num_var] = -10000000.0;
		ip->bu[ncons + num_var] = 0.0;
	}
	INIT_FAIL(ip->fail);
	nag_ip_init(&ip->options);
	ip->options.branch_dir = Nag_Branch_InitX;
	ip->options.print_level = Nag_Iter;
	strncpy(ip->options.outfile, "ip_out", 80);// 80 is the length of options.outfile 
}

void free_hid_nag_ip_struct()
{
	free(ip->bl);
	free(ip->bu);
	free(ip->a);
	free(ip->c);
	free(ip->intvar);
	nag_ip_free(&ip->options, "", &ip->fail);
	if (ip->fail.code != NE_NOERROR) {
		printf("Error from nag_ip_free (h02xzc).\n%s\n", ip->fail.message);
		exit(0);
	}	
	free(ip);
}

static void _construct_a(double *a, const double *w, int class, int lencase, int numclass)
{
	// w: lencase x numclass
	// a: num_constraints x num_variables
	// num_constraints = numclass - 1;
	// lencase = num_variables;
	int nclass, nl;
	for (nclass = 0; nclass < class; nclass++)
		for (nl = 0; nl < lencase; nl++)
			a[nclass * lencase + nl] = w[nl * numclass + nclass] - w[nl * numclass + class];
	for (nclass = class + 1; nclass < numclass; nclass++)
		for (nl = 0; nl < lencase; nl++)
			a[(nclass-1) * lencase + nl] = w[nl * numclass + nclass] - w[nl * numclass + class];
}

static void _hid_correction(const double *w, const double *bh, const int *V, double *H, int numvis, int numhid, const int *numclass, int case_id, classify_t *clssfy)
{
	int lenw_cur = 0;
	int nv, nh;
	double fval;

	memcpy(ip->c, bh, sizeof(double) * numhid);
	for (nv = 0; nv < numvis; nv++) {
		for (nh = 0; nh < numhid; nh++)
			ip->c[nh] += w[lenw_cur + V[nv] * (numhid + 1) + nh];
		lenw_cur += numclass[nv] * (numhid + 1);
	}
	for (nh = 0; nh < numhid; nh++)
		ip->c[nh] *= -1.0;

	_construct_a(ip->a, clssfy->w, clssfy->labels[case_id], clssfy->lencase, clssfy->numclass);

	nag_ip_bb(ip->num_variables, ip->num_constraints, ip->a, ip->num_variables, ip->bl, ip->bu, ip->intvar, ip->c, NULL, 0, NULLFN, H, &fval, &ip->options, NAGCOMM_NULL, &ip->fail);
	if (ip->fail.code != NE_NOERROR) {
		printf("Error from nag_ip_bb (h02bbc).\n%s\n", ip->fail.message);
		exit(0);
	}
}

void classify_get_hid(const double *w, const double *bh, const int *V, double *H, int numvis, int numhid, const int *numclass, int numcase, void *reserved)
{
	//generate H
	rsmhid_min(w, bh, V, H, numvis, numhid, numclass, numcase, NULL);
	classify_t *clssfy = reserved;
	clssfy->data = H;
	prediction(clssfy);
	int nc, nv, nh;
	int right = 0;
	for (nc = 0; nc < numcase; nc++) {
		if (clssfy->pred[nc] == clssfy->labels[nc]) {	
			right++;
			continue;
		}
		_hid_correction(w, bh, V + nc * numvis, H + nc * (numhid+1), numvis, numhid, numclass, nc, clssfy);
	}
	fprintf(stdout, "accurate: %lf\n", (double)right / numcase);
}
