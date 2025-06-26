#include "RFFEAnalyzerSettings.h"
#include "RFFEUtil.h"
#include <AnalyzerHelpers.h>

typedef struct {
    U32 simulationSampleRateHz;
    RFFEAnalyzerSettings *settings;
    BitState *sclk;
    BitState *sdata;
    ClockGenerator clockGenerator;
    U8 LSFRData;
} RFFESimulationDataGenerator;

void RFFESimulationDataGenerator_Init(RFFESimulationDataGenerator *generator, U32 simulationSampleRate, RFFEAnalyzerSettings *settings) {
    generator->simulationSampleRateHz = simulationSampleRate;
    generator->settings = settings;
    generator->LSFRData = 0xE1; // Must be non-zero

    ClockGenerator_Init(&generator->clockGenerator, simulationSampleRate / 10, simulationSampleRate);

    if (settings->mSclkChannel != UNDEFINED_CHANNEL) {
        generator->sclk = RFFEUtil_AddChannel(settings->mSclkChannel, generator->simulationSampleRateHz, BIT_LOW);
    } else {
        generator->sclk = NULL;
    }

    if (settings->mSdataChannel != UNDEFINED_CHANNEL) {
        generator->sdata = RFFEUtil_AddChannel(settings->mSdataChannel, generator->simulationSampleRateHz, BIT_LOW);
    } else {
        generator->sdata = NULL;
    }

    // insert 10 bit-periods of idle
    RFFEUtil_AdvanceAll(generator->sclk, generator->sdata, ClockGenerator_AdvanceByHalfPeriod(&generator->clockGenerator, 10.0));
}

U32 RFFESimulationDataGenerator_GenerateSimulationData(RFFESimulationDataGenerator *generator, U64 largestSampleRequested, U32 sampleRate, SimulationChannelDescriptor **simulationChannels) {
    U64 adjustedLargestSampleRequested = AnalyzerHelpers_AdjustSimulationTargetSample(largestSampleRequested, sampleRate, generator->simulationSampleRateHz);
    
    while (generator->sclk->GetCurrentSampleNumber() < adjustedLargestSampleRequested) {
        CreateRffeTransaction(generator);
        RFFEUtil_AdvanceAll(generator->sclk, generator->sdata, ClockGenerator_AdvanceByHalfPeriod(&generator->clockGenerator, 80.0));
    }

    *simulationChannels = RFFEUtil_GetChannelArray();
    return RFFEUtil_GetChannelCount();
}

void CreateRffeTransaction(RFFESimulationDataGenerator *generator) {
    U8 cmd;
    U8 sa_addrs[] = {0x5};

    for (U32 adr = 0; adr < sizeof(sa_addrs) / sizeof(sa_addrs[0]); adr++) {
        for (U32 i = 0; i < 256; i++) {
            CreateSSC(generator);
            CreateSlaveAddress(generator, sa_addrs[adr]);

            cmd = i & 0xff;
            CreateByteFrame(generator, cmd);

            switch (RFFEUtil_decodeRFFECmdFrame(cmd)) {
                case RFFEAnalyzerResults_RffeTypeExtWrite:
                    CreateByteFrame(generator, CreateRandomData(generator));
                    for (U32 j = 0; j <= RFFEUtil_byteCount(cmd); j++) {
                        CreateByteFrame(generator, CreateRandomData(generator));
                    }
                    CreateBusPark(generator);
                    break;
                case RFFEAnalyzerResults_RffeTypeReserved:
                    CreateBusPark(generator);
                    break;
                case RFFEAnalyzerResults_RffeTypeMasterRead:
                    CreateByteFrame(generator, CreateRandomData(generator));
                    CreateBusPark(generator);
                    for (U32 j = 0; j <= RFFEUtil_byteCount(cmd); j++) {
                        CreateByteFrame(generator, CreateRandomData(generator));
                    }
                    CreateBusPark(generator);
                    break;
                case RFFEAnalyzerResults_RffeTypeMasterWrite:
                    CreateByteFrame(generator, CreateRandomData(generator));
                    for (U32 j = 0; j <= RFFEUtil_byteCount(cmd); j++) {
                        CreateByteFrame(generator, CreateRandomData(generator));
                    }
                    CreateBusPark(generator);
                    break;
                case RFFEAnalyzerResults_RffeTypeMasterHandoff:
                    CreateBusPark(generator);
                    for (U32 j = 0; j <= RFFEUtil_byteCount(cmd); j++) {
                        CreateByteFrame(generator, (CreateRandomData(generator) & 0x18) | 0x80); // Mask Out bits that are always zero and always set the ACK bit
                    }
                    CreateBusPark(generator);
                    break;
                case RFFEAnalyzerResults_RffeTypeInterrupt:
                    CreateBusPark(generator);
                    CreateBits(generator, 2, 0x3); // Always indicate interrupts
                    CreateBusPark(generator);
                    for (U32 j = 0; j < 16; j++) {
                        CreateBits(generator, 2, CreateRandomData(generator) & 0x2); // Interrupts are 1 bit of interrupt followed by a 0/BP
                    }
                    break;
                case RFFEAnalyzerResults_RffeTypeExtRead:
                    CreateByteFrame(generator, CreateRandomData(generator));
                    CreateBusPark(generator);
                    for (U32 j = 0; j <= RFFEUtil_byteCount(cmd); j++) {
                        CreateByteFrame(generator, CreateRandomData(generator));
                    }
                    CreateBusPark(generator);
                    break;
                case RFFEAnalyzerResults_RffeTypeExtLongWrite:
                    CreateByteFrame(generator, CreateRandomData(generator));
                    CreateByteFrame(generator, CreateRandomData(generator));
                    for (U32 j = 0; j <= RFFEUtil_byteCount(cmd); j++) {
                        CreateByteFrame(generator, CreateRandomData(generator));
                    }
                    CreateBusPark(generator);
                    break;
                case RFFEAnalyzerResults_RffeTypeExtLongRead:
                    CreateByteFrame(generator, CreateRandomData(generator));
                    CreateByteFrame(generator, CreateRandomData(generator));
                    CreateBusPark(generator);
                    for (U32 j = 0; j <= RFFEUtil_byteCount(cmd); j++) {
                        CreateByteFrame(generator, CreateRandomData(generator));
                    }
                    CreateBusPark(generator);
                    break;
                case RFFEAnalyzerResults_RffeTypeNormalWrite:
                    CreateByteFrame(generator, CreateRandomData(generator));
                    CreateBusPark(generator);
                    break;
                case RFFEAnalyzerResults_RffeTypeNormalRead:
                    CreateBusPark(generator);
                    CreateByteFrame(generator, CreateRandomData(generator));
                    CreateBusPark(generator);
                    break;
                case RFFEAnalyzerResults_RffeTypeWrite0:
                    CreateBusPark(generator);
                    break;
            }
        }
    }
}

void CreateSSC(RFFESimulationDataGenerator *generator) {
    if (generator->sclk->GetCurrentBitState() == BIT_HIGH) {
        generator->sclk->Transition();
    }
    
    if (generator->sdata->GetCurrentBitState() == BIT_HIGH) {
        generator->sdata->Transition();
    }

    RFFEUtil_AdvanceAll(generator->sclk, generator->sdata, ClockGenerator_AdvanceByHalfPeriod(&generator->clockGenerator, 2.0));

    // sdata pulse for 1-clock cycle
    generator->sdata->Transition();
    RFFEUtil_AdvanceAll(generator->sclk, generator->sdata, ClockGenerator_AdvanceByHalfPeriod(&generator->clockGenerator, 2.0));
    generator->sdata->Transition();
    
    // sdata and sclk state low for 1-clock cycle
    RFFEUtil_AdvanceAll(generator->sclk, generator->sdata, ClockGenerator_AdvanceByHalfPeriod(&generator->clockGenerator, 2.0));
}

void CreateSlaveAddress(RFFESimulationDataGenerator *generator, U8 addr) {
    CreateBits(generator, 4, addr & 0x0f);
}

void CreateByteFrame(RFFESimulationDataGenerator *generator, U8 byte) {
    CreateByte(generator, byte);
    CreateParity(generator, byte);
}

void CreateByte(RFFESimulationDataGenerator *generator, U8 cmd) {
    CreateBits(generator, 8, cmd);
}

void CreateParity(RFFESimulationDataGenerator *generator, U8 byte) {
    CreateBits(generator, 1, ParityTable256[byte]); // Use a different algorithm for generating parity than we have for checking it
}

void CreateBusPark(RFFESimulationDataGenerator *generator) {
    CreateBits(generator, 1, 0);
}

void CreateBits(RFFESimulationDataGenerator *generator, U8 bits, U8 cmd) {
    BitState bit;
    U8 idx = 0x1 << (bits - 1);
    
    for (U32 i = 0; i < bits; i++) {
        generator->sclk->Transition();
        RFFEUtil_AdvanceAll(generator->sclk, generator->sdata, ClockGenerator_AdvanceByHalfPeriod(&generator->clockGenerator, 0.5));
        
        if (cmd & idx) {
            bit = BIT_HIGH;
        } else {
            bit = BIT_LOW;
        }
        
        generator->sdata->TransitionIfNeeded(bit);
        
        RFFEUtil_AdvanceAll(generator->sclk, generator->sdata, ClockGenerator_AdvanceByHalfPeriod(&generator->clockGenerator, 0.5));
        generator->sclk->Transition();
        
        RFFEUtil_AdvanceAll(generator->sclk, generator->sdata, ClockGenerator_AdvanceByHalfPeriod(&generator->clockGenerator, 1.0));
        
        idx = idx >> 0x1;
    }
}

U8 CreateRandomData(RFFESimulationDataGenerator *generator) {
    U8 lsb;

    lsb = generator->LSFRData & 0x1;
    generator->LSFRData >>= 1;
    if (lsb) {
        generator->LSFRData ^= 0xB8; // Maximum period polynomial for Galois LFSR of 8 bits
    }
    return generator->LSFRData;
}

// 后续的定义与初始化工作, 带有相应的赋值等处理。
