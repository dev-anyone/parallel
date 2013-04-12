template <typename IntNum> IntNum uplog2(IntNum n) { // Calculates the logarithm, base 2, of a (integral) number n, rounding up (counting the number of binary digits).
	if (n == 0) { throw; }
	IntNum logv = -1;
	while (n)
	{
		++logv;
		n >>= 1;
	}
	return logv;
}
