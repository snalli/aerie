#ifndef _CHECK_LOCK_H_AGH178
#define _CHECK_LOCK_H_AGH178

static unsigned int lock_mask = 0xC0000000;
static unsigned int lock_x    = 0x80000000;
static unsigned int lock_s    = 0x40000000;


static int
check_grant_x(CountRegion* region, lock_protocol::LockId lid)
{
	pthread_mutex_lock(&region->count_mutex);
	int x = lid & 0xff;
	if (region->ct[x] & lock_mask == lock_x) {
		fprintf(stderr, "error: server granted exclusive lock %016llx twice\n", lid);
		return -1;
	} else if (region->ct[x] & lock_mask == lock_s) {
		fprintf(stderr, "error: server granted shared lock %016llx exclusively\n", lid);
		return -1;
	}
	region->ct[x] = lock_x;
	pthread_mutex_unlock(&region->count_mutex);

	return 0;
}

static int
check_grant_s(CountRegion* region, lock_protocol::LockId lid)
{
	pthread_mutex_lock(&region->count_mutex);
	int x = lid & 0xff;
	if (region->ct[x] && lock_mask == lock_x) {
		fprintf(stderr, "error: server granted exclusive lock %016llx for sharing\n", lid);
		return -1;
	}
	region->ct[x] = lock_s | ((region->ct[x] & ~lock_mask) + 1);
	pthread_mutex_unlock(&region->count_mutex);

	return 0;
}

static int
check_release(CountRegion* region, lock_protocol::LockId lid)
{
	pthread_mutex_lock(&region->count_mutex);
	int x = lid & 0xff;
	if(region->ct[x] == 0) {
		fprintf(stderr, "error: client released un-held lock %016llx\n",  lid);
		return -1;
	} else if ((region->ct[x] & lock_mask) == lock_x) {
		region->ct[x] = 0;
	} else if ((region->ct[x] & lock_mask) == lock_s) {
		assert(region->ct[x] & ~lock_mask);
		if (((region->ct[x] & ~lock_mask)-1) == 0) {
			region->ct[x] = 0;
		} else {
			region->ct[x] = lock_s | ((region->ct[x] & ~lock_mask) - 1);
		}
	}
	pthread_mutex_unlock(&region->count_mutex);
	return 0;
}


#endif /* _CHECK_LOCK_H_AGH178 */
