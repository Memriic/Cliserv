#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <map>
#include <pthread.h>
#include <fcntl.h>
#include <semaphore.h>
#include <cmath>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
using namespace std;

struct thread_arg
{
    string symbol;
    string code;
    double modCumProbs;
    double cumulative;
    int codeLengths;
    int threadid;
    int turn;
    string name;
    pthread_mutex_t *bsem_first;
    pthread_mutex_t *bsem_second;
    pthread_cond_t *waitTurn;
};

string doubleToBinary(double num, double prob)
{
    string binary = "";
    int integral = num;                 // get the whole number part of the double
    double fractional = num - integral; // get the fractional part of the double
    int CodeLength = (ceil(log2(1 / prob) + 1));
    while (integral)
    {
        int rem = integral % 2;
        binary.push_back(rem + '0');
        integral /= 2;
    }
    reverse(binary.begin(), binary.end());

    while (CodeLength--)
    {
        fractional *= 2;
        int fract_int = fractional;
        if (fract_int == 1)
        {
            fractional -= fract_int;
            binary.push_back(1 + '0');
        }
        else
            binary.push_back(0 + '0');
    }

    return binary;
}

map<char, int> getCharFreq(string charArr)
{
    map<char, int> charFreqs;
    for (char i : charArr)
    {
        charFreqs[i]++;
    }
    return charFreqs;
}
vector<double> getModProbs(vector<double> probs)
{
    vector<double> result;
    for (int i = 0; i < probs.size(); i++)
    {
        double tempSum = 0;
        for (int j = 0; j < i; j++)
        {
            tempSum += probs[j];
        }
        result.push_back(probs[i] * 0.5 + tempSum);
    }
    return result;
}
vector<int> getCodeLengths(vector<double> probs)
{
    vector<int> result;
    for (auto &prob : probs)
    {
        result.push_back(ceil(log2(1.0 / prob) + 1));
    }
    return result;
}

void *shanonCoding(void *argPtr)
{
    // type casting argPtr to type thread_arg
    struct thread_arg *ptr = (struct thread_arg *)argPtr;
    double thread_num = ptr->threadid;
    string sym = ptr->symbol;
    double probability = ptr->modCumProbs;
    double cum = ptr->cumulative + probability;
    ptr->cumulative = cum;
    double difference = cum - probability;
    double fx = difference + (probability / 2);
    pthread_mutex_unlock(ptr->bsem_first);

    string code = doubleToBinary(fx, probability);
    // mechanism to print in order...

    pthread_mutex_lock(ptr->bsem_second);
    while (ptr->turn != thread_num)
    {
        pthread_cond_wait(ptr->waitTurn, ptr->bsem_second);
    }
    pthread_mutex_unlock(ptr->bsem_second);

    cout << "Symbol " << sym << ", Code: " << code << endl;

    pthread_mutex_lock(ptr->bsem_second);
    (ptr->turn)++;
    pthread_cond_broadcast(ptr->waitTurn);
    pthread_mutex_unlock(ptr->bsem_second);

    return nullptr;
}

int main()
{
    // semaphores setup
    pthread_mutex_t bsem_one;
    pthread_mutex_init(&bsem_one, NULL);
    pthread_mutex_t bsem_two;
    pthread_mutex_init(&bsem_two, NULL);
    pthread_cond_t waitTurn = PTHREAD_COND_INITIALIZER;
    char name[] = "parentToC";

    // receive and store input
    string charArr;
    cin >> charArr;

    // sort string before computing frequencies
    sort(charArr.begin(), charArr.end());

    // find frequency and probabilities from input
    map<char, int> charFreqs = getCharFreq(charArr);

    // get unique chars and respective freqs
    int size = charFreqs.size();
    vector<char> symbols;
    vector<int> freqs;
    int total = 0;
    vector<double> probs;
    for (const auto &elem : charFreqs)
    {
        symbols.push_back(elem.first);
        freqs.push_back(elem.second);
        total += elem.second;
    }

    // get probabilities
    for (const auto &elem : freqs)
    {
        probs.push_back(elem / total);
    }
    vector<double> modCumProbs = getModProbs(probs);
    vector<int> codeLengths = getCodeLengths(probs);
    thread_arg arg;
    pthread_t th[size];

    // create child threads
    cout << "SHANNON-FANO-ELIAS Codes:" << endl;
    cout << endl;

    arg.name = name;
    arg.turn = 0;
    arg.bsem_first = &bsem_one;
    arg.bsem_second = &bsem_two;
    arg.waitTurn = &waitTurn;
    double total2;
    double tempFreq;
    vector<double> tempVec;
    for (int i = 0; i < size; i++)
    {

        total2 += freqs[i];
    }
    for (int i = 0; i < size; i++)
    {

        tempFreq = freqs[i] / total2;
        tempVec.push_back(tempFreq);
    }

    for (int i = 0; i < size; i++)
    {
        pthread_mutex_lock(&bsem_one);
        // initializing thread arg
        arg.symbol = symbols[i];
        arg.modCumProbs = tempVec[i];
        arg.threadid = i;
        if (pthread_create(&th[i], nullptr, shanonCoding, &arg) != 0)
        {
            perror("failed to create thread...");
        }
    }
    // joining threads
    for (int i = 0; i < size; i++)
    {
        if (pthread_join(th[i], nullptr) != 0)
        {
            perror("failed to join thread...");
        }
    }

    return 0;
}