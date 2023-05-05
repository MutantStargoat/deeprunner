#ifndef LOADING_H_
#define LOADING_H_

void loading_start(int nitems);
void loading_additems(int num);
void loading_step(void);	/* step and update (redraw) */
void loading_update(void);	/* update without stepping */

#endif	/* LOADING_H_ */
