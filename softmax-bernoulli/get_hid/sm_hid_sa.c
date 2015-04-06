#include "../utils/random.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include "sm_hid.h"
#include <pthread.h>

#define MAX_NODE 10000

static void _simu_anneal(double *h, const double *w, const uint8_t *pos, const double *bh, int dim, int times)
{
	int nd, nl, nt;
	double esum = 0.0, emax = 0.0;
	double e, p;
	double _hmax[MAX_NODE];
	for (nt = 0; nt < times; nt++) {
		nd = (int)(1.0 * random_self() / RAND_SELF_MAX * dim);  
		e = bh[nd];
		for (nl = 0; nl < dim; nl++) {
			if (h[nl] < 0.5) // vecH[nl] == 0.0
				continue;
			if (pos[nd * dim + nl])
				e += w[nd * dim + nl];
			if (pos[nl * dim + nd])
				e += w[nl * dim + nd];
		}
		if (e >= 0.0) {
			if (h[nd] < 0.5) { // 0 -> 1
				esum += e;
				h[nd] = 1.0;
			} else {           // 1 -> 0|1
				p = 1.0 / (1 + exp(e)); //p(h_j = 0 | h_{-j})
				h[nd] = random_self() > RAND_SELF_MAX * p ? 1.0 : 0.0;
				esum += (h[nd] - 1.0) * e;
			}
		} else {
			if (h[nd] < 0.5) { // 0 -> 0|1
				p = 1.0 / (1 + exp(e)); //p(h_j = 0 | h_{-j})
				h[nd] = random_self() > RAND_SELF_MAX * p ? 1.0 : 0.0;
				esum += h[nd] * e;
			} else {           // 1 -> 0
				esum -= e;
				h[nd] = 0.0;
			}
		}
		if (esum > emax) {
			memcpy(_hmax, h, dim * sizeof(double));
			emax = esum;
		}
	}
	//printf("_simu_annel: emax %lf\r", emax);
	memcpy(h, _hmax, dim * sizeof(double));
}

void sm_hid_sa(const sm_info_t *sm, hid_nag_ip_t *ip, const int *V, double *H, int numcase, int numsample, void *reserved)
{
	int nh, nc, ns, nl;
	double *bh = reserved;
	double *x0 = H;
	for (nc = 0; nc < numcase; nc++) {
		fprintf(stdout, "nc %d in sm_hid_sa\r", nc);
		const int *vecV = V + nc * sm->numvis;
		double *vecH = H + nc * sm->numhid;
		memcpy(bh, sm->w->bh, sm->numhid * sizeof(double));
		for (nh = 0; nh < sm->numhid; nh++) {
			for (nl = 0; nl < sm->len_h2v[nh]; nl++) {
				int id_v = sm->h2v[nh][nl];
				bh[nh] += sm->w->w[id_v][vecV[id_v] * (sm->len_v2h[id_v] + 1) + sm->pos_h2v[nh][nl]];
			}
		}
		memcpy(vecH, x0, sm->numhid * sizeof(double));
		//pr_array(stdout, vecH, 1, 18, 'd');
		_simu_anneal(vecH, sm->w->bm_w, sm->bm_pos, bh, sm->numhid, numsample * sm->numhid);
		//x0 = vecH;
		//pr_array(stdout, vecH, 1, 18, 'd');
	}
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
	sm_hid_sa(para->sm, NULL, para->V, para->H, para->numcase, para->numsample, para->bh);
	return (void *) para->id;
}

#define MAX_THREAD_NUM	64

void sm_hid_sa_thr(const sm_info_t *sm, hid_nag_ip_t *ip, const int *V, double *H, int numcase, int numsample, void *reserved)
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
