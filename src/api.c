#include "api.h"

#include "decoder.c"

bool api_decode(u32 data, Instruction *outp)
{
    inst_decode(outp, data);
    return true;
}
