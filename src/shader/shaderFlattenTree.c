#include <dolphin.h>
#include "shader.h"

struct ShaderTEVStage EmptyAlphaInstruction = {
    // channel
    { SHADER_TEV_ALPHA },
    // type
    { SHADER_TYPE_EMPTY },
    // parent[8]
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
    // numParents
    { 0 },
    // numNonAllocatedChildren
    { 0 },
    // TEVArg[4]
    { SHADER_APREV, SHADER_ZERO, SHADER_ZERO, SHADER_ZERO },
    // tevStage[4]
    { NULL, NULL, NULL, NULL },
    // op
    { GX_TEV_ADD },
    // bias
    { GX_TB_ZERO },
    // scale
    { GX_CS_SCALE_1 },
    // clamp
    { 0 },
    // mode
    { GX_TC_LINEAR },
    // out_reg
    { SHADER_APREV },
    // rasInput
    { SHADER_RASNONE },
    // texGenIdx
    { 0x63 },
    // texInput
    { SHADER_TEXNONE },
    // LERPType
    { SHADER_SIMPLE },
    // outputIndex
    { 0 },
    // outputArg
    { SHADER_APREV }
};

struct ShaderTEVStage EmptyColorInstruction = {
    // channel
    { SHADER_TEV_COLOR },
    // type
    { SHADER_TYPE_EMPTY },
    // parent[8]
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
    // numParents
    { 0 },
    // numNonAllocatedChildren
    { 0 },
    // TEVArg[4]
    { SHADER_CPREV, SHADER_ZERO, SHADER_ZERO, SHADER_ZERO },
    // tevStage[4]
    { NULL, NULL, NULL, NULL },
    // op
    { GX_TEV_ADD },
    // bias
    { GX_TB_ZERO },
    // scale
    { GX_CS_SCALE_1 },
    // clamp
    { 0 },
    // mode
    { GX_TC_LINEAR },
    // out_reg
    { SHADER_CPREV },
    // rasInput
    { SHADER_RASNONE },
    // texGenIdx
    { 0x63 },
    // texInput
    { SHADER_TEXNONE },
    // LERPType
    { SHADER_SIMPLE },
    // outputIndex
    { 0 },
    // outputArg
    { SHADER_CPREV }
};

static struct ShaderTEVStage * AllocationList[32];

// functions
static void FindSchedulableInstructions(struct ShaderTEVStage * stage, struct ShaderTEVStage * * SIList, u8 * numSI);
static u8 IsInstructionInSIList(struct ShaderTEVStage * stage, struct ShaderTEVStage * * SIList, u8 numSI);
static u8 Flatten(struct ShaderTEVStage * * SIList, u8 numSI);
static u8 AddInstruction(struct ShaderTEVStage * stage);
static void RemoveInstruction(struct ShaderTEVStage * stage, u8 index);
static u8 FindStartPoint(struct ShaderTEVStage * stage);
static void RemoveInstructionFromSIList(u8 index, struct ShaderTEVStage * * SIList, u8 * numSI);
static void RemoveInstructionFromSIList2(struct ShaderTEVStage * stage, struct ShaderTEVStage * * SIList, u8 * numSI);
static void AddInstructionToSIList(struct ShaderTEVStage * instruction, struct ShaderTEVStage * * SIList, u8 * numSI);
static void AddInstructionToSIList2(struct ShaderTEVStage * instruction, u8 index, struct ShaderTEVStage * * SIList, u8 * numSI);
static u8 CheckParallelList(struct ShaderTEVStage * stage1, struct ShaderTEVStage * stage2);
static void PadInstructionList(void);
static void UnPadInstructionList(void);
static u8 AddRootAlphaInstruction(struct ShaderTEVStage * stage);
static u8 FindLastAlphaInstruction(void);

void FlattenTEVTree(void) {
    struct ShaderTEVStage * SIList[32];
    u8 numSI;
    u8 i;

    numSI = 0;
    for(i = 0; i < 16; i++) {
        AlphaInstructions[i] = 0;
        ColorInstructions[i] = 0;
    }
    TEVPool[0].tevStage[1]->numNonAllocatedChildren = 1;
    for(i = 0; i < 32; i++) {
        AllocationList[i] = 0;
    }
    FindSchedulableInstructions(TEVPool, SIList, &numSI);
    Flatten(SIList, numSI);
}

static void FindSchedulableInstructions(struct ShaderTEVStage * stage, struct ShaderTEVStage * * SIList, u8 * numSI) {
    u8 i;
    u8 j;

    for(j = 0; j < 32; j++) {
        if (AllocationList[j] == stage) {
            break;
        }
        if (AllocationList[j] == 0) {
            AllocationList[j] = stage;
            for(i = 0; i < 4; i++) {
                if (stage->tevStage[i]) {
                    FindSchedulableInstructions(stage->tevStage[i], SIList, numSI);
                    stage->numNonAllocatedChildren++;
                }
            }
            if ((stage->numNonAllocatedChildren == 0) && (IsInstructionInSIList(stage, SIList, *numSI) != 0)) {
                SIList[*numSI] = stage;
                *numSI = *numSI + 1;
            }
            return;
        }
    }
}

static u8 IsInstructionInSIList(struct ShaderTEVStage * stage, struct ShaderTEVStage * * SIList, u8 numSI) {
    u8 i;

    for(i = 0; i < numSI; i++) {
        if (SIList[i] == stage) {
            return 0;
        }
    }
    return 1;
}

static u8 Flatten(struct ShaderTEVStage * * SIList, u8 numSI) {
    u8 i;
    u8 j;
    struct ShaderTEVStage * currentInstruction;
    u8 instrIndex;
    u8 cost;

    if (numSI) {
        for(i = 0; i < numSI; i++) {
            currentInstruction = SIList[i];
            instrIndex = AddInstruction(currentInstruction);
            if (instrIndex != 0x63) {
                RemoveInstructionFromSIList(i, SIList, &numSI);
                for(j = 0; j < currentInstruction->numParents; j++) {
                    currentInstruction->parent[j]->numNonAllocatedChildren--;
                    if (currentInstruction->parent[j]->numNonAllocatedChildren == 0 && currentInstruction->parent[j]->type != 6) {
                        AddInstructionToSIList(currentInstruction->parent[j], SIList, &numSI);
                    }
                }
                if (Flatten(SIList, numSI) != 0) {
                    return 1;
                }
                for(j = 0; j < currentInstruction->numParents; j++) {
                    currentInstruction->parent[j]->numNonAllocatedChildren++;
                    if (currentInstruction->parent[j]->numNonAllocatedChildren == 1) {
                        RemoveInstructionFromSIList2(currentInstruction->parent[j], SIList, &numSI);
                    }
                }
                RemoveInstruction(currentInstruction, instrIndex);
                AddInstructionToSIList2(currentInstruction, i, SIList, &numSI);
            }
        }
    } else {
        instrIndex = AddRootAlphaInstruction(TEVPool[0].tevStage[1]);
        if (instrIndex != 0x63) {
            PadInstructionList();
            cost = CheckResourceAllocation();
            if (cost < BestNumTEVStages) {
                BestNumTEVStages = cost;
                for(i = 0; i < BestNumTEVStages; i++) {
                    if (AlphaInstructions[i]) {
                        BestAlphaInstructions[i] = *AlphaInstructions[i];
                    } else {
                        BestAlphaInstructions[i] = EmptyAlphaInstruction;
                    }
                    if (ColorInstructions[i]) {
                        BestColorInstructions[i] = *ColorInstructions[i];
                    } else {
                        BestColorInstructions[i] = EmptyColorInstruction;
                    }
                }
            }
            if (cost == OptimalTreeSize) {
                return 1;
            }
            if(cost <= OptimalTreeSize)
                OSPanic(__FILE__, 0x144, "cost is fewer than optimal - error in compile!");
            UnPadInstructionList();
            RemoveInstruction(TEVPool[0].tevStage[1], instrIndex);
        }
    }
    return 0;
}

static u8 AddInstruction(struct ShaderTEVStage * stage) {
    struct ShaderTEVStage * * currentList;
    struct ShaderTEVStage * * parallelList;
    u8 startPoint;
    u8 i;

    if (stage->channel == 0) {
        currentList = ColorInstructions;
        parallelList = AlphaInstructions;
    } else {
        currentList = AlphaInstructions;
        parallelList = ColorInstructions;
    }
    startPoint = FindStartPoint(stage);
    for(i = startPoint; i < 16; i++) {
        if (currentList[i] == 0 && CheckParallelList(stage, parallelList[i])) {
            currentList[i] = stage;
            return i;
        }
    }
    return 0x63;
}

static void RemoveInstruction(struct ShaderTEVStage * stage, u8 index) {
    struct ShaderTEVStage * * currentList;

    if (stage->channel == 0) {
        currentList = ColorInstructions;
    } else {
        currentList = AlphaInstructions;
    }
    currentList[index] = 0;
}

static u8 FindStartPoint(struct ShaderTEVStage * stage) {
    u8 i;
    u8 j;
    u8 numChildren;
    u8 numNotFound;
    struct ShaderTEVStage * tevStage[4];

    numChildren = 0;
    for(i = 0; i < 4; i++) {
        if(stage->tevStage[i]) {
            tevStage[numChildren] = stage->tevStage[i];
            numChildren++;
        }
    }
    if (numChildren) {
        numNotFound = numChildren;
        for(i = 0; i < 16; i++) {
            for(j = 0; j < numChildren; j++) {
                if (ColorInstructions[i] == tevStage[j] || AlphaInstructions[i] == tevStage[j]) {
                    numNotFound--;
                    if (numNotFound == 0) {
                        return i + 1;
                    }
                }
            }
        }
    }
    return 0;
}

static void RemoveInstructionFromSIList(u8 index, struct ShaderTEVStage * * SIList, u8 * numSI) {
    u8 i;

    for(i = index; i < (*numSI - 1); i++) {
        SIList[i] = SIList[i + 1];
    }
    *numSI-=1;
}

static void RemoveInstructionFromSIList2(struct ShaderTEVStage * stage, struct ShaderTEVStage * * SIList, u8 * numSI) {
    u8 i;

    for(i = 0; i < (*numSI); i++) {
        if (SIList[i] == stage) {
            break;
        }
    }
    for(i; i < (*numSI - 1); i++) {
        SIList[i] = SIList[i + 1];
    }
    *numSI-=1;
}

static void AddInstructionToSIList(struct ShaderTEVStage * instruction, struct ShaderTEVStage * * SIList, u8 * numSI) {
    SIList[*numSI] = instruction;
    *numSI+=1;
}

static void AddInstructionToSIList2(struct ShaderTEVStage * instruction, u8 index, struct ShaderTEVStage * * SIList, u8 * numSI) {
    struct ShaderTEVStage * temp;
    u8 i;

    *numSI+=1;
    for(i = index; i < *numSI; i++) {
        temp = SIList[i];
        SIList[i] = instruction;
        instruction = temp;
    }
}

static u8 CheckParallelList(struct ShaderTEVStage * stage1, struct ShaderTEVStage * stage2) {
    if (stage2 == 0) {
        return 1;
    }
    if ((stage1->texGenIdx != 0x63) && (stage2->texGenIdx != 0x63) && (stage2->texGenIdx != stage1->texGenIdx)) {
        return 0;
    }
    if ((stage1->texInput != 8) && (stage2->texInput != 8) && (stage2->texInput != stage1->texInput)) {
        return 0;
    }
    if ((stage1->rasInput != 2) && (stage2->rasInput != 2) && (stage2->rasInput != stage1->rasInput)) {
        return 0;
    }
    if (stage1->mode != stage2->mode) {
        return 0;
    }
    return 1;
}

static void PadInstructionList(void) {
    u8 i;

    for(i = 0; i < 16; i++) {
        if (ColorInstructions[i] == 0 && AlphaInstructions[i] == 0) {
            break;
        }
        if (ColorInstructions[i] == 0  && AlphaInstructions[i]->mode) {
            ColorInstructions[i] = &TEVPool[TEVCounter++];
        }
        if (AlphaInstructions[i] == 0  && ColorInstructions[i]->mode) {
            AlphaInstructions[i] = &TEVPool[TEVCounter++];
        }
    }
}

static void UnPadInstructionList(void) {
    u8 i;

    for(i = 0; i < 16; i++) {
        if (ColorInstructions[i] == 0 && AlphaInstructions[i] == 0) {
            break;
        }
        if (ColorInstructions[i] && ColorInstructions[i]->type == 6) {
            ColorInstructions[i] = 0;
            TEVCounter--;
        }
        if (AlphaInstructions[i]  && AlphaInstructions[i]->type == 6) {
            AlphaInstructions[i] = 0;
            TEVCounter--;
        }
    }
}

static u8 AddRootAlphaInstruction(struct ShaderTEVStage * stage) {
    u8 startPoint;
    u8 i;

    startPoint = FindLastAlphaInstruction();
    if (startPoint == 0x63) {
        startPoint = 0;
    } else {
        startPoint++;
    }
    for(i = startPoint; i < 16; i++) {
        if (AlphaInstructions[i] == 0) {
            if (CheckParallelList(stage, ColorInstructions[i])) {
                AlphaInstructions[i] = stage;
                return i;
            }
        }
    }
    return 0x63;
}

static u8 FindLastAlphaInstruction(void) {
    u8 last;
    u8 i;

    last = 0x63;
    for(i = 0; i < 16; i++) {
        if (AlphaInstructions[i]) {
            last = i;
        }
    }
    return last;
}

void CopyCompiledTEVStages(SHDRStage *compiled, struct ShaderTEVStage * instructions, u32 numStages, SHDRRas *resources) {
    u32 i;
    u32 j;

    for(i = 0; i < numStages; i++) {
        for(j = 0; j < 4; j++) {
            compiled[i].TEVArg[j] = instructions[i].TEVArg[j];
            if (compiled[i].TEVArg[j] >= 25u && compiled[i].TEVArg[j] <= 42u) {
                if (compiled[i].TEVArg[j] <= 32u) {
                    resources->complexUsed[compiled[i].TEVArg[j] - SHADER_COMPLEXINPUT0_RGB] = 1;
                } else {
                    resources->complexUsed[compiled[i].TEVArg[j] - SHADER_COMPLEXINPUT0_A] = 1;
                }
            }
        }
        compiled[i].op = instructions[i].op;
        compiled[i].bias = instructions[i].bias;
        compiled[i].scale = instructions[i].scale;
        compiled[i].clamp = instructions[i].clamp;
        compiled[i].mode = instructions[i].mode;
        compiled[i].out_reg = instructions[i].out_reg;
        compiled[i].rasInput = instructions[i].rasInput;
        compiled[i].texGenIdx = instructions[i].texGenIdx;
        compiled[i].texInput = instructions[i].texInput;
        if (compiled[i].rasInput != 2) {
            resources->rasUsed[compiled[i].rasInput] = 1;
        }
        if (compiled[i].texInput != 8) {
            resources->textureUsed[compiled[i].texInput] = 1;
        }
    }
}
