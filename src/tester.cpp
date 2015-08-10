#include "tester.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

Tester::Tester()
{
}

Tester::~Tester()
{
}

bool Tester::run(logger *log)
{
    bool result = true;

    srand(time(NULL));

    for (int i = 0; i < 100; i++) {
        if (this->test(log)) {
            log("Success");
        } else {
            log("Failure");
            result = false;
        }
    }

    return result;
}

bool Tester::test(logger *log)
{
    bool result = false;
    uint32_t random_length = (rand() % 500) + 10;

    uint8_t *array = new uint8_t[random_length];

    for (uint32_t i = 0; i < random_length; i++) {
        array[i] = (uint8_t)(rand() % 0xff);
    }

    uint32_t random_crc = 0;
    random_crc |= ((uint8_t)(rand() % 0xff)) << 0;
    random_crc |= ((uint8_t)(rand() % 0xff)) << 8;
    random_crc |= ((uint8_t)(rand() % 0xff)) << 16;
    random_crc |= ((uint8_t)(rand() % 0xff)) << 24;

    uint32_t random_address = (uint32_t)(rand() % (random_length - 4));

    char str[1024];
    snprintf(str, sizeof(str), "Test array of size %d byte(s), target CRC 0x%08x", random_length, random_crc);
    log(str);

    this->patch(array, random_length, random_length, random_address, random_crc, log);

    uint32_t crc = AllOnes;

    for (uint32_t i = 0; i < random_length; i++) {
        crc = this->getHashNext(crc, array[i]);
    }

    crc ^= AllOnes;

    if (crc == random_crc) {
        result = true;
    }

    delete []array;

    return result;
}
