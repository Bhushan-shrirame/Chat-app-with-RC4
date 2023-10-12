#include "rc4.h"
#include <fstream>
#include <unordered_set>
#include <cstring>

std::unordered_set<std::string> dict;

bool cracked(std::string word)
{
    for (auto &x : word)
    {
        x = tolower(x);
    }
    if (dict.find(word) != dict.end())
    {
        return true;
    }
    return false;
}

void set_keys(std::fstream &fdict)
{
    std::string words;
    while (std::getline(fdict, words))
    {
        dict.insert(words);
    }
}
int main()
{
    std::string fileName;
    std::cin >> fileName;
    std::string msg;
    std::fstream fr, fw, fdict;
    fr.open(fileName, std::fstream::in);
    fdict.open("dict.txt", std::fstream::in);
    fw.open("dec_" + fileName, std::fstream::out);
    msg = "";
    int key = -1;
    int secured = 0;
    set_keys(fdict);
    while (std::getline(fr, msg))
    {
        if (msg.size() >= 11 && msg.substr(0, 11) == "disable-rc4")
        {
            secured = 0;
        }
        if (secured)
        {
            if (key == -1)
            {
                for (int i = 1; i < 3000; i++)
                {
                    bool keyFound = false;
                    std::string dec = encryptDecrypt(msg, i);
                    std::string to_find = "";
                    for (int j = 0; j < dec.size(); j++)
                    {
                        if (dec[j] == ' ')
                        {
                            keyFound = cracked(to_find);
                            to_find = "";
                            if (keyFound)
                            {
                                break;
                            }
                        }
                        to_find += dec[j];
                    }
                    if (to_find != "")
                        keyFound = cracked(to_find);
                    if (keyFound)
                    {
                        key = i;
                        break;
                    }
                }
            }
            fw << encryptDecrypt(msg, key) << "\n";
        }
        else
        {
            if (msg.size() >= 10 && msg.substr(0, 10) == "enable-rc4")
            {
                secured = 1;
            }
            fw << msg << "\n";
        }
    }
}