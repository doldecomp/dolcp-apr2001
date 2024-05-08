#include <dolphin.h>
#include "charPipeline.h"

static struct ShaderTEVStage * Children[32];
static struct ShaderTEVStage * Parents[32];
static u8 CollapseIndex[32];
static struct ShaderTEVStage * OptimalColorStages[16];
static struct ShaderTEVStage * OptimalAlphaStages[16];
static struct ShaderTEVStage * CopyList[33];
static struct ShaderTEVStage * AddList[32];
struct ShaderTEVStage CollapsedTEVPool[33];

static u8 NumCandidates;
static u8 BestAlpha;
static u8 BestColor;
static u8 AddListCounter;
u8 OptimalTreeSize;

// functions
static void PruneTEVStages(struct ShaderTEVStage * stage);
static void FindLERPTypes(struct ShaderTEVStage * stage, u8 recursion);
static u8 DetermineNonTrivialLERParams(struct ShaderTEVStage * stage);
static u8 DetermineSimpleTEVType(struct ShaderTEVStage * stage);
static enum SHADER_COLOR_TYPE DetermineTrivialTEVType(struct ShaderTEVStage * stage);
static void ReParentTEVStage(struct ShaderTEVStage * oldParent, struct ShaderTEVStage * newParent, struct ShaderTEVStage * current);
static void CreateCollapseList(struct ShaderTEVStage * stage, struct ShaderTEVStage * parent, u8 index);
static void CollapseRecursive(void);
static void CollapseTEVStageTRIVIAL(struct ShaderTEVStage *stage, struct ShaderTEVStage *parent, u8 index);
static void CollapseTEVStageSIMPLE(struct ShaderTEVStage *stage, struct ShaderTEVStage *parent, u8 index);
static void CollapseTEVStageLERP(struct ShaderTEVStage *stage, struct ShaderTEVStage *parent);
static void RemoveStageFromLists(u8 index);
static void AddStageToLists(u8 index, struct ShaderTEVStage * stage, struct ShaderTEVStage * parent, u8 newIndex);
static void AssignMiscParameters(struct ShaderTEVStage * stage, struct ShaderTEVStage * parent);
static void DetachStage(struct ShaderTEVStage * stage, struct ShaderTEVStage * parent);
static void AttachStage(struct ShaderTEVStage * stage, struct ShaderTEVStage * parent);
static u8 ReParentTEVStageInCollapseList(struct ShaderTEVStage * oldParent, struct ShaderTEVStage * newParent, struct ShaderTEVStage * current, u8 newIndex);
static void FindNumOptimalTEVStages(struct ShaderTEVStage * stage);
static void CreateAddList(struct ShaderTEVStage * stage);
static void PermuteTree(void);
static void SetUpFlatten(struct ShaderTEVStage * stage);

void PruneTEVTree(void) {
    u8 i;

    PruneTEVStages(TEVPool);
    FindLERPTypes(TEVPool, 1);
    AddListCounter = 0;    
    for(i = 0; i < 32; i++) {
        AddList[i] = 0;
    }
    CreateAddList(TEVPool);
    OptimalTreeSize = 0x63;
    PermuteTree();
}

static void PruneTEVStages(struct ShaderTEVStage * stage) {
    u8 i;

    if (stage->TEVArg[2] == SHADER_ZERO) {
        stage->TEVArg[1] = SHADER_ZERO;
        stage->tevStage[1] = NULL;
    } else if (stage->TEVArg[2] == SHADER_ONE) {
        stage->TEVArg[0] = SHADER_ZERO;
        stage->tevStage[0] = NULL;
    } else if ((stage->TEVArg[0] == SHADER_ZERO) && (stage->TEVArg[1] == SHADER_ZERO)) {
        stage->TEVArg[2] = SHADER_ZERO;
        stage->tevStage[2] = NULL;
    } else if ((stage->TEVArg[0] == SHADER_ONE) && (stage->TEVArg[1] == SHADER_ONE)) {
        stage->TEVArg[2] = SHADER_ZERO;
        stage->tevStage[2] = NULL;
    }
    for(i = 0; i < 4; i++) {
        if (stage->tevStage[i]) {
            PruneTEVStages(stage->tevStage[i]);
        }
    }
}

static void FindLERPTypes(struct ShaderTEVStage * stage, u8 recursion) {
    u8 i;
    u8 numNonTrivialParams;

    if (stage->type == SHADER_TYPE_COMPLEX) {
        numNonTrivialParams = DetermineNonTrivialLERParams(stage);
        if (numNonTrivialParams > 1) {
            stage->LERPType = SHADER_LERP;
        } else if (numNonTrivialParams != 0) {
            stage->LERPType = SHADER_SIMPLE;
            stage->outputIndex = DetermineSimpleTEVType(stage);
        } else {
            stage->LERPType = SHADER_TRIVIAL;
            stage->outputArg = DetermineTrivialTEVType(stage);
        }
    }
    if (recursion) {
        for(i = 0; i < 4; i++) {
            if(stage->tevStage[i]) {
                FindLERPTypes(stage->tevStage[i], 1);
            }
        }
    }
}

static u8 DetermineNonTrivialLERParams(struct ShaderTEVStage * stage) {
    u8 count;
    u8 i;

    count = 0;
    for(i = 0; i < 3; i++) {
        if (stage->tevStage[i]) {
            count++;
        } else if(stage->TEVArg[i] != SHADER_ONE && stage->TEVArg[i] != SHADER_ZERO) {
            count++;
        }
    }
    return count;
}

static u8 DetermineSimpleTEVType(struct ShaderTEVStage * stage) {
    u8 i;

    for(i = 0; i < 3; i++) {
        if(stage->TEVArg[i] < SHADER_NONTRIVIAL) {
            return i;
        }
    }
    return 0;
}

static enum SHADER_COLOR_TYPE DetermineTrivialTEVType(struct ShaderTEVStage * stage) {
    if ((stage->TEVArg[0] == SHADER_ZERO) && (stage->TEVArg[1] == SHADER_ZERO)) {
        return SHADER_ZERO;
    }
    return SHADER_ONE;    
}

static void ReParentTEVStage(struct ShaderTEVStage * oldParent, struct ShaderTEVStage * newParent, struct ShaderTEVStage * current) {
    u32 i;

    if (current) {
        for(i = 0; i < 8; i++) {
            if (current->parent[i] == oldParent) {
                current->parent[i] = newParent;
                return;
            }
        }
        OSPanic(__FILE__, 0x175, "parent never found in shader prune TEV tree!!!");
    }
}

static void CreateCollapseList(struct ShaderTEVStage * stage, struct ShaderTEVStage * parent, u8 index) {
    u8 i;

    for(i = 0; i < 4; i++) {
        if (stage->tevStage[i] && stage->tevStage[i]->type == 5) {
            CreateCollapseList(stage->tevStage[i], stage, i);
        }
    }
    if ((parent) && (parent->type != 6)) {
        if (stage->op == 0) {
            switch(stage->LERPType) {
                case 0:
                    if (stage->outputArg == 0xC9) {
                        if (CheckResourceCollision(stage, parent) == 0) {
                            return;
                        }
                        break;
                    }
                    
                    if ((stage->TEVArg[3] == 0xC9)) {
                        if (CheckResourceCollision(stage, parent) == 0) {
                            return;
                        }

                        break;
                    }
                    
                    return;
                case 1:
                    if ((stage->TEVArg[3] == 0xC9)) {
                        if (CheckResourceCollision(stage, parent) == 0) {
                            return;
                        }

                        break;
                    }
                    
                    return;
                case 2:
                    if ((stage->TEVArg[3] == 0xC9)) {
                        if ((parent->LERPType == 1)) {
                            if ((parent->tevStage[3] != stage)) {
                                if (CheckResourceCollision(stage, parent) == 0) {
                                    return;
                                }
                                
                                break;
                            }
                        }
                    }
                    
                    return;
            }
        }

        for(i = 0; i < NumCandidates; i++) {
            if (Children[i] == stage && Parents[i] == parent) {
                return;
            }
        }
        Children[NumCandidates] = stage;
        Parents[NumCandidates] = parent;
        CollapseIndex[NumCandidates] = index;
        NumCandidates++;
        ASSERTMSGLINE(0x1BE, NumCandidates < 32, "collapse problems in CreateCollapseList");
    }
}

static void CollapseRecursive(void) {
    u8 i;
    struct ShaderTEVStage * stage;
    struct ShaderTEVStage * parent;
    u8 index;

    if (NumCandidates != 0) {
        for(i = 0; i < NumCandidates; i++) {
            stage = Children[i];
            parent = Parents[i];
            index = CollapseIndex[i];
            RemoveStageFromLists(i);
            if (stage->op == 0) {
                switch(stage->LERPType) {
                    case 0:
                        CollapseTEVStageTRIVIAL(stage, parent, index);
                        break;
                    case 1:
                        CollapseTEVStageSIMPLE(stage, parent, index);
                        break;
                    case 2:
                        CollapseTEVStageLERP(stage, parent);
                        break;
                }
            }
            AddStageToLists(i, stage, parent, index);
        }
        return;
    }
    BestColor = BestAlpha = 0;
    for(i = 0; i < 16; i++) {
        OptimalColorStages[i] = 0;
        OptimalAlphaStages[i] = 0;
    }
    FindNumOptimalTEVStages(TEVPool);
    if (BestAlpha > BestColor) {
        BestColor = BestAlpha;
    }
    OptimalTreeSize = BestColor;
    if (OptimalTreeSize < BestNumTEVStages) {
        SetUpFlatten(TEVPool);
        FlattenTEVTree();
    }
}

static void CollapseTEVStageTRIVIAL(struct ShaderTEVStage *stage, struct ShaderTEVStage *parent, u8 index) {
    u8 whichCollapse;
    enum SHADER_COLOR_TYPE oldARG;
    struct ShaderTEVStage * oldStage;
    enum SHADER_LERP_TYPE oldLERPType;
    u8 oldCoord;
    enum SHADER_TEX oldMap;
    enum SHADER_RAS oldColor;
    u8 oldIdx;

    whichCollapse = 0;

    if (stage->outputArg == 0xC9) {
        if (CheckResourceCollision(stage, parent) != 0) {
            oldARG = parent->TEVArg[index];
            oldStage = parent->tevStage[index];
            oldLERPType = parent->LERPType;
            ReParentTEVStage(stage, parent, stage->tevStage[3]);
            oldIdx = ReParentTEVStageInCollapseList(stage, parent, stage->tevStage[3], index);
            parent->TEVArg[index] = stage->TEVArg[3];
            parent->tevStage[index] = stage->tevStage[3];
            FindLERPTypes(parent, 0);
            DetachStage(stage, parent);
            whichCollapse = 1;
        }
    } else if ((stage->TEVArg[3] == 0xC9) && (CheckResourceCollision(stage, parent) != 0)) {
        oldARG = parent->TEVArg[index];
        oldStage = parent->tevStage[index];
        oldLERPType = parent->LERPType;
        parent->TEVArg[index] = stage->outputArg;
        parent->tevStage[index] = NULL;
        FindLERPTypes(parent, 0);
        DetachStage(stage, parent);
        whichCollapse = 2;
    }
    if (whichCollapse != 0) {
        oldCoord = parent->texGenIdx;
        oldMap = parent->texInput;
        oldColor = parent->rasInput;
        AssignMiscParameters(stage, parent);
    }
    CollapseRecursive();
    if (whichCollapse != 0) {
        if (whichCollapse == 1) {
            ReParentTEVStage(parent, stage, stage->tevStage[3]);
            if (oldIdx != 0x63) {
                ReParentTEVStageInCollapseList(parent, stage, stage->tevStage[3], oldIdx);
            }
        }
        parent->TEVArg[index] = oldARG;
        parent->tevStage[index] = oldStage;
        parent->LERPType = oldLERPType;
        parent->texGenIdx = oldCoord;
        parent->texInput = oldMap;
        parent->rasInput = oldColor;
        AttachStage(stage, parent);
    }
}

static void CollapseTEVStageSIMPLE(struct ShaderTEVStage *stage, struct ShaderTEVStage *parent, u8 index) {
    u8 whichCollapse;
    enum SHADER_COLOR_TYPE oldARG;
    struct ShaderTEVStage * oldStage;
    enum SHADER_LERP_TYPE oldLERPType;
    u8 oldCoord;
    enum SHADER_TEX oldMap;
    enum SHADER_RAS oldColor;
    u8 oldIdx;

    whichCollapse = 0;
    if ((stage->TEVArg[3] == 0xC9) && (CheckResourceCollision(stage, parent) != 0)) {
        oldARG = parent->TEVArg[index];
        oldStage = parent->tevStage[index];
        oldLERPType = parent->LERPType;
        oldCoord = parent->texGenIdx;
        oldMap = parent->texInput;
        oldColor = parent->rasInput;
        AssignMiscParameters(stage, parent);
        ReParentTEVStage(stage, parent, stage->tevStage[stage->outputIndex]);
        oldIdx = ReParentTEVStageInCollapseList(stage, parent, stage->tevStage[stage->outputIndex], index);
        DetachStage(stage, parent);
        parent->TEVArg[index] = stage->TEVArg[stage->outputIndex];
        parent->tevStage[index] = stage->tevStage[stage->outputIndex];
        FindLERPTypes(parent, 0);
        whichCollapse = 1;
    }
    CollapseRecursive();
    if (whichCollapse != 0) {
        ReParentTEVStage(parent, stage, stage->tevStage[stage->outputIndex]);
        if (oldIdx != 0x63) {
            ReParentTEVStageInCollapseList(parent, stage, stage->tevStage[stage->outputIndex], oldIdx);
        }
        parent->TEVArg[index] = oldARG;
        parent->tevStage[index] = oldStage;
        parent->LERPType = oldLERPType;
        parent->texGenIdx = oldCoord;
        parent->texInput = oldMap;
        parent->rasInput = oldColor;
        AttachStage(stage, parent);
    }
}

static void CollapseTEVStageLERP(struct ShaderTEVStage *stage, struct ShaderTEVStage *parent) {
    u8 whichCollapse;
    enum SHADER_COLOR_TYPE oldARG[3];
    struct ShaderTEVStage * oldStage[3];
    enum SHADER_LERP_TYPE oldLERPType;
    u8 oldCoord;
    enum SHADER_TEX oldMap;
    enum SHADER_RAS oldColor;
    u8 oldIdx[3];

    whichCollapse = 0;
    if ((stage->TEVArg[3] == 0xC9) && (parent->LERPType == 1) && (parent->tevStage[3] != stage) && (CheckResourceCollision(stage, parent) != 0)) {
        oldARG[0] = parent->TEVArg[0];
        oldStage[0] = parent->tevStage[0];
        oldARG[1] = parent->TEVArg[1];
        oldStage[1] = parent->tevStage[1];
        oldARG[2] = parent->TEVArg[2];
        oldStage[2] = parent->tevStage[2];
        oldLERPType = parent->LERPType;
        oldCoord = parent->texGenIdx;
        oldMap = parent->texInput;
        oldColor = parent->rasInput;
        AssignMiscParameters(stage, parent);
        ReParentTEVStage(stage, parent, stage->tevStage[0]);
        ReParentTEVStage(stage, parent, stage->tevStage[1]);
        ReParentTEVStage(stage, parent, stage->tevStage[2]);
        oldIdx[0] = ReParentTEVStageInCollapseList(stage, parent, stage->tevStage[0], 0);
        oldIdx[1] = ReParentTEVStageInCollapseList(stage, parent, stage->tevStage[1], 1);
        oldIdx[2] = ReParentTEVStageInCollapseList(stage, parent, stage->tevStage[2], 2);
        parent->TEVArg[0] = stage->TEVArg[0];
        parent->tevStage[0] = stage->tevStage[0];
        parent->TEVArg[1] = stage->TEVArg[1];
        parent->tevStage[1] = stage->tevStage[1];
        parent->TEVArg[2] = stage->TEVArg[2];
        parent->tevStage[2] = stage->tevStage[2];
        FindLERPTypes(parent, 0);
        DetachStage(stage, parent);
        whichCollapse = 1;
    }
    CollapseRecursive();
    if (whichCollapse != 0) {
        ReParentTEVStage(parent, stage, stage->tevStage[0]);
        ReParentTEVStage(parent, stage, stage->tevStage[1]);
        ReParentTEVStage(parent, stage, stage->tevStage[2]);
        if (oldIdx[0] != 0x63) {
            ReParentTEVStageInCollapseList(parent, stage, stage->tevStage[0], oldIdx[0]);
        }
        if (oldIdx[1] != 0x63) {
            ReParentTEVStageInCollapseList(parent, stage, stage->tevStage[1], oldIdx[1]);
        }
        if (oldIdx[2] != 0x63) {
            ReParentTEVStageInCollapseList(parent, stage, stage->tevStage[2], oldIdx[2]);
        }
        parent->TEVArg[0] = oldARG[0];
        parent->tevStage[0] = oldStage[0];
        parent->TEVArg[1] = oldARG[1];
        parent->tevStage[1] = oldStage[1];
        parent->TEVArg[2] = oldARG[2];
        parent->tevStage[2] = oldStage[2];
        parent->LERPType = oldLERPType;
        parent->texGenIdx = oldCoord;
        parent->texInput = oldMap;
        parent->rasInput = oldColor;
        AttachStage(stage, parent);
    }
}

static void RemoveStageFromLists(u8 index) {
    u8 i;

    for(i = index; i < (NumCandidates - 1); i++) {
        Children[i] = Children[i+1];
        Parents[i] = Parents[i+1];
        CollapseIndex[i] = CollapseIndex[i+1];
    }
    NumCandidates -= 1;
}

static void AddStageToLists(u8 index, struct ShaderTEVStage * stage, struct ShaderTEVStage * parent, u8 newIndex) {
    struct ShaderTEVStage * tempStage;
    struct ShaderTEVStage * tempParent;
    u8 tempIndex;
    u8 i;

    NumCandidates++;

    for(i = index; i < NumCandidates; i++) {
        tempStage = Children[i];
        tempParent = Parents[i];
        tempIndex = CollapseIndex[i];
        Children[i] = stage;
        Parents[i] = parent;
        CollapseIndex[i] = newIndex;
        stage = tempStage;
        parent = tempParent;
        newIndex = tempIndex;
    }
}

static void AssignMiscParameters(struct ShaderTEVStage * stage, struct ShaderTEVStage * parent) {
    if (stage->texGenIdx != 0x63) {
        parent->texGenIdx = stage->texGenIdx;
    }
    if (stage->texInput != 8) {
        parent->texInput = stage->texInput;
    }
    if (stage->rasInput != 2) {
        parent->rasInput = stage->rasInput;
    }
}

static void DetachStage(struct ShaderTEVStage * stage, struct ShaderTEVStage * parent) {
    u32 i;

    for(i = 0; i < stage->numParents; i++) {
        if (stage->parent[i] == parent) {
            break;
        }
    }
    for(i; i < (stage->numParents - 1); i++) {
        stage->parent[i] = stage->parent[i+1];
    }
    stage->numParents--;
    stage->parent[stage->numParents] = 0;
}

static void AttachStage(struct ShaderTEVStage * stage, struct ShaderTEVStage * parent) {
    stage->parent[stage->numParents] = parent;
    stage->numParents++;
}

static u8 ReParentTEVStageInCollapseList(struct ShaderTEVStage * oldParent, struct ShaderTEVStage * newParent, struct ShaderTEVStage * current, u8 newIndex) {
    u32 i;
    u8 temp;

    if (current == 0) {
        return 0x63;
    }
    for(i = 0; i < 32; i++) {
        if (Children[i] == current && Parents[i] == oldParent) {
            Parents[i] = newParent;
            temp = CollapseIndex[i];
            CollapseIndex[i] = newIndex;
            return temp;
        }
        if (Children[i] == 0) {
            return 0x63;
        }
    }
    return 0x63;
}

static void FindNumOptimalTEVStages(struct ShaderTEVStage * stage) {
    u8 i;

    if (stage->type != 6) {
        if (stage->channel == 0) {
            for(i = 0; i < 16; i++) {
                if (stage == OptimalColorStages[i]) {
                    break;
                }
                if (OptimalColorStages[i] == 0) {
                    OptimalColorStages[i] = stage;
                    BestColor++;
                    break;
                }
            }
        } else {
            for(i = 0; i < 16; i++) {
                if (stage == OptimalAlphaStages[i]) {
                    break;
                }
                if (OptimalAlphaStages[i] == 0) {
                    OptimalAlphaStages[i] = stage;
                    BestAlpha++;
                    break;
                }
            }
        }
    }
    if ((stage->type == 5) || (stage->type == 6)) {
        for(i = 0; i < 4; i++) {
            if(stage->tevStage[i]) {
                FindNumOptimalTEVStages(stage->tevStage[i]);
            }
        }
    }
}

static void CreateAddList(struct ShaderTEVStage * stage) {
    u8 i;

    for(i = 0; i < 4; i++) {
        if (stage->tevStage[i]) {
            CreateAddList(stage->tevStage[i]);
        }
    }
    if ((stage->op == 0) && (stage->TEVArg[0] != 0xC9) && (stage->TEVArg[3] != 0xC9) && (stage->TEVArg[1] == 0xC9) && (stage->TEVArg[2] == 0xC9)) {
        for(i = 0; i < AddListCounter; i++) {
            if (AddList[i] == stage) {
                return;
            }
        }
        AddList[AddListCounter] = stage;
        AddListCounter++;
    }
}

static void PermuteTree(void) {
    enum SHADER_COLOR_TYPE tempArg;
    struct ShaderTEVStage * tempStage;
    struct ShaderTEVStage * stage;

    if (AddListCounter != 0) {
        --AddListCounter;
        PermuteTree();
        stage = AddList[AddListCounter];
        tempArg = stage->TEVArg[0];
        tempStage = stage->tevStage[0];
        stage->TEVArg[0] = stage->TEVArg[3];
        stage->tevStage[0] = stage->tevStage[3];
        stage->TEVArg[3] = tempArg;
        stage->tevStage[3] = tempStage;
        PermuteTree();
        AddListCounter += 1;
        return;
    }
    NumCandidates = 0;
    CreateCollapseList(TEVPool, NULL, 0U);
    CollapseRecursive();
}

static void SetUpFlatten(struct ShaderTEVStage * stage) {
    u8 i;

    if (stage) {
        stage->numNonAllocatedChildren = 0;
        for(i = 0; i < 4; i++) {
            SetUpFlatten(stage->tevStage[i]);
        }
    }
}
