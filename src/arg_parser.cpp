#include "arg_parser.h"
#include "3dparty/simpleopt.h"
#include <stdlib.h>
#include <iostream>
#include <cstdio>
#include <string.h>
#include <fstream>
using namespace std;

ArgumentsParser::ArgumentsParser(int argc, char *argv[])
{
    CSimpleOpt::SOption g_rgOptions[] = {
        {OPT_ADDRESS, "--address", SO_REQ_SEP},
        {OPT_CRC,     "--crc", SO_REQ_SEP},
        {OPT_CRC_AT,  "--crc-at", SO_REQ_SEP},
        {OPT_INPUT,   "--input", SO_REQ_SEP},
        {OPT_OUTPUT,  "--output", SO_REQ_SEP},
        {OPT_VERBOSE, "--verbose", SO_NONE},
        {OPT_HELP,    "--help", SO_NONE},
        {OPT_HELP,    "-?", SO_NONE},
        SO_END_OF_OPTIONS
    };

    this->mCrcSource = CrcFromNone;
    this->mVerbose = false;
    this->mCollectedOpts = OPT_NONE;

    CSimpleOpt args(argc, argv, g_rgOptions);

    while (args.Next()) {
        if (args.LastError() == SO_SUCCESS) {
            // handle option, using OptionId(), OptionText() and OptionArg()
            switch (args.OptionId()) {
                case OPT_ADDRESS: {
                    uint32_t a;
                    sscanf(args.OptionArg(), "%x", &a);
                    this->mCrcWriteAddress = a;
                    this->mCollectedOpts = (Opts)(this->mCollectedOpts | args.OptionId());
                    break;
                }

                case OPT_CRC: {
                    uint32_t a;
                    sscanf(args.OptionArg(), "%x", &a);
                    this->mCrcResult = a;
                    this->mCrcSource = CrcFromInput;
                    this->mCollectedOpts = (Opts)(this->mCollectedOpts | args.OptionId());
                    break;
                }

                case OPT_CRC_AT: {
                    uint32_t a;
                    sscanf(args.OptionArg(), "%x", &a);
                    this->mCrcReadAddress = a;
                    this->mCrcSource = CrcFromAddress;
                    this->mCollectedOpts = (Opts)(this->mCollectedOpts | args.OptionId());
                    break;
                }

                case OPT_INPUT: {
                    snprintf(this->mInputFileName, sizeof(this->mInputFileName), "%s", args.OptionArg());
                    this->mCollectedOpts = (Opts)(this->mCollectedOpts | args.OptionId());
                    break;
                }

                case OPT_OUTPUT: {
                    snprintf(this->mOutputFileName, sizeof(this->mOutputFileName), "%s", args.OptionArg());
                    this->mCollectedOpts = (Opts)(this->mCollectedOpts | args.OptionId());
                    break;
                }

                case OPT_VERBOSE: {
                    this->mVerbose = true;
                    this->mCollectedOpts = (Opts)(this->mCollectedOpts | args.OptionId());
                    break;
                }

                default: {
                    this->mCollectedOpts = OPT_NONE;
                    break;
                }
            }

            /*_sntprintf(strBuffer, sizeof(strBuffer),
                _T("Option, ID: %d, Text: '%s', Argument: '%s'\n"),
                args.OptionId(), args.OptionText(),
                (args.OptionArg() != NULL ? args.OptionArg() : _T("")));
            Log(strBuffer);*/
        } else {
            // handle error, one of: SO_OPT_INVALID, SO_OPT_MULTIPLE,
            // SO_ARG_INVALID, SO_ARG_INVALID_TYPE, SO_ARG_MISSING
            this->mCollectedOpts = OPT_NONE;
        }
    }
}

ArgumentsParser::~ArgumentsParser(void)
{
}

bool ArgumentsParser::valid(void)
{
    return (this->mValid);
}

uint32_t ArgumentsParser::address(void)
{
    return (this->mCrcWriteAddress);
}

uint32_t ArgumentsParser::crc(void)
{
    return (this->mCrcResult);
}

const char *ArgumentsParser::inputFileName(void) const
{
    return (this->mInputFileName);
}

const char *ArgumentsParser::outputFileName(void) const
{
    return (this->mOutputFileName);
}

bool ArgumentsParser::verbose(void)
{
    return (this->mVerbose);
}

bool ArgumentsParser::validate(logger *log)
{
    bool result = true;
    char strBuffer[2048];

    if ((this->mCollectedOpts & OPT_INPUT) == OPT_NONE) {
        log("Input file not specified");
        result = false;
    }

    if ((this->mCollectedOpts & OPT_OUTPUT) == OPT_NONE) {
        log("Output file not specified");
        result = false;
    }

    if ((this->mCollectedOpts & (OPT_CRC | OPT_CRC_AT)) == OPT_NONE) {
        log("Desired CRC not specified");
        result = false;
    }

    if (result) {
        // try open source file
        ifstream fileIn(this->mInputFileName, ios::in | ios::binary | ios::ate);

        if (fileIn.is_open()) {
            // get length
            streampos filesize = fileIn.tellg();

            // if file not empty
            if (filesize > 0) {
                // save length
                this->mFileSize = (uint32_t)filesize;

                // if specified, read crc from position in file
                if (this->mCrcSource == CrcFromAddress) {
                    fileIn.seekg(this->mCrcReadAddress, ios::beg);
                    uint32_t readedCrc = 0;

                    if (fileIn.read((char *)readedCrc, 4).gcount() == 4) {
                        this->mCrcResult = readedCrc;
                    } else {
                        result = false;
                    }
                }

                if (this->mCrcSource != CrcFromAddress) {
                    fileIn.seekg(0, ios::beg);
                    char b;

                    if (fileIn.read(&b, 1).gcount() != 1) {
                        result = false;
                    }
                }
            }

            fileIn.close();
        } else {
            result = false;
        }

        if (!result) {
            snprintf(strBuffer, sizeof(strBuffer), "Cannot open input file: %s", this->mInputFileName);
            log(strBuffer);
        }
    }

    if (result) {
        // try open output file
        ofstream fileOut(this->mOutputFileName, ios::out | ios::binary);

        if (fileOut.is_open()) {
            fileOut.close();
        } else {
            snprintf(strBuffer, sizeof(strBuffer), "Cannot open output file: %s", this->mOutputFileName);
            log(strBuffer);
            result = false;
        }
    }

    if (result) {
        if (this->mCrcWriteAddress < this->mFileSize) {
            uint32_t addr = this->mCrcWriteAddress;
            addr += 4;

            if (addr < 4 || addr > this->mFileSize) {
                snprintf(strBuffer, sizeof(strBuffer), "Invalid address for CRC: 0x%08x", this->mCrcWriteAddress);
                log(strBuffer);
                result = false;
            }
        }
    }

    return result;
}
