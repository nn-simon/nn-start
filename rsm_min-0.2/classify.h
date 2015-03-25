#ifndef __CLASSIFY_H
#define __CLASSIFY_H

typedef struct classify {
	int	numclass;
	int	lencase;
	int	numcase;
	double	*w; // lencase x numclass
	double	*data; //numcase x lencase
	int	*labels;
	int	*pred;
	void 	*mem;
} classify_t;

void prediction(classify_t *);
void clssfy_build(classify_t *);
void clssfy_clear(classify_t *);
#endif
