#include "recover.h"
#include <iostream>
#include <fstream>

using namespace std;

Recover::Recover(void)
{
    uint32_t dwCrc;
    this->mTable = new uint32_t[TableSize];

    // 256 values representing ASCII character codes.
    for (uint32_t i = 0; i < TableSize; i++) {
        dwCrc = i;

        for (int j = 0; j < 8; j++) {
            dwCrc = ((dwCrc & 0x00000001) != 0) ? (dwCrc >> 1) ^ DefaultPolynomial : dwCrc >> 1;
        }

        this->mTable[i] = dwCrc;
    }
}

Recover::~Recover(void)
{
    delete [] this->mTable;
}

void Recover::patchFile(char *inputFileName, char *outputFileName, uint32_t address, CrcFrom crcSource, uint32_t crc, logger *log)
{
    char strBuffer[2048];

    snprintf(strBuffer, sizeof(strBuffer), "Input file: %s", inputFileName);
    log(strBuffer);

    snprintf(strBuffer, sizeof(strBuffer), "Output file: %s", outputFileName);
    log(strBuffer);

    snprintf(strBuffer, sizeof(strBuffer), "Address: %08x", address);
    log(strBuffer);

    if (crcSource == CrcFromInput) {
        snprintf(strBuffer, sizeof(strBuffer), "Target CRC: %08x", crc);
        log(strBuffer);
    } else {
        snprintf(strBuffer, sizeof(strBuffer), "Target CRC at address: %08x", crc);
        log(strBuffer);
    }

    snprintf(strBuffer, sizeof(strBuffer), "\nOpening file: '%s'...", inputFileName);
    log(strBuffer);

    ifstream fileIn(inputFileName, ios::in | ios::binary | ios::ate);

    if (fileIn.is_open()) {
        streampos filesize = fileIn.tellg();

        if (filesize > 0) {
            uint32_t len = (uint32_t)filesize;
            uint8_t *buffer = new uint8_t[len];
            fileIn.seekg(0, ios::beg);
            fileIn.read((char *)buffer, len);
            fileIn.close();

            snprintf(strBuffer, sizeof(strBuffer), "Readed %d byte(s)", len);
            log(strBuffer);

            if (crcSource == CrcFromAddress) {
                uint32_t holdedCrc = buffer[crc] | (buffer[crc + 1] << 8) | (buffer[crc + 2] << 16) | (buffer[crc + 3] << 24);
                crc = holdedCrc;

                snprintf(strBuffer, sizeof(strBuffer), "Readed Target CRC: %08x", crc);
                log(strBuffer);
            }

            log("Applying patch...");

            this->patch(buffer, len, address, crc, log);

            snprintf(strBuffer, sizeof(strBuffer), "\nOpening file: '%s'...", outputFileName);
            log(strBuffer);

            ofstream fileOut(outputFileName, ios::out | ios::binary | ios::ate);

            if (fileOut.is_open()) {
                fileOut.write((char *)buffer, len);
                fileOut.flush();
                fileOut.close();

                snprintf(strBuffer, sizeof(strBuffer), "Written %d byte(s)", len);
                log(strBuffer);
            } else {
                snprintf(strBuffer, sizeof(strBuffer), "Error: Can't open file: '%s'", outputFileName);
                log(strBuffer);
            }

            delete []buffer;
        } else if (filesize == 0) {
            fileIn.close();
            snprintf(strBuffer, sizeof(strBuffer), "Error: File empty: '%s'", inputFileName);
            log(strBuffer);
        } else {
            // < 0
            fileIn.close();
            snprintf(strBuffer, sizeof(strBuffer), "Error: Can't get file size: '%s'", inputFileName);
            log(strBuffer);
        }
    } else {
        snprintf(strBuffer, sizeof(strBuffer), "Error: Can't open file: '%s'", inputFileName);
        log(strBuffer);
    }
}

void Recover::patch(uint8_t *buffer, uint32_t size, uint32_t address, uint32_t crc, logger *log)
{
    char strBuffer[2048];

    if (address + 4 >= size) {
        snprintf(strBuffer, sizeof(strBuffer), "Error: Given address: %08x, but data array of size %08x", address, size);
        log(strBuffer);
        return;
    }

    log("Calculating CRC forward...");
    // ������ crc � ������ ����������� ������ �� ��������
    uint32_t crcForward = AllOnes;

    for (uint32_t i = 0; i < size && i < address; i++) {
        crcForward = this->getHashNext(crcForward, buffer[i]);
    }

    snprintf(strBuffer, sizeof(strBuffer), "... %08x", crcForward);
    log(strBuffer);

    log("Calculating CRC backward...");
    // ������ crc � �������� ����������� ������ �� ��������
    uint32_t crcBackward = crc ^ AllOnes;

    for (uint32_t i = size - 1; i >= address + 4; i--) {
        crcBackward = this->getHashPrev(crcBackward, buffer[i]);
    }

    snprintf(strBuffer, sizeof(strBuffer), "... %x", crcBackward);
    log(strBuffer);

    log("Calculating stub data...");

    // ������ ��������
    uint32_t stub = this->calculateStub(crcForward, crcBackward);

    snprintf(strBuffer, sizeof(strBuffer), "... %08x", stub);
    log(strBuffer);

    log("Applying stub...");

    // ��������� ��������
    buffer[address + 0] = this->getByte(stub, 3);
    buffer[address + 1] = this->getByte(stub, 2);
    buffer[address + 2] = this->getByte(stub, 1);
    buffer[address + 3] = this->getByte(stub, 0);

    log("Comparing...");

    // ����������� ���������� crc ��� ��������� � ������
    uint32_t crcControl = AllOnes;

    for (uint32_t i = 0; i < size; i++) {
        crcControl = this->getHashNext(crcControl, buffer[i]);
    }

    crcControl ^= AllOnes;

    snprintf(strBuffer, sizeof(strBuffer), "Target CRC: %08x, result CRC: %08x", crc, crcControl);
    log(strBuffer);

    if (crcControl != crc) {
        log("o_O Can't find stub for CRC-32!");
    }
}

void Recover::findInTable(uint8_t sourceValue,  uint32_t *tableValue, uint32_t *tableIndex)
{
    union {
        uint32_t dword;
        uint8_t bytes[4];
    } a;

    for (uint32_t i = 0; i < TableSize; i++) {
        // ���� ������� ���� ���������
        a.dword = this->mTable[i];

        if (sourceValue == a.bytes[3]) {
            *tableValue = a.dword;
            *tableIndex = i;
            break;
        }
    }
}

void Recover::setByte(uint32_t *dword, uint8_t index, uint8_t byte)
{
    union {
        uint32_t dword;
        uint8_t bytes[4];
    } a;

    if (index <= 3) {
        a.dword = *dword;
        a.bytes[index] = byte;
        *dword = a.dword;
    }
}
uint8_t Recover::getByte(uint32_t dword, uint8_t index)
{
    union {
        uint32_t dword;
        uint8_t bytes[4];
    } a;

    if (index <= 3) {
        a.dword = dword;
        return (a.bytes[index]);
    }

    return (0);
}


uint32_t Recover::getHashNext(uint32_t previousValue, uint8_t nextByte)
{
    uint32_t result = this->mTable[(previousValue ^ nextByte) & 0xFF] ^ (previousValue >> 8);
    return (result);
}

uint32_t Recover::getHashPrev(uint32_t nextValue, uint8_t nextByte)
{
    uint32_t tableIndex = 0;
    uint32_t tableValue = 0;
    this->findInTable(this->getByte(nextValue, 3), &tableValue, &tableIndex);
    // ��� �� ����� ��������� �������� � ��� ������
    // ������� ���� �� (previousValue ^ nextByte) = tableIndex
    uint32_t prevValueL = (tableIndex ^ nextByte) & 0x000000ff;

    // � (crc32Table[(previousValue ^ nextByte) & 0xFF]) = tableValue
    // ������ ����� ����� (previousValue >> 8), �.�. prevValue ��� �������� �����
    uint32_t prevValueH = nextValue ^ tableValue;
    prevValueH = (prevValueH << 8) & 0xffffff00;

    uint32_t prevValue = prevValueH | prevValueL;

    return (prevValue);
}

uint32_t Recover::calculateStub(uint32_t a, uint32_t f)
{
    uint32_t result = 0;

    uint32_t e = 0;
    uint32_t indexE = 0;
    this->findInTable(this->getByte(f, 3), &e, &indexE);

    uint32_t d = 0;
    uint32_t indexD = 0;
    this->findInTable(this->getByte(f, 2) ^ this->getByte(e, 2), &d, &indexD);

    uint32_t c = 0;
    uint32_t indexC = 0;
    this->findInTable(this->getByte(f, 1) ^ this->getByte(e, 1) ^ this->getByte(d, 2), &c, &indexC);

    uint32_t b = 0;
    uint32_t indexB = 0;
    this->findInTable(this->getByte(f, 0) ^ this->getByte(e, 0) ^ this->getByte(d, 1) ^ this->getByte(c, 2), &b, &indexB);

    this->setByte(&result, 3, indexB ^ this->getByte(a, 0));
    this->setByte(&result, 2, indexC ^ this->getByte(b, 0) ^ this->getByte(a, 1));
    this->setByte(&result, 1, indexD ^ this->getByte(c, 0) ^ this->getByte(b, 1) ^ this->getByte(a, 2));
    this->setByte(&result, 0, indexE ^ this->getByte(d, 0) ^ this->getByte(c, 1) ^ this->getByte(b, 2) ^ this->getByte(a, 3));

    return (result);
}
