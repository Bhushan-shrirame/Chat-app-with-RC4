#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#define SIZE 128

std::vector<int> getVec(int key)
{
    std::vector<int> k;
    while (key)
    {
        int t = key % 10;
        key = key / 10;
        k.push_back(t);
    }
    std::reverse(k.begin(), k.end());
    return k;
}

std::vector<int> strToVec(std::string msg)
{
    std::vector<int> toReturn;
    for (char c : msg)
    {
        toReturn.push_back((int)(c));
    }
    return toReturn;
}

std::string vecToStr(std::vector<int> msg)
{
    std::string toReturn;
    for (int i : msg)
    {
        toReturn += (char)i;
    }
    return toReturn;
}

std::string encryptDecrypt(std::string msg, int k)
{
    int K[SIZE], S[SIZE];
    std::vector<int> key = getVec(k);
    std::vector<int> message = strToVec(msg);
    std::vector<int> keyStream, eD;

    int n = key.size();
    for (int i = 0; i < SIZE; i++)
    {
        S[i] = i;
        K[i] = key[i % n];
    }

    // Key Scheduling
    int j = 0;
    for (int i = 0; i < SIZE; i++)
    {
        j = (j + S[i] + K[i]) % SIZE;
        std::swap(S[i], S[j]);
    }

    // Pseudo Random Generation Algorithm

    int i = 0;
    j = 0;
    int msgSize = message.size();
    while (msgSize--)
    {
        i = (i + 1) % SIZE;
        j = (j + S[j]) % SIZE;
        std::swap(S[i], S[j]);
        int t = (S[i] + S[j]) % SIZE;
        keyStream.push_back(t);
    }
    int temp = message.size();

    for (int x = 0; x < temp; x++)
    {
        eD.push_back((message[x] ^ keyStream[x]));
    }
    return vecToStr(eD);
}
