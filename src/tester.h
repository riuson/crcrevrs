#ifndef SRC_TESTER_H_
#define SRC_TESTER_H_

#include "logger.h"
#include "recover.h"

class Tester : private Recover
{
public:
    Tester();
    virtual ~Tester();

    bool run(logger *log);

private:
    bool test(logger *log);
};

#endif /* SRC_TESTER_H_ */
