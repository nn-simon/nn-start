#include <math.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "sm_hid.h"
#include "../utils/random.h"
#include <nag.h>
#include <nagh02.h>
#include <nag_string.h>
#include <nag_stdlib.h>

void rsm_hid(const sm_info_t *sm, hid_nag_ip_t *ip, const int *V, double *H, int numcase, int numsample, void *reserved)
{
	int nv, nh, nc, nl;
	double *bh = reserved;
	for (nc = 0; nc < numcase; nc++) {
		fprintf(stdout, "nc %d in rsm_hid\r", nc);
		const int *vecV = V + nc * sm->numvis;
		double *vecH = H + nc * sm->numhid;
		memcpy(bh, sm->w->bh, sm->numhid * sizeof(double));
		for (nh = 0; nh < sm->numhid; nh++) {
			for (nl = 0; nl < sm->len_h2v[nh]; nl++) {
				int id_v = sm->h2v[nh][nl];
				bh[nh] += sm->w->w[id_v][vecV[id_v] * (sm->len_v2h[id_v] + 1) + sm->pos_h2v[nh][nl]];
			}
			vecH[nh] = bh[nh] > 0 ? 1.0 : 0.0;
		}
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
	//ip->options.prob = Nag_MIQP2;
	ip->options.branch_dir = Nag_Branch_InitX;
	ip->options.print_level = Nag_NoPrint;
	strncpy(ip->options.outfile, "ip_out", 80);// 80 is the length of options.outfile 

//	fprintf(stdout, "ip_bb start\n");
	nag_ip_bb(ip->num_variables, ip->num_constraints, ip->a, ip->num_variables, ip->bl, ip->bu, ip->intvar,
		ip->c, NULL, 0, NULLFN, H, &fval, &ip->options, NAGCOMM_NULL, &ip->fail);
	if (ip->fail.code != NE_NOERROR) {
		printf("Error from nag_ip_bb (h02bbc).\n%s\n", ip->fail.message);
		exit(0);
	}
	fprintf(stdout, "ip_bb val:%lf\n", fval);

	nag_ip_free(&ip->options, "", &ip->fail);
	if (ip->fail.code != NE_NOERROR) {
		printf("Error from nag_ip_free (h02xzc).\n%s\n", ip->fail.message);
		exit(0);
	}
}

static int _count(double *h1, double *h2, int dim)
{
	int nd, cnt = 0;
	for (nd = 0; nd < dim; nd++)
		if (h1[nd] > 0.5 && h2[nd] > 0.5)
			cnt++;
		else if(h1[nd] < 0.5 && h2[nd] < 0.5)
			cnt++;
	return cnt;
}

static int _change(double *h, int dim, int times)
{
	int nt;
	for (nt = 0; nt < times; nt++) {
		int pos = (int)(random01() * dim);
		//fprintf(stderr, "nt(%d):%d %d %lf\n", nt, dim, pos, random01());
		h[pos] = h[pos] > 0.5 ? 0.0 : 1.0;
	}
}

void classify_rsm_hid(const sm_info_t *sm, hid_nag_ip_t *ip, const int *V, double *H, int numcase, void *reserved)
{
	//generate H
	classify_t *clssfy = reserved;
	int nc, nl;
	int right = 0;
	double _h[10000];
	//fprintf(stdout, "iter start\n");
	for (nc = 0; nc < numcase; nc++) {
		if (clssfy->pred[nc] == clssfy->labels[nc]) {	
			right++;
			continue;
		}
		fprintf(stdout, "%d %d:", nc, right);
		_change(H + nc * sm->numhid, sm->numhid, sm->numhid / 3);
		memcpy(_h, H + nc * sm->numhid, sm->numhid * sizeof(double));
		_hid_correction(sm, clssfy, V + nc * sm->numvis, H + nc * sm->numhid, clssfy->labels[nc], ip);
		fprintf(stdout, "cnt %d\n", _count(_h, H + nc * sm->numhid, sm->numhid));
	}
	fprintf(stdout, "accurate: %lf\n", (double)right / numcase);
}

typedef struct {
	sm_info_t *sm;
	int *V;
	double *H;
	int numcase;
	int numsample;
	double *bh;
	int id;
} thr_para_t;

static void *thr_fn(void *arg)
{
	thr_para_t *para = arg;
	//test_random();
	//fprintf(stdout, "id(%d):%p %p %p %d %d %p\n", para->id, para->sm, para->V, para->H, para->numcase, para->numsample, para->bh);
	rsm_hid(para->sm, NULL, para->V, para->H, para->numcase, para->numsample, para->bh);
	return (void *) para->id;
}

#define MAX_THREAD_NUM	64
#define MAX_NODE 10000

void rsm_hid_thr(const sm_info_t *sm, hid_nag_ip_t *ip, const int *V, double *H, int numcase, int numsample, void *reserved)
{
	int thr_num = *(int*)reserved;
	if (thr_num > MAX_THREAD_NUM - 1) {
		fprintf(stderr, "notice: thr_num(%d) > MAX_THREAD_NUM(%d) - 1\n", thr_num, MAX_THREAD_NUM);
		thr_num = MAX_THREAD_NUM - 1;
	}
	thr_para_t para[MAX_THREAD_NUM];
	pthread_t thrid[MAX_THREAD_NUM];
	double bh[MAX_THREAD_NUM * MAX_NODE];
	int batchsize = numcase / thr_num;

	int nthr;
	for (nthr = 0; nthr <= thr_num; nthr++) {
		para[nthr].sm = sm;
		para[nthr].V = V + nthr * batchsize * sm->numvis;
		para[nthr].H = H + nthr * batchsize * sm->numhid;
		para[nthr].numcase = batchsize;
		para[nthr].numsample = numsample;
		para[nthr].bh = bh + nthr * sm->numhid;
		para[nthr].id = nthr;
	}
	para[thr_num].numcase = numcase - batchsize * thr_num;
	int err;
	for (nthr = 0; nthr <= thr_num; nthr++) {
		err = pthread_create(&thrid[nthr], NULL, thr_fn, &para[nthr]);
		if (err != 0) {
			fprintf(stderr, "can't create thread %d\n", nthr);
			exit(0);
		}
		//fprintf(stdout, "thr: %d %d %ld\n", nthr, para[nthr].numcase, thrid[nthr]);
	}
	for (nthr = 0; nthr <= thr_num; nthr++) {
		void *tret;
		err = pthread_join(thrid[nthr], &tret);
		if (err != 0) {
			fprintf(stderr, "can't join thread %d\n", nthr);
			exit(0);
		}
		fprintf(stdout, "thread %d exit code %d\r", nthr, (int)tret);
	}
}
