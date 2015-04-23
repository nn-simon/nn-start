#include <math.h>
#include <stdio.h>
#include "gb.h"

static double _v_grad_aux(const gb_info_t *gb, gb_w_t *gradw, const double *vecV, const double *vecH)
{
	double cost = 0.0;
	int nv, nl;
	for (nv = 0; nv < gb->numvis; nv++) {
		double avg = 0.0;
		double sigma = gb->w->sigma[nv];
		double sigma2 = sigma * sigma;
		for (nl = 0; nl < gb->numhid; nl++)
			if (gb->vh_pos[nv * gb->numhid + nl])
				avg += gb->w->w[nv * gb->numhid + nl] * vecH[nl];
		avg = avg * gb->w->sigma[nv] + gb->w->bv[nv];
		double tp = vecV[nv] - avg;
		gradw->sigma[nv] += -(sigma2 + tp * (gb->w->bv[nv] - vecV[nv])) / (sigma * sigma2);
		gradw->bv[nv] += tp / sigma2;
		for (nl = 0; nl < gb->numhid; nl++)
			if (gb->vh_pos[nv * gb->numhid + nl] && vecH[nl] > 0.5)
				gradw->w[nv * gb->numhid + nl] += tp / sigma;
		cost -= log(sigma) + tp * tp / sigma2;
	}
	return cost;
}

static double _h_grad_aux(const gb_info_t *gb, gb_w_t *gradw, const double *vecV, const double *vecH)
{
	double cost = 0.0;
	int nh, nl;
	for (nh = 0; nh < gb->numhid; nh++) {
		// bh part
		double e = gb->w->bh[nh];
		// gaussian part
		for (nl = 0; nl < gb->numvis; nl++) {
			if (gb->vh_pos[nl * gb->numhid +nh])
				e += gb->w->w[nl * gb->numhid + nh] * vecV[nl] / gb->w->sigma[nl];
		}
		// bm part
		if (gb->type) {
			for (nl = 0; nl < gb->numhid; nl++) {
				if (vecH[nl] < 0.5) // vecH[nl] == 0.0
					continue;
				if (gb->bm_pos[nh * gb->numhid + nl])
					e += gb->w->bm_w[nh * gb->numhid + nl];
				if (gb->bm_pos[nl * gb->numhid + nh])
					e += gb->w->bm_w[nl * gb->numhid + nh];
			}
		}
		//p(h_j|h_{-j}, V)
		double p = 1.0 / (1.0 + exp(-e));
		double tp = vecH[nh] - p - gb->lambda * e;
		// grad: gaussian part
		for (nl = 0; nl < gb->numvis; nl++) {
			double sigma = gb->w->sigma[nl];
			gradw->sigma[nl] += -tp * gb->w->w[nl * gb->numhid +nh] * vecV[nl] / (sigma * sigma);
			if (gb->vh_pos[nl * gb->numhid +nh])
				gradw->w[nl * gb->numhid + nh] += tp * vecV[nl] / sigma;
		}
		// grad: bm part
		if (gb->type) {
			for (nl = 0; nl < gb->numhid; nl++) {
				if (vecH[nl] < 0.5) // vecH[nl] == 0.0
					continue;
				if (gb->bm_pos[nh * gb->numhid + nl])
					gradw->bm_w[nh * gb->numhid + nl] += tp;
				if (gb->bm_pos[nl * gb->numhid + nh])
					gradw->bm_w[nl * gb->numhid + nh] += tp;
			}
		}
		//grad: bh part
		gradw->bh[nh] += tp;
		//cost += vecH[nh] * log(p) + (1 - vecH[nh]) * log(1 - p);
		if (vecH[nh] < 0.5)
			cost += log(1 - p);
		else
			cost += log(p);
		cost -= gb->lambda * e * e / 2.0;
	}
	return cost;
}

double gb_grad(const gb_info_t *gb, gb_w_t *gradw, const double *V, const double *H, int numcase)
{
	int nl, nc;
	double cost = 0.0;
	for (nc = 0; nc < numcase; nc++) {
		const double *vecV = V + nc * gb->numvis;
		const double *vecH = H + nc * gb->numhid;
		cost += _v_grad_aux(gb, gradw, vecV, vecH);
		cost += _h_grad_aux(gb, gradw, vecV, vecH);
	}

	for (nl = 0; nl < gb->numhid; nl++)
		gradw->bh[nl] /= numcase;
	for (nl = 0; nl < gb->numhid * gb->numhid; nl++)
		gradw->bm_w[nl] /= numcase;
	return cost / numcase;
}
