#ifndef _MERSENNE_H
#define _MERSENNE_H

class Mersenne {
  static const int N = 624;
  unsigned int mt[N];
  int mti;
public:
  Mersenne();
  Mersenne(int seed);
  Mersenne(unsigned int *array, int count);
  Mersenne(const Mersenne &copy);
  Mersenne &operator=(const Mersenne &copy);

  void seed(int s);
  void seed(unsigned int *array, int len);

  unsigned int next32();
  int next31();
  double nextClosed();
  double nextHalfOpen();
  double nextOpen();
  int next(int bound);
};

#endif