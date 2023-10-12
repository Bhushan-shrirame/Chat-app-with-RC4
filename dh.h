#include <iostream>

long long multiply(long x, long y, int n)
{
    int toReturn = 0;
    int pow = 1;
    long long val = (x * y) % n;
    return val;
}

int compute(int g, int x, int n)
{
    int toReturn = 1;
    int pow = g;
    while (x)
    {
        int t = x & 1;
        if (t)
        {
            toReturn = multiply(toReturn, pow, n);
        }
        pow = multiply(pow, pow, n);
        x >>= 1;
    }
    return toReturn;
}