#include "api.h"

#include "decode.c"

bool api_decode(u32 raw, Instr *out)
{
    return decode_instr(raw, out);
}

bool api_lookup(u32 raw, InstrInfo *out)
{
    return decode_instr_info(raw, out);
}
