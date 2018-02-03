#ifndef CC1310_PA_H_
#define CC1310_PA_H_

#define PA_SLEEP 0
#define PA_RX 1
#define PA_TX 2
void pa_init();
void pa_mode(int mode);

#endif /* CC1310_PA_H_ */