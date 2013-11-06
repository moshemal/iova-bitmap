#include <linux/module.h>       /* Needed by all modules */
#include <linux/kernel.h>       /* Needed for KERN_INFO */
#include <linux/init.h>         /* Needed for the macros */
#include "iova-bitmap.h"
#define TEST_PERF "moshe IOVA_BITMAP test: "

#define ASSERT_EQ_N(a,b) if((a) != (b)) return 0

typedef int (*bitmap_test) (struct iova_domain* iovad); 

static int
printDomain(struct iova_domain* iovad){
	int i;
	unsigned long longs = BITS_TO_LONGS(IOVA_DOMAIN_SIZE + 1);
	printk(TEST_PERF"printing the domain (little endien):\n");
	for (i = 0; i < longs; ++i)
	{
		printk(TEST_PERF "word %d: %lX\n",i,iovad->bitmap[i]);
	}
	return 1;
}

static
bitmap_test tests[] = {
  &printDomain
};


static void
runTestSuit(void){
	int i, successes = 0;
	struct iova_domain iovad;
	int numberOfTests = sizeof(tests)/sizeof(bitmap_test);
	char* resString;
  
	init_iova_domain(&iovad, 1UL);

  for (i = 0; i < numberOfTests; ++i)
  {
  	int res;
  		
  	res = tests[i](&iovad);
  	successes += res;
  	if (res)
  	  resString = "success";
  	else
  		resString = "fail";
  	
  	printk(TEST_PERF "test: %d %s\n",i,resString);	
  }
  printk(TEST_PERF "----------------------------------\n");
  printk(TEST_PERF " %d results are success of %d tests\n", successes, numberOfTests);
  printk(TEST_PERF "----------------------------------\n");
}

static int init(void)
{
  printk(TEST_PERF "Loading test iova module..........\n");
  printk(TEST_PERF "----------------------------------\n");
  runTestSuit();
	return 0;
}

static void exit_module(void)
{
	printk(TEST_PERF "Goodbye Mr. Moshe\n");
}


module_init(init);
module_exit(exit_module);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Moshe Malka");
MODULE_DESCRIPTION("iova test");
