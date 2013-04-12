template <typename IntNum> IntNum log2(IntNum n) {
	if (n == 0) { throw; }
	IntNum logv = -1;
	while (n)
	{
		++logv;
		n >>= 1;
	}
	return logv;
}