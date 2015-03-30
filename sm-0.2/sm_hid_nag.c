#include <math.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "sm_hid.h"
#include <nag.h>
#include <nagh02.h>
#include <nag_string.h>
#include <nag_stdlib.h>

void init_hid_nag_ip_struct(hid_nag_ip_t *ip, int num_var, int num_cons)
{
	ip->num_variables = num_var;
	ip->num_constraints = num_cons;
	ip->bl = (double*) malloc(sizeof(double) * (num_var + num_cons));
	ip->bu = (double*) malloc(sizeof(double) * (num_var + num_cons));
	ip->a = (double*) malloc(sizeof(double) * num_var * num_cons);
	ip->c = (double*) malloc(sizeof(double) * num_var);
	ip->intvar = (Nag_Boolean*) malloc(sizeof(Nag_Boolean) * num_var);
	ip->all_real = (Nag_Boolean*) malloc(sizeof(Nag_Boolean) * num_var);
	ip->h = (double *) malloc(sizeof(double) * num_var * num_var);
	ip->x0 = (double *) malloc(sizeof(double) * num_var);
	if (!(ip->bl && ip->bu && ip->a && ip->c && ip->intvar && ip->h && ip->x0 && ip->all_real)) {
		fprintf(stderr,"malloc in init_hid_nag_ip_struct!\n");
		exit(0);
	}
	int nvar, ncons;
	srand(time(NULL));
	for (nvar = 0; nvar < num_var; nvar++) {
		ip->bl[nvar] = 0.0;
		ip->bu[nvar] = 1.0;
		ip->intvar[nvar] = Nag_TRUE;
		ip->all_real[nvar] = Nag_FALSE;
		ip->x0[nvar] = (double)rand() / RAND_MAX > 0.5 ? 1.0 : 0.0; 
	}
	for (ncons = 0; ncons < num_cons; ncons++) {
		ip->bl[ncons + num_var] = -10000000.0;
		ip->bu[ncons + num_var] = 0.0;
	}
	INIT_FAIL(ip->fail);
}

void free_hid_nag_ip_struct(hid_nag_ip_t *ip)
{
	free(ip->bl);
	free(ip->bu);
	free(ip->a);
	free(ip->c);
	free(ip->intvar);
	free(ip->all_real);
	free(ip->h);
	free(ip->x0);
}

static void _construct_h(double *h, const double *bm_w, const uint8_t *bm_pos, int dim)
{
	int row, col;
	for (row = 0; row < dim; row++) {
		h[row * dim + row] = 0.0;
	}
	for (row = 0; row < dim; row++) {
		for (col = row + 1; col < dim; col++) {
			h[row * dim + col] = (bm_w[row * dim + col] * bm_pos[row * dim + col] + bm_w[row + col * dim] * bm_pos[row + col * dim]) / 2.0;
			h[row + col * dim] = h[row * dim + col];
		}
	}
}

void sm_hid_nag(const sm_info_t *sm, hid_nag_ip_t *ip, const int *V, double *H, int numcase, void *reserved)
{
	int nv, nh, nc, nl;
	double fval;
	_construct_h(ip->h, sm->w->bm_w, sm->bm_pos, sm->numhid);
	for (nl = 0; nl < sm->numhid * sm->numhid; nl++)
		ip->h[nl] *= -1.0; //nag_ip_bb can only get minimized value
	for (nc = 0; nc < numcase; nc++) {
		//printf("nc %d in sm_hid_nag\n", nc);
		const int *vecV = V + nc * sm->numvis;
		double *vecH = H + nc * sm->numhid;
		memcpy(ip->c, sm->w->bh, sm->numhid * sizeof(double));
		for (nh = 0; nh < sm->numhid; nh++) {
			for (nl = 0; nl < sm->len_h2v[nh]; nl++) {
				int id_v = sm->h2v[nh][nl];
				ip->c[nh] += sm->w->w[id_v][vecV[id_v] * (sm->len_v2h[id_v] + 1) + sm->pos_h2v[nh][nl]];
			}
			ip->c[nh] *= -1.0;//nag_ip_bb can only get minimized value
		}
		memcpy(vecH, ip->x0, sm->numhid * sizeof(double));
		nag_ip_init(&ip->options);
		ip->options.prob = Nag_MIQP2;
		ip->options.branch_dir = Nag_Branch_InitX;
		ip->options.print_level = Nag_NoPrint;
		strncpy(ip->options.outfile, "ip_out", 80);// 80 is the length of options.outfile 

		nag_ip_bb(ip->num_variables, ip->num_constraints, ip->a, ip->num_variables, ip->bl, ip->bu, ip->all_real,
			ip->c, ip->h, ip->num_variables, NULLFN, vecH, &fval, &ip->options, NAGCOMM_NULL, &ip->fail);
		if (ip->fail.code != NE_NOERROR) {
			printf("Error from nag_ip_bb (h02bbc).\n%s\n", ip->fail.message);
			exit(0);
		}

		nag_ip_free(&ip->options, "", &ip->fail);
		if (ip->fail.code != NE_NOERROR) {
			printf("Error from nag_ip_free (h02xzc).\n%s\n", ip->fail.message);
			exit(0);
		}
		for (nh < 0; nh < sm->numhid; nh++)
			vecH[nh] = vecH[nh] > 0.5 ? 1.0 : 0.0;
/*
		nag_ip_init(&ip->options);
		ip->options.prob = Nag_MIQP2;
		ip->options.branch_dir = Nag_Branch_InitX;
		ip->options.print_level = Nag_Soln;
		strncpy(ip->options.outfile, "ip_out", 80);// 80 is the length of options.outfile

		nag_ip_bb(ip->num_variables, ip->num_constraints, ip->a, ip->num_variables, ip->bl, ip->bu, ip->intvar,
			ip->c, ip->h, ip->num_variables, NULLFN, vecH, &fval, &ip->options, NAGCOMM_NULL, &ip->fail);
		if (ip->fail.code != NE_NOERROR) {
			printf("Error from nag_ip_bb (h02bbc).\n%s\n", ip->fail.message);
			exit(0);
		}

		nag_ip_free(&ip->options, "", &ip->fail);
		if (ip->fail.code != NE_NOERROR) {
			printf("Error from nag_ip_free (h02xzc).\n%s\n", ip->fail.message);
			exit(0);
		}
*/
	}
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

static void _hid_correction(const sm_info_t *sm, const classify_t *clssfy, const int *V, double *H, int label, hid_nag_ip_t *ip)
{
	int nv, nh, nc, nl;
	memcpy(ip->c, sm->w->bh, sm->numhid * sizeof(double));
	for (nh = 0; nh < sm->numhid; nh++) {
		for (nl = 0; nl < sm->len_h2v[nh]; nl++) {
			int id_v = sm->h2v[nh][nl];
			ip->c[nh] += sm->w->w[id_v][V[id_v] * (sm->len_v2h[id_v] + 1) + sm->pos_h2v[nh][nl]];
		}
		ip->c[nh] *= -1.0;//nag_ip_bb can only get minimized value
	}
	double fval;

	_construct_a(ip->a, clssfy->w, label, clssfy->lencase, clssfy->numclass);
	nag_ip_init(&ip->options);
	ip->options.prob = Nag_MIQP2;
	ip->options.branch_dir = Nag_Branch_InitX;
	ip->options.print_level = Nag_NoPrint;
	strncpy(ip->options.outfile, "ip_out", 80);// 80 is the length of options.outfile 

	nag_ip_bb(ip->num_variables, ip->num_constraints, ip->a, ip->num_variables, ip->bl, ip->bu, ip->intvar,
		ip->c, ip->h, ip->num_variables, NULLFN, H, &fval, &ip->options, NAGCOMM_NULL, &ip->fail);
	if (ip->fail.code != NE_NOERROR) {
		printf("Error from nag_ip_bb (h02bbc).\n%s\n", ip->fail.message);
		exit(0);
	}

	nag_ip_free(&ip->options, "", &ip->fail);
	if (ip->fail.code != NE_NOERROR) {
		printf("Error from nag_ip_free (h02xzc).\n%s\n", ip->fail.message);
		exit(0);
	}
}

void classify_get_hid(const sm_info_t *sm, hid_nag_ip_t *ip, hid_nag_ip_t *ip_constraints, const int *V, double *H, int numcase, void *reserved)
{
	//generate H
	sm_hid_nag(sm, ip, V, H, numcase, NULL);
	classify_t *clssfy = reserved;
	clssfy->data = H;
	prediction(clssfy);
	int nc, nl;
	int right = 0;
	for (nc = 0; nc < numcase; nc++) {
		if (clssfy->pred[nc] == clssfy->labels[nc]) {	
			right++;
			continue;
		}
		_construct_h(ip_constraints->h, sm->w->bm_w, sm->bm_pos, sm->numhid);
		for (nl = 0; nl < sm->numhid * sm->numhid; nl++)
			ip_constraints->h[nl] *= -1.0; //nag_ip_bb can only get minimized value
		_hid_correction(sm, clssfy, V + nc * sm->numvis, H + nc * sm->numhid, clssfy->labels[nc], ip_constraints);
	}
	fprintf(stdout, "accurate: %lf\n", (double)right / numcase);
}
