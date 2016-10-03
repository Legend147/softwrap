#ifndef __bloom_h
#define __bloom_h

#define BLOOM	1

#define BloomGranularity 2
#define BloomMag 10
#define BloomSize (1<<BloomMag)

class BloomFilter
{
public:
	inline int checkBloom(uint64_t ptr)
	{
		int i = getBloom(ptr);
		return (BloomField[i >> 6]&(1<<(i&63)));
	};

	inline void setBloom(uint64_t ptr)
	{
		int i = getBloom(ptr);
		BloomField[i >> 6] |= (1 << (i&63));
	};

	inline void setBloomRange(uint64_t ptr, size_t size)
	{
		for (uint64_t i = ptr; i <= (ptr+size); i+=(1<<BloomGranularity))
			setBloom(i);
	}

	inline int checkBloomRange(uint64_t ptr, size_t size)
	{
		for (uint64_t i = ptr; i <= (ptr+size); i+=(1<<BloomGranularity))
			if (checkBloom(i))
				return 1;
		return 0;
	}

	inline void clearBloom()
	{
		for (int i = 0; i < BloomSize; i++)
			BloomField[i] = 0;
	}

private:
	unsigned long BloomField[BloomSize];

	inline int getBloom(uint64_t ptr)
	{
		return (ptr >> BloomGranularity) & ((BloomSize << 6) - 1);
	};

};
#endif
