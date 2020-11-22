
#ifndef __BOOGA_H
#define __BOOGA_H
/*
 * booga.h -- definitions for the char module
 *
 */
#ifndef BOOGA_MAJOR
#define BOOGA_MAJOR 0   /* dynamic major by default */
#endif

#ifndef BOOGA_NR_DEVS
#define BOOGA_NR_DEVS 4    /* booga0 through booga3 */
#endif
/*
 * Split minors in two parts
 */
#define TYPE(dev)   (MINOR(dev) >> 4)  /* high nibble */
#define NUM(dev)    (MINOR(dev) & 0xf) /* low  nibble */

/*
 * The different configurable parameters
 */
struct booga_stats {
	long int num_open_0;
	long int num_open_1;
	long int num_open_2;
	long int num_open_3;
	long int bytes_read; 
	long int bytes_write; 
	long int boogas_printed;
	long int nekas_printed; 
	long int googoos_printed;
	long int woogas_printed;
	struct semaphore sem;
};
typedef struct booga_stats booga_stats;

/*extern booga_stats Example_Device_Stats;*/

#endif /* __BOOGA_H */
