
#include "RFFEAnalyzer.h"
#include "RFFEAnalyzerSettings.h"
#include "RFFEUtil.h"
#include <AnalyzerChannelData.h>

// 结构体定义，替换类
typedef struct RFFEAnalyzer_t {
  Analyzer2 super;
  RFFEAnalyzerSettings* mSettings;
  bool mSimulationInitilized;
  BitState mSclkCurrent;
  BitState mSclkPrevious;
  BitState mSdataCurrent;
  BitState mSdataPrevious;
  bool mUnexpectedSSC;
  U64 mUnexpectedSSCStart;
  U64 mSamplePosition;
  U64 mSampleClkOffsets[16];
  U64 mSampleDataOffsets[16];
  AnalyzerResults::MarkerType mSampleMarker[16];
  RFFEAnalyzerResults* mResults;
  AnalyzerChannelData* mSdata;
  AnalyzerChannelData* mSclk;
  U32 mSampleRateHz;
  RFFEAnalyzerResults::RffeFrameType mRffeCmdType;
  RFFEAnalyzerSimulationDataGenerator mSimulationDataGenerator;
} RFFEAnalyzer;


// 函数声明，替换类成员函数
void RFFEAnalyzer_SetupResults(RFFEAnalyzer* self);
void RFFEAnalyzer_WorkerThread(RFFEAnalyzer* self);
U8 RFFEAnalyzer_FindStartSeqCondition(RFFEAnalyzer* self);
U8 RFFEAnalyzer_FindCommandFrame(RFFEAnalyzer* self);
U8 RFFEAnalyzer_FindISI(RFFEAnalyzer* self);
void RFFEAnalyzer_FindInterruptSlots(RFFEAnalyzer* self);
void RFFEAnalyzer_FindByteFrame(RFFEAnalyzer* self, RFFEAnalyzerResults::RffeFrameType type);
void RFFEAnalyzer_FindParity(RFFEAnalyzer* self, bool expParity, U64 extra_data);
void RFFEAnalyzer_FindBusPark(RFFEAnalyzer* self);
void RFFEAnalyzer_GotoNextTransition(RFFEAnalyzer* self);
void RFFEAnalyzer_GotoSclkEdge(RFFEAnalyzer* self, BitState edge_type);
BitState RFFEAnalyzer_GetNextBit(RFFEAnalyzer* self, U8 idx);
U64 RFFEAnalyzer_GetBitStream(RFFEAnalyzer* self, U8 len);
bool RFFEAnalyzer_CheckClockRate(RFFEAnalyzer* self);
void RFFEAnalyzer_FillInFrame(RFFEAnalyzer* self, RFFEAnalyzerResults::RffeFrameType type, U64 frame_data, U64 extra_data, U32 idx_start, U32 idx_end, U8 flags);
bool RFFEAnalyzer_NeedsRerun(RFFEAnalyzer* self);
U32 RFFEAnalyzer_GenerateSimulationData(RFFEAnalyzer* self, U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels);
U32 RFFEAnalyzer_GetMinimumSampleRateHz(RFFEAnalyzer* self);
const char* RFFEAnalyzer_GetAnalyzerName(const RFFEAnalyzer* self);


// 初始化函数，替换构造函数
RFFEAnalyzer* RFFEAnalyzer_Create() {
  RFFEAnalyzer* self = (RFFEAnalyzer*)malloc(sizeof(RFFEAnalyzer));
  if (self == NULL) {
    return NULL;
  }
  Analyzer2_Init(&self->super);
  self->mSettings = RFFEAnalyzerSettings_Create();
  Analyzer2_SetAnalyzerSettings(&self->super, self->mSettings);
  self->mSimulationInitilized = false;
  self->mSclkCurrent = BIT_LOW;
  self->mSclkPrevious = BIT_LOW;
  self->mSdataCurrent = BIT_LOW;
  self->mSdataPrevious = BIT_LOW;
  self->mUnexpectedSSC = false;

  return self;
}


// 销毁函数，替换析构函数
void RFFEAnalyzer_Destroy(RFFEAnalyzer* self) {
  Analyzer2_KillThread(&self->super);
  RFFEAnalyzerSettings_Destroy(self->mSettings);
  RFFEAnalyzerResults_Destroy(self->mResults);
  free(self);
}

void RFFEAnalyzer_SetupResults(RFFEAnalyzer* self) {
  self->mResults = RFFEAnalyzerResults_Create(self, self->mSettings);
  Analyzer2_SetAnalyzerResults(&self->super, self->mResults);
  RFFEAnalyzerResults_AddChannelBubblesWillAppearOn(self->mResults, self->mSettings->mSdataChannel);
}

// ==============================================================================
// Main data parsing method
// ==============================================================================
void RFFEAnalyzer_WorkerThread(RFFEAnalyzer* self) {
  U8 byte_count;

  self->mSampleRateHz = Analyzer2_GetSampleRate(&self->super);

  self->mSdata = Analyzer2_GetAnalyzerChannelData(&self->super, self->mSettings->mSdataChannel);
  self->mSclk = Analyzer2_GetAnalyzerChannelData(&self->super, self->mSettings->mSclkChannel);

  RFFEAnalyzerResults_CancelPacketAndStartNewPacket(self->mResults);

  while (1) {
    // 处理异常，替换 try-catch
    if (self->mSclk == NULL || self->mSdata == NULL) {
      RFFEAnalyzerResults_CancelPacketAndStartNewPacket(self->mResults);
      break;
    }
    

    // Look for an SSC
    // This method only returns false if there is no more data to be scanned
    // in which case, we call the Cancel and wait for new data method in the
    // API
    if (!RFFEAnalyzer_FindStartSeqCondition(self)) {
      RFFEAnalyzerResults_CancelPacketAndStartNewPacket(self->mResults);
      break;
    }

    // Find and parse the Slave Address and the RFFE Command
    // Return ByteCount field - depending on the command it may or may not be
    // relevent
    byte_count = RFFEAnalyzer_FindCommandFrame(self);

    // We know what kind of packet we are handling now, and various parameters
    // including the number of data bytes.  Go ahead and handle the different
    // cases
    switch (self->mRffeCmdType) {
      case RFFEAnalyzerResults::RffeTypeExtWrite:
        RFFEAnalyzer_FindByteFrame(self, RFFEAnalyzerResults::RffeAddressField);
        for (U32 i = 0; i <= byte_count; i += 1) {
          RFFEAnalyzer_FindByteFrame(self, RFFEAnalyzerResults::RffeDataField);
        }
        break;
      case RFFEAnalyzerResults::RffeTypeReserved:
        break;
      case RFFEAnalyzerResults::RffeTypeMasterRead:
        RFFEAnalyzer_FindByteFrame(self, RFFEAnalyzerResults::RffeAddressField);
        RFFEAnalyzer_FindBusPark(self);
        for (U32 i = 0; i <= byte_count; i += 1) {
          RFFEAnalyzer_FindByteFrame(self, RFFEAnalyzerResults::RffeDataField);
        }
        break;
      case RFFEAnalyzerResults::RffeTypeMasterWrite:
        RFFEAnalyzer_FindByteFrame(self, RFFEAnalyzerResults::RffeAddressField);
        for (U32 i = 0; i <= byte_count; i += 1) {
          RFFEAnalyzer_FindByteFrame(self, RFFEAnalyzerResults::RffeDataField);
        }
        break;
      case RFFEAnalyzerResults::RffeTypeMasterHandoff:
        RFFEAnalyzer_FindBusPark(self);
        for (U32 i = 0; i <= byte_count; i += 1) {
          RFFEAnalyzer_FindByteFrame(self, RFFEAnalyzerResults::RffeDataField);
        }
        break;
      case RFFEAnalyzerResults::RffeTypeInterrupt:
        RFFEAnalyzer_FindBusPark(self);
        if (RFFEAnalyzer_FindISI(self)) {
          RFFEAnalyzer_FindInterruptSlots(self);
        }
        break;
      case RFFEAnalyzerResults::RffeTypeExtRead:
        RFFEAnalyzer_FindByteFrame(self, RFFEAnalyzerResults::RffeAddressField);
        RFFEAnalyzer_FindBusPark(self);
        for (U32 i = 0; i <= byte_count; i += 1) {
          RFFEAnalyzer_FindByteFrame(self, RFFEAnalyzerResults::RffeDataField);
        }
        break;
      case RFFEAnalyzerResults::RffeTypeExtLongWrite:
        RFFEAnalyzer_FindByteFrame(self, RFFEAnalyzerResults::RffeAddressHiField);
        RFFEAnalyzer_FindByteFrame(self, RFFEAnalyzerResults::RffeAddressLoField);
        for (U32 i = 0; i <= byte_count; i += 1) {
          RFFEAnalyzer_FindByteFrame(self, RFFEAnalyzerResults::RffeDataField);
        }
        break;
      case RFFEAnalyzerResults::RffeTypeExtLongRead:
        RFFEAnalyzer_FindByteFrame(self, RFFEAnalyzerResults::RffeAddressHiField);
        RFFEAnalyzer_FindByteFrame(self, RFFEAnalyzerResults::RffeAddressLoField);
        RFFEAnalyzer_FindBusPark(self);
        for (U32 i = 0; i <= byte_count; i += 1) {
          RFFEAnalyzer_FindByteFrame(self, RFFEAnalyzerResults::RffeDataField);
        }
        break;
      case RFFEAnalyzerResults::RffeTypeNormalWrite:
        RFFEAnalyzer_FindByteFrame(self, RFFEAnalyzerResults::RffeDataField);
        break;
      case RFFEAnalyzerResults::RffeTypeNormalRead:
        RFFEAnalyzer_FindBusPark(self);
        RFFEAnalyzer_FindByteFrame(self, RFFEAnalyzerResults::RffeDataField);
        break;
      case RFFEAnalyzerResults::RffeTypeWrite0:
        break;
    }

    // Finish up with a Bus Park (except for interrupts)
    if (self->mRffeCmdType != RFFEAnalyzerResults::RffeTypeInterrupt) {
      RFFEAnalyzer_FindBusPark(self);
    }

    // Commit the Result and call the API Required finished? method.
    RFFEAnalyzerResults_CommitPacketAndStartNewPacket(self->mResults);
    Analyzer2_CheckIfThreadShouldExit(&self->super);
  }
}

// ==============================================================================
// Worker thread support methods
// ==============================================================================
U8 RFFEAnalyzer_FindStartSeqCondition(RFFEAnalyzer* self) {
  bool ssc_possible = 0;
  U8 flags = 0;

  // Check for no more data, and return error status if none remains
  if (!AnalyzerChannelData_DoMoreTransitionsExistInCurrentData(self->mSclk))
    return 0;
  if (!AnalyzerChannelData_DoMoreTransitionsExistInCurrentData(self->mSdata))
    return 0;

  // Scan for an SSC
  if (self->mUnexpectedSSC) {
    flags |= (DISPLAY_AS_ERROR_FLAG | DISPLAY_AS_WARNING_FLAG | RFFE_INCOMPLETE_PACKET_ERROR_FLAG);
    self->mUnexpectedSSC = false;                      // Reset our unexpected SSC state
    self->mSampleClkOffsets[0] = self->mUnexpectedSSCStart;  // The unexpected SSC logic
                                                 // saved the sdata rising edge
                                                 // position
    self->mSampleDataOffsets[0] = self->mUnexpectedSSCStart; // For the Marker
  } else {
    while (1) {
      RFFEAnalyzer_GotoNextTransition(self);

      if (self->mSclkCurrent == BIT_HIGH) {
        ssc_possible = 0;
      } else if (self->mSclkCurrent == BIT_LOW && self->mSclkPrevious == BIT_LOW && self->mSdataCurrent == BIT_HIGH && self->mSdataPrevious == BIT_LOW) {
        ssc_possible = 1;
        self->mSampleClkOffsets[0] = self->mSamplePosition;  // For the Frame Boundaries
        self->mSampleDataOffsets[0] = self->mSamplePosition; // For the Marker
      } else if (ssc_possible && self->mSclkCurrent == BIT_LOW && self->mSdataCurrent == BIT_LOW) {
        break; // SSC!
      }
    }
    // TODO: Here is where we could check RFFE SSC Pulse width, if we were so
    // inclined
    // ssc_pulse_width_in_samples = sdata_fedge_sample - sdata_redge_sample;
  }

  // SSC is complete at the rising edge of the SCLK
  RFFEAnalyzer_GotoSclkEdge(self, BIT_HIGH);
  self->mSampleClkOffsets[1] = self->mSamplePosition;

  // TODO: Here is where we could check SSC to SCLK rising edge width, again, if
  // we were so inclined
  // ssc_2_sclk_delay_in_samples = mSamplePosition - sdata_fedge_sample;

  // Setup the 'Start' Marker and send off the Frame to the AnalyzerResults
  self->mSampleMarker[0] = AnalyzerResults::Start;
  RFFEAnalyzer_FillInFrame(self, RFFEAnalyzerResults::RffeSSCField, 0, 0, 0, 1, flags);

  return 1;
}

// ------------------------------------------------------------------------------
U8 RFFEAnalyzer_FindCommandFrame(RFFEAnalyzer* self) {
  U8 byte_count = 0;
  U8 flags = 0;
  U64 RffeID;
  U64 RffeCmd;

  // Get RFFE SA+Command (4 + 8 bits) and decode the fields
  // Why grab all 12 bits?  So we can calculate parity across the SA+Cmd field
  // after we are done parsing the Command
  RffeCmd = RFFEAnalyzer_GetBitStream(self, 12);

  // Now look at the RFFE command and decode the various options.
  self->mRffeCmdType = RFFEUtil_decodeRFFECmdFrame((U8)(RffeCmd & 0xFF));
  byte_count = RFFEUtil_byteCount((U8)RffeCmd);
  RffeID = (RffeCmd & 0xF00) >> 8;

  // Check for an invalid Master address, if this is a Master Command
  if ((self->mRffeCmdType == RFFEAnalyzerResults::RffeTypeMasterRead || self->mRffeCmdType == RFFEAnalyzerResults::RffeTypeMasterWrite ||
       self->mRffeCmdType == RFFEAnalyzerResults::RffeTypeMasterHandoff) &&
      (RffeID > 0x3)) {
    flags = RFFE_INVALID_MASTER_ID;
  }
  // Log the Master or Slave Address
  RFFEAnalyzer_FillInFrame(self, RFFEAnalyzerResults::RffeSAField, RffeID, 0, 0, 4, flags);
  flags = 0;

  switch (self->mRffeCmdType) {
    case RFFEAnalyzerResults::RffeTypeReserved:
      // 8 Bit Reserved Cmd Type
      flags |= (DISPLAY_AS_ERROR_FLAG | DISPLAY_AS_WARNING_FLAG | RFFE_INVALID_CMD_ERROR_FLAG);
      RFFEAnalyzer_FillInFrame(self, RFFEAnalyzerResults::RffeCommandField, self->mRffeCmdType, 0, 4, 12, flags);
      break;
    case RFFEAnalyzerResults::RffeTypeMasterRead:
    case RFFEAnalyzerResults::RffeTypeMasterWrite:
    case RFFEAnalyzerResults::RffeTypeMasterHandoff:
    case RFFEAnalyzerResults::RffeTypeInterrupt:
      // Interrupt/Master commands - 8 Bit RFFE Command
      RFFEAnalyzer_FillInFrame(self, RFFEAnalyzerResults::RffeCommandField, self->mRffeCmdType, 0, 4, 12, flags);
      break;
    case RFFEAnalyzerResults::RffeTypeExtWrite:
    case RFFEAnalyzerResults::RffeTypeExtRead:
      // 4 Bit Command w/ 4 bit Byte Count
      RFFEAnalyzer_FillInFrame(self, RFFEAnalyzerResults::RffeCommandField, self->mRffeCmdType, 0, 4, 8, flags);
      RFFEAnalyzer_FillInFrame(self, RFFEAnalyzerResults::RffeExByteCountField, (RffeCmd & 0x0F), 0, 8, 12, flags);
      break;
    case RFFEAnalyzerResults::RffeTypeExtLongWrite:
    case RFFEAnalyzerResults::RffeTypeExtLongRead:
      // 5 Bit Command w/ 3 bit Byte Count
      RFFEAnalyzer_FillInFrame(self, RFFEAnalyzerResults::RffeCommandField, self->mRffeCmdType, 0, 4, 9, flags);
      RFFEAnalyzer_FillInFrame(self, RFFEAnalyzerResults::RffeExLongByteCountField, (RffeCmd & 0x07), 0, 9, 12, flags);
      break;
    case RFFEAnalyzerResults::RffeTypeNormalWrite:
    case RFFEAnalyzerResults::RffeTypeNormalRead:
      // 3 Bit Command w/ 5 bit Addr
      RFFEAnalyzer_FillInFrame(self, RFFEAnalyzerResults::RffeCommandField, self->mRffeCmdType, 0, 4, 7, flags);
      RFFEAnalyzer_FillInFrame(self, RFFEAnalyzerResults::RffeAddressField, (RffeCmd & 0x1F), 0, 7, 12, flags);
      break;
    case RFFEAnalyzerResults::RffeTypeWrite0:
      // 1 Bit Command w/ 7 bit Write Data
      RFFEAnalyzer_FillInFrame(self, RFFEAnalyzerResults::RffeCommandField, self->mRffeCmdType, 0, 4, 5, flags);
      RFFEAnalyzer_FillInFrame(self, RFFEAnalyzerResults::RffeShortDataField, (RffeCmd & 0x7F), 0, 5, 12, flags);
      break;
  }

  // Check Parity - Parity bit covers the full SA/Command field (12 bits)
  RFFEAnalyzer_FindParity(self, RFFEUtil_CalcParity(RffeCmd), 0);

  return byte_count;
}

// ------------------------------------------------------------------------------
U8 RFFEAnalyzer_FindISI(RFFEAnalyzer* self) {
  U64 byte = RFFEAnalyzer_GetBitStream(self, 2);
  RFFEAnalyzer_FillInFrame(self, RFFEAnalyzerResults::RffeISIField, byte, 0, 0, 2, 0);
  RFFEAnalyzer_FindBusPark(self);
  return (byte & 0x2); // Return the ISI bit
}

// ------------------------------------------------------------------------------
void RFFEAnalyzer_FindInterruptSlots(RFFEAnalyzer* self) {
  for (S8 i = 15; i >= 0; i -= 1) {
    U64 byte = RFFEAnalyzer_GetBitStream(self, 1);
    RFFEAnalyzer_FillInFrame(self, RFFEAnalyzerResults::RffeIntSlotField, byte, i, 0, 1, 0); // Send int number as a note
    RFFEAnalyzer_FindBusPark(self);
  }
}

// ------------------------------------------------------------------------------
void RFFEAnalyzer_FindByteFrame(RFFEAnalyzer* self, RFFEAnalyzerResults::RffeFrameType type) {
  U64 byte = RFFEAnalyzer_GetBitStream(self, 8);
  RFFEAnalyzer_FillInFrame(self, type, byte, 0, 0, 8, 0);
  RFFEAnalyzer_FindParity(self, RFFEUtil_CalcParity(byte), 1);
}

// ------------------------------------------------------------------------------
void RFFEAnalyzer_FindParity(RFFEAnalyzer* self, bool expParity, U64 extra_data) {
  BitState bitstate;
  bool data_parity;
  U64 parity_value;
  U8 flags = 0;

  // Get the Parity Bit on the next sample clock
  bitstate = RFFEAnalyzer_GetNextBit(self, 0);

  // Store away tailing sample position as the end of the Parity bit
  self->mSampleClkOffsets[1] = self->mSamplePosition;

  if (bitstate == BIT_HIGH) {
    parity_value = 1;
    data_parity = true;
    self->mSampleMarker[0] = AnalyzerResults::One;
  } else {
    parity_value = 0;
    data_parity = false;
    self->mSampleMarker[0] = AnalyzerResults::Zero;
  }

  if (data_parity != expParity) {
    flags |= (DISPLAY_AS_ERROR_FLAG | DISPLAY_AS_WARNING_FLAG | RFFE_PARITY_ERROR_FLAG);
    self->mSampleMarker[0] = AnalyzerResults::ErrorDot;
  }

  RFFEAnalyzer_FillInFrame(self, RFFEAnalyzerResults::RffeParityField, parity_value, extra_data, 0, 1, flags);
}

// ------------------------------------------------------------------------------
void RFFEAnalyzer_FindBusPark(RFFEAnalyzer* self) {
  U64 half_clk;

  // Mark this as a stop
  self->mSampleMarker[0] = AnalyzerResults::Stop;

  // Enter at the rising edge of SCLK
  self->mSampleClkOffsets[0] = self->mSamplePosition;

  // Goto Falling
  RFFEAnalyzer_GotoSclkEdge(self, BIT_LOW);

  // Now at falling edge of clk
  self->mSampleDataOffsets[0] = self->mSamplePosition;

  // Now the bus park is offically complete, but for display
  // purposes it would look better if we had the Bus Park
  // 'Bubble' extend for what would be the full clock period.
  half_clk = self->mSampleDataOffsets[0] - self->mSampleClkOffsets[0];

  // If there is another clock edge less thank a 1/2 clock period out
  // (plus a little) then extend the Bus Park to that edge.
  // Otherwise edge the Bus Park for the 1/2 clock period from the
  // previous SClk Rising -> Falling
  if (AnalyzerChannelData_WouldAdvancingCauseTransition(self->mSclk, (U32)(half_clk + 2))) {
    RFFEAnalyzer_GotoSclkEdge(self, BIT_HIGH);
    self->mSampleClkOffsets[1] = AnalyzerChannelData_GetSampleNumber(self->mSclk);
  } else {
    self->mSampleClkOffsets[1] = self->mSampleDataOffsets[0] + half_clk;
  }

  RFFEAnalyzer_FillInFrame(self, RFFEAnalyzerResults::RffeBusParkField, 0, 0, 0, 1, 0);
}

// ==============================================================================
// Advance to the next transition on the bus (SCLK or SDATA transition)
// ==============================================================================
void RFFEAnalyzer_GotoNextTransition(RFFEAnalyzer* self) {
  U64 SclkEdgeSample;
  U64 SdataEdgeSample;

  // Store the previous Transition bus state
  self->mSclkPrevious = self->mSclkCurrent;
  self->mSdataPrevious = self->mSdataCurrent;

  // Look for a transition on SDATA without a clock transition
  SclkEdgeSample = AnalyzerChannelData_GetSampleOfNextEdge(self->mSclk);
  SdataEdgeSample = AnalyzerChannelData_GetSampleOfNextEdge(self->mSdata);

  if (SclkEdgeSample > SdataEdgeSample) {
    // Sclk is further out the sData
    self->mSamplePosition = SdataEdgeSample;
    AnalyzerChannelData_AdvanceToAbsPosition(self->mSclk, SdataEdgeSample);
    AnalyzerChannelData_AdvanceToAbsPosition(self->mSdata, SdataEdgeSample);
  } else {
    // Sdata transition is further out than Sclk
    self->mSamplePosition = SclkEdgeSample;
    AnalyzerChannelData_AdvanceToAbsPosition(self->mSclk, SclkEdgeSample);
    AnalyzerChannelData_AdvanceToAbsPosition(self->mSdata, SclkEdgeSample);
  }

  // Update the current transition bus state
  self->mSclkCurrent = AnalyzerChannelData_GetBitState(self->mSclk);
  self->mSdataCurrent = AnalyzerChannelData_GetBitState(self->mSdata);
}

// ==============================================================================
void RFFEAnalyzer_GotoSclkEdge(RFFEAnalyzer* self, BitState edge_type) {
  bool ssc_possible = 0;

  // Scan for the next falling edge on SCLK while watching for SSCs
  while (1) {
    RFFEAnalyzer_GotoNextTransition(self);
    if (self->mSclkCurrent == edge_type && self->mSclkPrevious != edge_type) {
      break;
    }

    // Unexpected SSC monitoring
    if (self->mSclkCurrent == BIT_HIGH) {
      ssc_possible = 0;
    } else if (self->mSclkCurrent == BIT_LOW && self->mSclkPrevious == BIT_LOW && self->mSdataCurrent == BIT_HIGH && self->mSdataPrevious == BIT_LOW) {
      self->mUnexpectedSSCStart = self->mSamplePosition;
      ssc_possible = 1;
    } else if (ssc_possible && self->mSclkCurrent == BIT_LOW && self->mSdataCurrent == BIT_LOW) {
      self->mUnexpectedSSC = true; // !!!
      // 抛出异常
      return;
    }
  }
}

// ==============================================================================
BitState RFFEAnalyzer_GetNextBit(RFFEAnalyzer* self, U8 idx) {
  BitState data;

  // Previous FindSSC or GetNextBit left us at the rising edge of the SCLK
  // Grab this as the current sample point for delimiting the frame.
  self->mSampleClkOffsets[idx] = self->mSamplePosition;

  // Goto the next SCLK falling edge, sample SDATA and
  // put a marker to indicate that this is the sampled edge
  RFFEAnalyzer_GotoSclkEdge(self, BIT_LOW);
  data = self->mSdataCurrent;

  self->mSampleDataOffsets[idx] = self->mSamplePosition;
  RFFEAnalyzerResults_AddMarker(self->mResults, self->mSamplePosition, AnalyzerResults::DownArrow, self->mSettings->mSclkChannel);

  RFFEAnalyzer_GotoSclkEdge(self, BIT_HIGH);

  // Return the sData Value sampled on the falling edge
  return data;
}

// --------------------------------------
U64 RFFEAnalyzer_GetBitStream(RFFEAnalyzer* self, U8 len) {
  U64 data;
  U32 i;
  BitState state;
  DataBuilder data_builder;

  DataBuilder_Reset(&data_builder, &data, AnalyzerEnums::MsbFirst, len);

  // starting at rising edge of clk
  for (i = 0; i < len; i++) {
    state = RFFEAnalyzer_GetNextBit(self, i);
    DataBuilder_AddBit(&data_builder, state);

    if (state == BIT_HIGH) {
      self->mSampleMarker[i] = AnalyzerResults::One;
    } else {
      self->mSampleMarker[i] = AnalyzerResults::Zero;
    }
  }
  self->mSampleClkOffsets[i] = self->mSamplePosition;

  return data;
}

// ==============================================================================
// Physical Layer checks, if we so choose to implement them.
// ==============================================================================
bool RFFEAnalyzer_CheckClockRate(RFFEAnalyzer* self) {
  // TODO:  Compare the clock pulse width based on sample
  // rate against the spec.
  return true;
}

// ==============================================================================
// Results and Screen Markers
// ==============================================================================
void RFFEAnalyzer_FillInFrame(RFFEAnalyzer* self, RFFEAnalyzerResults::RffeFrameType type, U64 frame_data, U64 extra_data, U32 idx_start, U32 idx_end, U8 flags) {
  Frame frame;

  frame.mType = (U8)type;
  frame.mData1 = frame_data;
  frame.mData2 = extra_data; // Additional Data (could be used for AnalyzerResults if required)
  frame.mStartingSampleInclusive = self->mSampleClkOffsets[idx_start];
  frame.mEndingSampleInclusive = self->mSampleClkOffsets[idx_end];
  frame.mFlags = flags;

  // Add Markers to the SDATA stream while we are creating the Frame
  // That is if the Frame is non-zero length and also merits a marker.
  for (U32 i = idx_start; i < idx_end; i += 1) {
    RFFEAnalyzerResults_AddMarker(self->mResults, self->mSampleDataOffsets[i], self->mSampleMarker[i], self->mSettings->mSdataChannel);
  }

  RFFEAnalyzerResults_AddFrame(self->mResults, frame);

  // New FrameV2 code.
  FrameV2 frame_v2;
  // you can add any number of key value pairs. Each will get it's own column in the data table.
  FrameV2_AddString(&frame_v2, "type", RffeFrameString[type]);
  FrameV2_AddInteger(&frame_v2, "data", frame_data);
  FrameV2_AddInteger(&frame_v2, "extra_data", extra_data);
  FrameV2_AddInteger(&frame_v2, "flags", flags);
  RFFEAnalyzerResults_AddFrameV2(self->mResults, frame_v2, "frame", frame.mStartingSampleInclusive, frame.mEndingSampleInclusive);

  RFFEAnalyzerResults_CommitResults(self->mResults);
  Analyzer2_ReportProgress(&self->super, frame.mEndingSampleInclusive);
}

// ==============================================================================
// Boilerplate for the API
// ==============================================================================
bool RFFEAnalyzer_NeedsRerun(RFFEAnalyzer* self) {
  return false;
}

// --------------------------------------
U32 RFFEAnalyzer_GenerateSimulationData(RFFEAnalyzer* self, U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels) {
  if (self->mSimulationInitilized == false) {
    RFFEAnalyzerSimulationDataGenerator_Initialize(&self->mSimulationDataGenerator, Analyzer2_GetSimulationSampleRate(&self->super), self->mSettings);
    self->mSimulationInitilized = true;
  }

  return RFFEAnalyzerSimulationDataGenerator_GenerateSimulationData(&self->mSimulationDataGenerator, minimum_sample_index, device_sample_rate, simulation_channels);
}

// --------------------------------------
U32 RFFEAnalyzer_GetMinimumSampleRateHz(RFFEAnalyzer* self) {
  return 50000000;
}

// --------------------------------------
const char* RFFEAnalyzer_GetAnalyzerName(const RFFEAnalyzer* self) {
  return "RFFEv2.0";
}


// 函数指针数组，用于封装函数
static const struct RFFEAnalyzer_Interface {
  RFFEAnalyzer* (*Create)();
  void (*Destroy)(RFFEAnalyzer*);
  void (*SetupResults)(RFFEAnalyzer*);
  void (*WorkerThread)(RFFEAnalyzer*);
  bool (*NeedsRerun)(RFFEAnalyzer*);
  U32 (*GenerateSimulationData)(RFFEAnalyzer*, U64, U32, SimulationChannelDescriptor**);
  U32 (*GetMinimumSampleRateHz)(RFFEAnalyzer*);
  const char* (*GetAnalyzerName)(const RFFEAnalyzer*);
} RFFEAnalyzer_Interface = {
  RFFEAnalyzer_Create,
  RFFEAnalyzer_Destroy,
  RFFEAnalyzer_SetupResults,
  RFFEAnalyzer_WorkerThread,
  RFFEAnalyzer_NeedsRerun,
  RFFEAnalyzer_GenerateSimulationData,
  RFFEAnalyzer_GetMinimumSampleRateHz,
  RFFEAnalyzer_GetAnalyzerName,
};


// 提供外部接口，方便其他模块调用
const struct RFFEAnalyzer_Interface* GetRFFEAnalyzerInterface() {
  return &RFFEAnalyzer_Interface;
}

// --------------------------------------
const char* GetAnalyzerName() {
  return "RFFEv2.0";
}

// --------------------------------------
Analyzer* CreateAnalyzer() {
  const struct RFFEAnalyzer_Interface* interface = GetRFFEAnalyzerInterface();
  return interface->Create();
}

// --------------------------------------
void DestroyAnalyzer(Analyzer* analyzer) {
  const struct RFFEAnalyzer_Interface* interface = GetRFFEAnalyzerInterface();
  interface->Destroy((RFFEAnalyzer*)analyzer);
}
