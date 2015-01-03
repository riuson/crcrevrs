//============================================================================
// Name        : crcrevrs.cpp
// Author      : riuson
// Version     :
// Copyright   : Creative Commons
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include "arg_parser.h"
#include "recover.h"

using namespace std;

void ShowHelp(void);
void Log(const char *str);
void LogNull(const char *str);

int main(int argc, char* argv[]) {

    ArgumentsParser parser(argc, argv);

    if (parser.valid())
    {
        logger *log = &LogNull;
        if (parser.verbose() != 0)
        {
            log = &Log;
        }

        Recover recover;
        recover.patchFile(parser.inputFileName(), parser.outputFileName(), parser.address(), parser.crcSource(), parser.crc(), log);
    }
    else
    {
        ShowHelp();
    }

    return (0);
}

void Log(const char *str)
{
    cout << str << endl;
}

void LogNull(const char *str)
{
    (void)str;
}

void ShowHelp(void)
{
    Log(
"CRC-32 Recovery Utility\n\
Using:\n\
crcrec.exe --address 0x08 --crc 0x123456ab --input \"e:\\data\\file.bin\" --output \"e:\\data\\file2.bin\" \n\
--address   address of stub to correct calculated crc.\n\
--crc       target crc, what need to be calculated from file.\n\
--crc-at    target crc placed in file at specified address.\n\
--verbose   show log.\n\
--input     input binary file.\n\
--output    output binary file.\n");
}
