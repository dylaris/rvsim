#ifndef FLOW_H
#define FLOW_H

#define IS_TRAP(flow) ((flow) >= FLOW_ECALL)
typedef enum {
    FLOW_NONE = 0,
    FLOW_BRANCH_TAKEN,
    FLOW_BRANCH_NOT_TAKEN,
    FLOW_DIRECT_JUMP,
    FLOW_INDIRECT_JUMP,
    FLOW_ECALL
} Flow;

#endif // FLOW_H
