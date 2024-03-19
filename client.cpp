#include <iostream>
#include <unistd.h>
#include <algorithm>
#include <map>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
using namespace std;
struct thread_arg
{
    char symbol;
    char code[100];
    double modCumProb;
    int codeLength;
    vector<double> probs;
    struct sockaddr_in serv_addr;
};
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
void *talkToServer(void *argPtr)
{
    struct thread_arg *ptr = (struct thread_arg *)argPtr;
    int n, sockfd;
    // creating a socket specific to the thread p_argument
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        cerr << "ERROR opening socket" << endl;
    if (connect(sockfd, (struct sockaddr *)&ptr->serv_addr, sizeof(ptr->serv_addr)) < 0)
        cerr << "ERROR connecting" << endl;
    // writing to and reading from the server...
    // writing the modCumProb and codeLength, then reading the shanon code
    n = write(sockfd, &ptr->modCumProb, sizeof(double));
    if (n < 0)
        cerr << "ERROR writing to socket";
    n = write(sockfd, &ptr->codeLength, sizeof(int));
    if (n < 0)
        cerr << "ERROR writing to socket";
    // reading shanon code result from server
    bzero(ptr->code, sizeof(ptr->code));
    n = read(sockfd, &ptr->code, sizeof(ptr->code));
    if (n < 0)
        cerr << "ERROR reading from socket";
    close(sockfd);
    return nullptr;
}
int main(int argc, char *argv[])
{
    struct hostent *server;
    string temptempServer;
    char tempServer[50];
    struct sockaddr_in serv_addr;
    int portno;
    // socket--server_addr struct populatio
    portno = atoi(argv[2]);
    server = gethostbyname(argv[1]);
    if (server == NULL)
    {
        cout << stderr << "ERROR, no such host" << endl;
        exit(0);
    }
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(portno);
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
    thread_arg args[size];
    pthread_t th[size];
    // create child threads
    for (int i = 0; i < size; i++)
    {
        // initializing thread arg
        args[i].symbol = symbols[i];
        args[i].modCumProb = modCumProbs[i];
        args[i].codeLength = codeLengths[i];
        args[i].serv_addr = serv_addr;
        if (pthread_create(&th[i], nullptr, talkToServer, &args[i]) != 0)
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
    // cout resulting shanon codes
    cout << "SHANNON-FANO-ELIAS Codes:" << endl;
    cout << endl;
    for (int i = 0; i < size; i++)
    {
        cout << "Symbol " << args[i].symbol << ", Code: " << args[i].code << endl;
    }
    return 0;
}