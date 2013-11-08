#include <linux/module.h>       /* Needed by all modules */
#include <linux/kernel.h>       /* Needed for KERN_INFO */
#include <linux/init.h>         /* Needed for the macros */
#include "iova-bitmap.h"
#define TEST_PERF "moshe IOVA_BITMAP test: "

#define ASSERT_EQ_N(a,b) do {\
		if((a) != (b)) {\
			printk(TEST_PERF " %lX != %lx\n",a,b);\
			return 0;\
	  }\
  } while(0)

#define ASSERT_TRUE(x) do {\
		if (!(x)) {\
			printk(TEST_PERF "assert true fail\n"); \
			return 0;\
		}\
	} while(0)

#define ASSERT_FALSE(x) do {\
		if ((x)) {\
			printk(TEST_PERF "assert false fail\n"); \
			return 0;\
		}\
	} while(0)

typedef int (*bitmap_test) (struct iova_domain* iovad); 

static void
clearBitMap(struct iova_domain* iovad){
	int i;
	unsigned long longs = BITS_TO_LONGS(IOVA_DOMAIN_SIZE + 1);
	  iovad->bitmap[0] = 1UL;
	for (i = 1; i < longs; ++i) {
		iovad->bitmap[i] = 0UL;
	}
}

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

/* 
*  Testing:
*   bool alloc_iova(struct iova_domain *iovad, unsigned long size,
*                    unsigned long limit_pfn, bool size_aligned, struct iova* new_iova);
*/
static int
test_alloc(struct iova_domain *iovad){
	struct iova new_iova;

  ASSERT_TRUE(alloc_iova(iovad,1UL,-1,1,&new_iova));
  ASSERT_EQ_N(new_iova.pfn_lo, new_iova.pfn_hi);
  ASSERT_EQ_N(new_iova.pfn_lo, (IOVA_DOMAIN_SIZE - 1UL));
  ASSERT_EQ_N(iovad->bitmap[0],0x3UL);

  ASSERT_TRUE(alloc_iova(iovad,3UL,-1,1,&new_iova));
  ASSERT_EQ_N(new_iova.pfn_lo, 120UL);
  ASSERT_EQ_N(new_iova.pfn_hi, 123UL);
  ASSERT_EQ_N(iovad->bitmap[0],0x1e3UL);

  ASSERT_TRUE(alloc_iova(iovad,1UL,-1,1,&new_iova));
  ASSERT_EQ_N(new_iova.pfn_lo, 126UL);
  ASSERT_TRUE(alloc_iova(iovad,1UL,-1,1,&new_iova));
  ASSERT_EQ_N(new_iova.pfn_lo, 125UL);
  ASSERT_TRUE(alloc_iova(iovad,1UL,-1,1,&new_iova));
  ASSERT_EQ_N(new_iova.pfn_lo, 124UL);
  ASSERT_TRUE(alloc_iova(iovad,1UL,-1,1,&new_iova));
  ASSERT_EQ_N(new_iova.pfn_lo, 119UL);
  
  //try to alloc to long range
  ASSERT_EQ_N(iovad->bitmap[0],0x3ffUL);
  ASSERT_FALSE(alloc_iova(iovad,128UL,-1,1,&new_iova));
  ASSERT_EQ_N(iovad->bitmap[0],0x3ffUL);
  ASSERT_EQ_N(iovad->bitmap[1],0UL);
  return 1;  
}


/* 
*  Testing:
*   void free_iova(struct iova_domain *iovad, unsigned long pfn, unsigned long size);
*/
static int
test_free(struct iova_domain *iovad){
  struct iova new_iova;
	
	ASSERT_TRUE(alloc_iova(iovad,128UL,-1,0,&new_iova));
	ASSERT_EQ_N(iovad->bitmap[0], 0xFFFFFFFFFFFFFFFFUL);
	ASSERT_EQ_N(iovad->bitmap[1], 0xFFFFFFFFFFFFFFFFUL);
	ASSERT_EQ_N(iovad->bitmap[2] % 2UL, 1UL);
  
	free_iova(iovad,0,1);
	ASSERT_EQ_N(iovad->bitmap[2] % 2UL, 0UL);
	
	free_iova(iovad,124UL,5UL);
	ASSERT_EQ_N(iovad->bitmap[0], 0xFFFFFFFFFFFFFFE1UL);
	
	free_iova(iovad,1,32);
	ASSERT_EQ_N(iovad->bitmap[1], 0xFFFFFFFFUL);	

	return 1;
}

/* 
*  Testing:
*   void __free_iova(struct iova_domain *iovad, struct iova *iova);
*/
static int
test_free__(struct iova_domain *iovad){
	struct iova new_iova;
	
	ASSERT_TRUE(alloc_iova(iovad,128UL,-1,0,&new_iova));
	ASSERT_EQ_N(iovad->bitmap[0], 0xFFFFFFFFFFFFFFFFUL);
	ASSERT_EQ_N(iovad->bitmap[1], 0xFFFFFFFFFFFFFFFFUL);
	ASSERT_EQ_N(iovad->bitmap[2] % 2UL, 1UL);
  
  new_iova.pfn_lo = 0;
  new_iova.pfn_hi = 0;
	__free_iova(iovad,&new_iova);
	ASSERT_EQ_N(iovad->bitmap[2] % 2UL, 0UL);

	new_iova.pfn_lo = 124UL;
  new_iova.pfn_hi = 128UL;
	__free_iova(iovad,&new_iova);
	ASSERT_EQ_N(iovad->bitmap[0], 0xFFFFFFFFFFFFFFE1UL);
	
	new_iova.pfn_lo = 1UL;
  new_iova.pfn_hi = 32UL;
	__free_iova(iovad,&new_iova);
	ASSERT_EQ_N(iovad->bitmap[1], 0xFFFFFFFFUL);

	return 1;
}

/* 
*  Testing:
*   bool reserve_iova(struct iova_domain *iovad, unsigned long pfn_lo, unsigned long pfn_hi);
*/
static int
test_reserve(struct iova_domain *iovad){
	
	ASSERT_TRUE(reserve_iova(iovad,127UL,127UL));
  ASSERT_EQ_N(iovad->bitmap[0],0x3UL);
  ASSERT_EQ_N(iovad->bitmap[1],0x0UL);
  ASSERT_EQ_N(iovad->bitmap[2] % 2UL, 0UL);
  

  ASSERT_TRUE(reserve_iova(iovad,120UL,123UL));
  ASSERT_EQ_N(iovad->bitmap[0],0x1e3UL);
  ASSERT_EQ_N(iovad->bitmap[1],0x0UL);
  ASSERT_EQ_N(iovad->bitmap[2] % 2UL, 0UL);

  ASSERT_TRUE(reserve_iova(iovad,126UL,126UL));
  ASSERT_TRUE(reserve_iova(iovad,125UL,125UL));
  ASSERT_TRUE(reserve_iova(iovad,124UL,124UL));
  ASSERT_TRUE(reserve_iova(iovad,119UL,119UL));
  ASSERT_EQ_N(iovad->bitmap[0],0x3ffUL);
  ASSERT_EQ_N(iovad->bitmap[1],0x0UL);
  ASSERT_EQ_N(iovad->bitmap[2] % 2UL, 0UL);

  ASSERT_TRUE(reserve_iova(iovad,0UL,64UL));
  ASSERT_EQ_N(iovad->bitmap[0],0x3ffUL);
  ASSERT_EQ_N(iovad->bitmap[1],0xFFFFFFFFFFFFFFFFUL);
  ASSERT_EQ_N(iovad->bitmap[2] % 2UL, 1UL);

  return 1;  
}

/* 
*  Testing:
*   void copy_reserved_iova(struct iova_domain *from, struct iova_domain *to);
*/
static int
test_copy_reserved(struct iova_domain *iovad){
	struct iova_domain new_iovad;
	init_iova_domain(&new_iovad, 1UL);
  
  ASSERT_TRUE(reserve_iova(iovad,127UL,127UL));
  ASSERT_TRUE(reserve_iova(iovad,120UL,123UL));
  
  ASSERT_TRUE(reserve_iova(iovad,119UL,126UL));
  ASSERT_TRUE(reserve_iova(iovad,0UL,64UL));

  ASSERT_TRUE(reserve_iova(iovad,37UL,43UL));
  
  copy_reserved_iova(iovad, &new_iovad);

  ASSERT_EQ_N(iovad->bitmap[0],new_iovad.bitmap[0]);
  ASSERT_EQ_N(iovad->bitmap[1],new_iovad.bitmap[1]);
  ASSERT_EQ_N(iovad->bitmap[2] % 2UL, new_iovad.bitmap[2] % 2UL);


	return 1;
}


static
bitmap_test tests[] = {
  &test_alloc,
  &test_free,
  &test_free__,
  &test_reserve,
  &test_copy_reserved
};

static void
runTestSuit(void){
	int i, successes = 0;
	struct iova_domain iovad;
	int numberOfTests = sizeof(tests)/sizeof(bitmap_test);
	char* resString;	
	init_iova_domain(&iovad, 1UL);

  printk(TEST_PERF "sizeof domain: %lu\n", sizeof(iovad.bitmap));
  if (IOVA_DOMAIN_SIZE != 128){
  	printk(TEST_PERF "this test suport only domain of size 128\neborting\n");
  	return;
  }
  for (i = 0; i < numberOfTests; ++i)
  {
  	int res;
  	printk(TEST_PERF "running test %d\n",i + 1);
  	clearBitMap(&iovad);	
  	res = tests[i](&iovad);
  	successes += res;
  	if (res)
  	  resString = "success";
  	else{
  		printDomain(&iovad);
  		resString = "fail";
  	}
  	printk(TEST_PERF "test: %d %s\n",i + 1,resString);

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
