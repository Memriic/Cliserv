#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <algorithm>

using namespace std;

struct thread_arg
{
   char symbol;
   char code[100];
   int index;
   double modCumProb;
   int codeLength;
};

void fireman(int)
{
   while (waitpid(-1, NULL, WNOHANG) > 0)
      std::cout << "A child process ended" << std::endl;
}

string doubleToBinary(double num, int decimal_length)
{
   string binary = "";
   int integral = num;                 // get the whole number part of the double
   double fractional = num - integral; // get the fractional part of the double

   while (integral)
   {
      int rem = integral % 2;
      binary.push_back(rem + '0');
      integral /= 2;
   }
   reverse(binary.begin(), binary.end());
   binary.push_back('.');

   while (decimal_length--)
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

string getShanonCode(double modCumProb, int codeLength)
{
   return doubleToBinary(modCumProb, codeLength).erase(0, 1);
}

int main(int argc, char *argv[])
{
   int sockfd, newsockfd, portno, clilen;
   struct sockaddr_in serv_addr, cli_addr;
   int n;
   pid_t pid;
   thread_arg arg;
   signal(SIGCHLD, fireman);

   if (argc < 2)
   {
      std::cerr << "ERROR, no port provided\n";
      exit(1);
   }
   sockfd = socket(AF_INET, SOCK_STREAM, 0);
   if (sockfd < 0)
   {
      std::cerr << "ERROR opening socket";
      exit(1);
   }
   bzero((char *)&serv_addr, sizeof(serv_addr));
   portno = atoi(argv[1]);
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = INADDR_ANY;
   serv_addr.sin_port = htons(portno);
   if (bind(sockfd, (struct sockaddr *)&serv_addr,
            sizeof(serv_addr)) < 0)
   {
      std::cerr << "ERROR on binding";
      exit(1);
   }
   listen(sockfd, 5);
   clilen = sizeof(cli_addr);

   while (true)
   {
      if (newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, (socklen_t *)&clilen))
      {
         pid = fork();
         if (pid == 0)
         {
            if (newsockfd < 0)
            {
               std::cerr << "ERROR on accept";
               exit(1);
            }
            n = read(newsockfd, &arg.modCumProb, sizeof(double));
            if (n < 0)
            {
               std::cerr << "ERROR reading modcumprob from socket";
               exit(1);
            }
            n = read(newsockfd, &arg.codeLength, sizeof(int));
            if (n < 0)
            {
               std::cerr << "ERROR reading codelength from socket";
               exit(1);
            }

            string shanonCode = getShanonCode(arg.modCumProb, arg.codeLength);
            memcpy(arg.code, shanonCode.c_str(), shanonCode.size());
            n = write(newsockfd, &arg.code, sizeof(arg.code));
            if (n < 0)
            {
               std::cerr << "ERROR writing to socket";
               exit(1);
            }
            _exit(0);
         }
      }

      return 0;
   }
}