/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <cstdint>
#include <cstdio>
#include <random>
#include <algorithm>
#include <functional>
#include <ctime>

using random_bytes_engine = std::independent_bits_engine<std::mt19937, CHAR_BIT, uint8_t>;

std::array<uint8_t, 1000> globalArray;
int32_t numOfBytes = 0;

K_MUTEX_DEFINE(mLock);

void producer()
{
	k_sleep(K_MSEC(1000));

	k_mutex_lock(&mLock, K_FOREVER);

	random_bytes_engine rbe;
	rbe.seed(std::time(nullptr));

	int32_t n = rbe() % 50;
	std::generate(globalArray.begin() + numOfBytes,
				globalArray.begin() + n + numOfBytes, std::ref(rbe));

	numOfBytes += n;

	std::printf("DBG: producer produced %d random bytes\n", n);

	k_mutex_unlock(&mLock);
}

void consumer()
{
	k_sleep(K_SECONDS(10));

	k_mutex_lock(&mLock, K_FOREVER);

	std::printf("DBG: consumer triggered, Size of buffer is %d\n", numOfBytes);

	if(numOfBytes >= 512)
	{
		int32_t remBytes = numOfBytes - 512;
		for(int32_t i = numOfBytes; i >= remBytes; i--)
		{
			std::printf("0x%x ", globalArray[i]);
		}
		
		numOfBytes = remBytes;
		std::printf("\nRemaining bytes: %d\n", numOfBytes);
	}
	else 
	{
		std::printf("\tThe number of bytes produced is less than 512, sleeping again\n");
	}

	k_mutex_unlock(&mLock);
}

extern "C" void producerThread(void * p1, void * p2, void * p3)
{
	while(true)
	{
		producer();
	}
}

extern "C" void consumerThread(void *p1, void *p2, void *p3)
{
	while(true)
	{
		consumer();
	}
}

K_KERNEL_THREAD_DEFINE(th1, 500, producerThread, nullptr, nullptr, nullptr, 4, 0, 0);
K_KERNEL_THREAD_DEFINE(th2, 500, consumerThread, nullptr, nullptr, nullptr, 1, 0, 0);

int main(void)
{
	return 0;
}