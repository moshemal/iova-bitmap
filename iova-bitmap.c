/*
 * Copyright (c) 2013, Moshe Malka.
 *
 * This file is released under the GPLv2.
 *
 * Author: Moshe Malka <moshemal@cs.technion.ac.il>
 *
 */
#include "iova-bitmap.h"//#include <linux/iova-bitmap.h> 
#include <linux/bitmap.h>

static inline
unsigned long toPfn(unsigned long index){
  return IOVA_DOMAIN_SIZE - index;
}

static inline
unsigned long toIndex(unsigned long pfn){
  return IOVA_DOMAIN_SIZE - pfn;
}

void
init_iova_domain(struct iova_domain *iovad, unsigned long pfn_32bit)
{
	spin_lock_init(&iovad->iova_bitmap_lock);
	bitmap_clear(iovad->bitmap,1,IOVA_DOMAIN_SIZE);
	bitmap_set(iovad->bitmap,0,1);
	iovad->cached_index = 1;
}


/* Computes the padding size required, to make the
 * the start address naturally aligned on its size
 */
static int
iova_get_pad_size(int size, unsigned int limit_pfn)
{
	unsigned int pad_size = 0;
	unsigned int order = ilog2(size);

	if (order)
		pad_size = (limit_pfn + 1) % (1 << order);

	return pad_size;
}

/**
 * alloc_iova - allocates an iova
 * @iovad - iova domain in question
 * @size - size of page frames to allocate
 * @limit_pfn - max limit address
 * @size_aligned - set if size_aligned address range is required
 * This function allocates an iova in the range limit_pfn to IOVA_START_PFN
 * looking from limit_pfn instead from IOVA_START_PFN. If the size_aligned
 * flag is set then the allocated address iova->pfn_lo will be naturally
 * aligned on roundup_power_of_two(size).
 */
bool
alloc_iova(struct iova_domain *iovad, unsigned long size,
	unsigned long limit_pfn, bool size_aligned, struct iova* new_iova)
{
	unsigned long index, end,i, pad_size = 0;
	unsigned long min_index = iovad->cached_index;
	unsigned long flags;

	/* If size aligned is set then round the size to
	 * to next power of two.
	 */
	if (size_aligned){

		size = __roundup_pow_of_two(size);
	}

	spin_lock_irqsave(&iovad->iova_bitmap_lock, flags);

again:
	index = bitmap_find_next_zero_area(iovad->bitmap, IOVA_DOMAIN_SIZE + 1, min_index, size, 0);
    
	if (index > IOVA_DOMAIN_SIZE){
		spin_unlock_irqrestore(&iovad->iova_bitmap_lock, flags);
		return 0;
	}
		

	if (size_aligned)
		pad_size = iova_get_pad_size(size, toPfn(index));

	end = index + size + pad_size;
	if (end > IOVA_DOMAIN_SIZE + 1){
		spin_unlock_irqrestore(&iovad->iova_bitmap_lock, flags);
		return 0;
	}

	i = find_next_bit(iovad->bitmap, end, index);
	if (i < end) {
		min_index = i + 1;
		goto again;
	}

	limit_pfn = toPfn(index);

	/* pfn_lo will point to size aligned address if size_aligned is set */
	new_iova->pfn_lo = limit_pfn - (size + pad_size) + 1;
	new_iova->pfn_hi = new_iova->pfn_lo + size - 1;

	index = end - size;
	bitmap_set(iovad->bitmap, index, size);
	spin_unlock_irqrestore(&iovad->iova_bitmap_lock, flags);
	return 1;
}

void
free_iova(struct iova_domain *iovad, unsigned long pfn, unsigned long size){
	unsigned long flags, from;
	
	if (pfn + size > IOVA_DOMAIN_SIZE)
		size =  IOVA_DOMAIN_SIZE - pfn;
    
    from = toIndex(pfn) - size + 1;

	spin_lock_irqsave(&iovad->iova_bitmap_lock, flags);
	bitmap_clear(iovad->bitmap, from, size);
	spin_unlock_irqrestore(&iovad->iova_bitmap_lock, flags);
}

void
__free_iova(struct iova_domain *iovad, struct iova *iova){
	free_iova(iovad, iova->pfn_lo, iova->pfn_hi - iova->pfn_lo + 1);
}

bool
reserve_iova(struct iova_domain *iovad, unsigned long pfn_lo, unsigned long pfn_hi){
	unsigned long flags;
	unsigned long index_lo, size, i;

	
	if (pfn_lo > IOVA_DOMAIN_SIZE)
    	return 1;
    if (pfn_hi >= IOVA_DOMAIN_SIZE)
    	pfn_hi = IOVA_DOMAIN_SIZE - 1;

	index_lo = toIndex(pfn_hi);
    size = pfn_hi - pfn_lo + 1UL;

    spin_lock_irqsave(&iovad->iova_bitmap_lock, flags);
	
	bitmap_set(iovad->bitmap, index_lo, size);
	spin_unlock_irqrestore(&iovad->iova_bitmap_lock, flags);
	return 1;
}

void
copy_reserved_iova(struct iova_domain *from, struct iova_domain *to){
    bitmap_copy(to->bitmap, from->bitmap, IOVA_DOMAIN_SIZE + 1);
}
