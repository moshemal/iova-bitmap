/*
 * Copyright (c) 2013, Moshe Malka.
 *
 * This file is released under the GPLv2.
 *
 * Author: Moshe Malka <moshemal@cs.technion.ac.il>
 *
 */

#ifndef IOVA_BITMAP_H_
#define IOVA_BITMAP_H_

#include <linux/types.h>
#include <linux/kernel.h>
//#include <linux/bitmap.h>
#include <linux/dma-mapping.h>

/* IO virtual address start page frame number */
#define IOVA_START_PFN		(1)
#define IOVA_DOMAIN_SIZE 128//(1 << 20) //4kb pages 32 bit addresses

/* iova structure */
struct iova {
	unsigned long	pfn_lo; /* IOMMU dish out addr lo */
	unsigned long	pfn_hi; /* IOMMU dish out addr hi */
};

/* holds all the iova translations for a domain */
struct iova_domain {
	spinlock_t	    iova_bitmap_lock;  /* Lock to protect update of bitmap */
	DECLARE_BITMAP(bitmap, IOVA_DOMAIN_SIZE + 1);
	unsigned long	cached_index;      /* Save start point to look for free pfn */
};

void
init_iova_domain(struct iova_domain *iovad, unsigned long pfn_32bit);

bool
alloc_iova(struct iova_domain *iovad, unsigned long size,
			unsigned long limit_pfn, bool size_aligned, struct iova* new_iova);
void
free_iova(struct iova_domain *iovad, unsigned long pfn, unsigned long size);

void
__free_iova(struct iova_domain *iovad, struct iova *iova);

bool
reserve_iova(struct iova_domain *iovad, unsigned long pfn_lo, unsigned long pfn_hi);

void
copy_reserved_iova(struct iova_domain *from, struct iova_domain *to);

static inline void
put_iova_domain(struct iova_domain *iovad){
	return;
}

#endif /* IOVA_BITMAP_H_ */
