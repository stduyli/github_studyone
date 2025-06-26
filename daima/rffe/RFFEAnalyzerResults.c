#include "RFFEAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "RFFEAnalyzer.h"
#include "RFFEAnalyzerSettings.h"
#include <stdio.h>

// 结构体定义，替换类
typedef struct RFFEAnalyzerResults_t {
  AnalyzerResults super;
  RFFEAnalyzerSettings* mSettings;
  RFFEAnalyzer* mAnalyzer;
} RFFEAnalyzerResults;

// 函数声明，替换类成员函数
void RFFEAnalyzerResults_GenerateBubbleText(RFFEAnalyzerResults* self, U64 frame_index, Channel* channel, DisplayBase display_base);
void RFFEAnalyzerResults_GenerateExportFile(RFFEAnalyzerResults* self, const char* file, DisplayBase display_base, U32 export_type_user_id);
void RFFEAnalyzerResults_GenerateFrameTabularText(RFFEAnalyzerResults* self, U64 frame_index, DisplayBase display_base);
void RFFEAnalyzerResults_GeneratePacketTabularText(RFFEAnalyzerResults* self, U64 packet_id, DisplayBase display_base);
void RFFEAnalyzerResults_GenerateTransactionTabularText(RFFEAnalyzerResults* self, U64 transaction_id, DisplayBase display_base);

// 初始化函数，替换构造函数
RFFEAnalyzerResults* RFFEAnalyzerResults_Create(RFFEAnalyzer* analyzer, RFFEAnalyzerSettings* settings) {
  RFFEAnalyzerResults* self = (RFFEAnalyzerResults*)malloc(sizeof(RFFEAnalyzerResults));
  if (self == NULL) {
    return NULL;
  }
  AnalyzerResults_Init(&self->super);
  self->mSettings = settings;
  self->mAnalyzer = analyzer;
  return self;
}

// 销毁函数，替换析构函数
void RFFEAnalyzerResults_Destroy(RFFEAnalyzerResults* self) {
  free(self);
}


//
void RFFEAnalyzerResults_GenerateBubbleText(RFFEAnalyzerResults* self, U64 frame_index, Channel* channel, DisplayBase display_base) {
  char number_str[8];
  char results_str[16];

  *channel = *channel;  // 保留原代码逻辑，即使没有实际用途

  AnalyzerResults_ClearResultStrings(&self->super);
  Frame frame = AnalyzerResults_GetFrame(&self->super, frame_index);

  switch (frame.mType) {
    case RffeSSCField: {
      AnalyzerResults_AddResultString(&self->super, "SSC");
    } break;

    case RffeSAField: {
      AnalyzerHelpers_GetNumberString(frame.mData1, display_base, 4, number_str, 8);
      snprintf(results_str, sizeof(results_str), "SA:%s", number_str);
      AnalyzerResults_AddResultString(&self->super, "SA");
      AnalyzerResults_AddResultString(&self->super, results_str);
    } break;

    case RffeCommandField: {
      AnalyzerResults_AddResultString(&self->super, RffeCommandFieldStringShort[frame.mData1]);
      AnalyzerResults_AddResultString(&self->super, RffeCommandFieldStringMid[frame.mData1]);
    } break;

    case RffeExByteCountField: {
      AnalyzerHelpers_GetNumberString(frame.mData1, display_base, 4, number_str, 8);
      snprintf(results_str, sizeof(results_str), "BC:%s", number_str);
      AnalyzerResults_AddResultString(&self->super, "BC");
      AnalyzerResults_AddResultString(&self->super, results_str);
    } break;

    case RffeExLongByteCountField: {
      AnalyzerHelpers_GetNumberString(frame.mData1, display_base, 3, number_str, 8);
      snprintf(results_str, sizeof(results_str), "BC:%s", number_str);
      AnalyzerResults_AddResultString(&self->super, "BC");
      AnalyzerResults_AddResultString(&self->super, results_str);
    } break;

    case RffeAddressField: {
      AnalyzerHelpers_GetNumberString(frame.mData1, display_base, 8, number_str, 8);
      snprintf(results_str, sizeof(results_str), "A:%s", number_str);
      AnalyzerResults_AddResultString(&self->super, "A");
      AnalyzerResults_AddResultString(&self->super, results_str);
    } break;

    case RffeAddressHiField: {
      AnalyzerHelpers_GetNumberString(frame.mData1, display_base, 8, number_str, 8);
      snprintf(results_str, sizeof(results_str), "AH:%s", number_str);
      AnalyzerResults_AddResultString(&self->super, "A");
      AnalyzerResults_AddResultString(&self->super, results_str);

    } break;

    case RffeAddressLoField: {
      AnalyzerHelpers_GetNumberString(frame.mData1, display_base, 8, number_str, 8);
      snprintf(results_str, sizeof(results_str), "AL:%s", number_str);
      AnalyzerResults_AddResultString(&self->super, "A");
      AnalyzerResults_AddResultString(&self->super, results_str);
    } break;

    case RffeShortDataField: {
      AnalyzerHelpers_GetNumberString(frame.mData1, display_base, 7, number_str, 8);
      snprintf(results_str, sizeof(results_str), "D:%s", number_str);
      AnalyzerResults_AddResultString(&self->super, "D");
      AnalyzerResults_AddResultString(&self->super, results_str);
    } break;

    case RffeDataField: {
      AnalyzerHelpers_GetNumberString(frame.mData1, display_base, 8, number_str, 8);
      snprintf(results_str, sizeof(results_str), "D:%s", number_str);
      AnalyzerResults_AddResultString(&self->super, "D");
      AnalyzerResults_AddResultString(&self->super, results_str);
    } break;

    case RffeMasterHandoffAckField: {
      AnalyzerHelpers_GetNumberString(frame.mData1, display_base, 8, number_str, 8);
      snprintf(results_str, sizeof(results_str), "MHAck:%s", number_str);
      AnalyzerResults_AddResultString(&self->super, "MHA");
      AnalyzerResults_AddResultString(&self->super, results_str);
    } break;

    case RffeISIField: {
      AnalyzerHelpers_GetNumberString(frame.mData1, display_base, 2, number_str, 4);
      AnalyzerResults_AddResultString(&self->super, "ISI");
      AnalyzerResults_AddResultString(&self->super, "ISI");
    } break;

    case RffeIntSlotField: {
      AnalyzerHelpers_GetNumberString(frame.mData1, display_base, 1, number_str, 4);
      snprintf(results_str, sizeof(results_str), "INT%d", (unsigned char)frame.mData2);
      AnalyzerResults_AddResultString(&self->super, "I");
      AnalyzerResults_AddResultString(&self->super, results_str);
    } break;

    case RffeParityField: {
      AnalyzerHelpers_GetNumberString(frame.mData1, Decimal, 1, number_str, 4);
      snprintf(results_str, sizeof(results_str), "P:%s", number_str);
      AnalyzerResults_AddResultString(&self->super, "P");
      AnalyzerResults_AddResultString(&self->super, results_str);
    } break;

    case RffeBusParkField: {
      AnalyzerResults_AddResultString(&self->super, "B");
      AnalyzerResults_AddResultString(&self->super, "BP");
    } break;

    case RffeErrorCaseField:
    default: {
      char number1_str[20];
      char number2_str[20];

      AnalyzerHelpers_GetNumberString(frame.mData1, Hexadecimal, 32, number1_str, 10);
      AnalyzerHelpers_GetNumberString(frame.mData2, Hexadecimal, 32, number2_str, 10);
      snprintf(results_str, sizeof(results_str), "E:%s - %s", number1_str, number2_str);
      AnalyzerResults_AddResultString(&self->super, "E");
      AnalyzerResults_AddResultString(&self->super, results_str);
    } break;
  }
}

void RFFEAnalyzerResults_GenerateExportFile(RFFEAnalyzerResults* self, const char* file, DisplayBase display_base, U32 export_type_user_id) {
  U64 first_frame_id;
  U64 last_frame_id;
  U64 address;
  char time_str[16];
  char packet_str[16];
  char sa_str[8];
  char type_str[16];
  char addr_str[16];
  char parity_str[8];
  char parityCmd_str[8];
  char bc_str[8];
  char data_str[8];
  bool show_parity = self->mSettings->mShowParityInReport;
  bool show_buspark = self->mSettings->mShowBusParkInReport;
  char payload_str[256];
  char results_str[256];
  Frame frame;
  void* f = AnalyzerHelpers_StartFile(file);

  export_type_user_id = export_type_user_id;  // 保留原代码逻辑，即使没有实际用途

  U64 trigger_sample = self->mAnalyzer->GetTriggerSample();
  U32 sample_rate = self->mAnalyzer->GetSampleRate();

  if (show_parity) {
    snprintf(results_str, sizeof(results_str), "Time[s],Packet ID,SA,Type,Adr,BC,CmdP,Payload\n");
  } else {
    snprintf(results_str, sizeof(results_str), "Time[s],Packet ID,SA,Type,Adr,BC,Payload\n");
  }
  AnalyzerHelpers_AppendToFile((U8*)results_str, (U32)strlen(results_str), f);

  U64 num_packets = AnalyzerResults_GetNumPackets(&self->super);
  for (U32 i = 0; i < num_packets; i++) {
    // package id
    AnalyzerHelpers_GetNumberString(i, Decimal, 0, packet_str, 16);

    snprintf(sa_str, 8, "");
    snprintf(type_str, 8, "");
    snprintf(addr_str, 8, "");
    snprintf(parity_str, 8, "");
    snprintf(parityCmd_str, 8, "");
    snprintf(bc_str, 8, "");
    snprintf(data_str, 8, "");
    snprintf(payload_str, 256, "");
    snprintf(results_str, 256, "");
    address = 0x0;

    AnalyzerResults_GetFramesContainedInPacket(&self->super, i, &first_frame_id, &last_frame_id);
    for (U64 j = first_frame_id; j <= last_frame_id; j++) {
      frame = AnalyzerResults_GetFrame(&self->super, j);

      switch (frame.mType) {
        case RffeSSCField:
          // starting time using SSC as marker
          AnalyzerHelpers_GetTimeString(frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 16);
          break;

        case RffeSAField:
          AnalyzerHelpers_GetNumberString(frame.mData1, display_base, 4, sa_str, 8);
          break;

        case RffeCommandField:
          snprintf(type_str, sizeof(type_str), "%s", RffeCommandFieldStringMid[frame.mData1]);
          if (frame.mData1 == RffeTypeNormalWrite || frame.mData1 == RffeTypeNormalRead) {
            snprintf(bc_str, sizeof(bc_str), "0x0");
          } else if (frame.mData1 == RffeTypeWrite0) {
            snprintf(bc_str, sizeof(bc_str), "0x0");
            snprintf(addr_str, sizeof(addr_str), "0x0");
          }
          break;

        case RffeExByteCountField:
          AnalyzerHelpers_GetNumberString(frame.mData1, display_base, 4, bc_str, 8);
          break;

        case RffeExLongByteCountField:
          AnalyzerHelpers_GetNumberString(frame.mData1, display_base, 3, bc_str, 8);
          break;

        case RffeAddressField:
          address = frame.mData1;
          AnalyzerHelpers_GetNumberString(address, display_base, sizeof(addr_str), addr_str, 16);
          break;

        case RffeAddressHiField:
          address = (frame.mData1 << 8);
          AnalyzerHelpers_GetNumberString(address, display_base, sizeof(addr_str), addr_str, 16);
          break;

        case RffeAddressLoField:
          address |= frame.mData1;
          AnalyzerHelpers_GetNumberString(address, display_base, sizeof(addr_str), addr_str, 16);
          break;

        case RffeShortDataField:
          AnalyzerHelpers_GetNumberString(frame.mData1, display_base, 7, data_str, 8);
          snprintf(payload_str, sizeof(payload_str), "%s", data_str);
          break;

        case RffeDataField:
          AnalyzerHelpers_GetNumberString(frame.mData1, display_base, 8, data_str, 8);
          // Gather up all the data bytes into the payload
          if (strlen(payload_str) == 0) {
            snprintf(payload_str, sizeof(payload_str), "%s", data_str);
          } else {
            snprintf(payload_str, sizeof(payload_str), "%s %s", payload_str, data_str);
          }
          break;

        case RffeParityField:
          if (show_parity) {
            AnalyzerHelpers_GetNumberString(frame.mData1, Decimal, 1, parity_str, 4);
            if (frame.mData2) {
              // Data Parity
              if (strlen(payload_str) == 0) {
                snprintf(payload_str, sizeof(payload_str), "P%s", parity_str);
              } else {
                snprintf(payload_str, sizeof(payload_str), "%s P%s", payload_str, parity_str);
              }
            } else {
              snprintf(parityCmd_str, sizeof(parityCmd_str), "P%s", parity_str); // CmdParity
            }
          }
          break;

        case RffeBusParkField:
          if (show_buspark) {
            if (strlen(payload_str) == 0) {
              snprintf(payload_str, sizeof(payload_str), "BP");
            } else {
              snprintf(payload_str, sizeof(payload_str), "%s BP", payload_str);
            }
          }
          break;

        case RffeErrorCaseField:
        default:
          char number1_str[20];
          char number2_str[20];

          AnalyzerHelpers_GetNumberString(frame.mData1, Hexadecimal, 32, number1_str, 10);
          AnalyzerHelpers_GetNumberString(frame.mData2, Hexadecimal, 32, number2_str, 10);
          snprintf(payload_str, sizeof(payload_str), "E:%s - %s ", number1_str, number2_str);
          break;
      }
    }

    if (show_parity) {
      snprintf(results_str, sizeof(results_str), "%s,%s,%s,%s,%s,%s,%s,%s\n", time_str, packet_str, sa_str, type_str, addr_str, bc_str, parityCmd_str,
               payload_str);
    } else {
      snprintf(results_str, sizeof(results_str), "%s,%s,%s,%s,%s,%s,%s\n", time_str, packet_str, sa_str, type_str, addr_str, bc_str, payload_str);
    }
    AnalyzerHelpers_AppendToFile((U8*)results_str, (U32)strlen(results_str), f);

    if (AnalyzerResults_UpdateExportProgressAndCheckForCancel(&self->super, i, num_packets) == true) {
      AnalyzerHelpers_EndFile(f);
      return;
    }
  }
}

void RFFEAnalyzerResults_GenerateFrameTabularText(RFFEAnalyzerResults* self, U64 frame_index, DisplayBase display_base) {
  char time_str[16];
  char sa_str[8];
  char type_str[16];
  char parity_str[8];
  char parityCmd_str[8];
  char addr_str[8];
  char bc_str[8];
  char data_str[8];
  char frame_str[32];
  Frame frame;

  U64 trigger_sample = self->mAnalyzer->GetTriggerSample();
  U32 sample_rate = self->mAnalyzer->GetSampleRate();

  AnalyzerResults_ClearTabularText(&self->super);

  frame = AnalyzerResults_GetFrame(&self->super, frame_index);

  switch (frame.mType) {
    case RffeSSCField:
      // starting time using SSC as marker
      AnalyzerHelpers_GetTimeString(frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 16);
      snprintf(frame_str, sizeof(frame_str), "SSC");
      break;

    case RffeSAField:
      AnalyzerHelpers_GetNumberString(frame.mData1, display_base, 4, sa_str, 8);
      snprintf(frame_str, sizeof(frame_str), "SA: %s", sa_str);
      break;

    case RffeCommandField:
      snprintf(type_str, sizeof(type_str), "%s", RffeCommandFieldStringMid[frame.mData1]);
      snprintf(frame_str, sizeof(frame_str), "Type: %s", type_str);
      break;

    case RffeExByteCountField:
      AnalyzerHelpers_GetNumberString(frame.mData1, display_base, 4, bc_str, 8);
      snprintf(frame_str, sizeof(frame_str), "ExByteCount: %s", bc_str);
      break;

    case RffeExLongByteCountField:
      AnalyzerHelpers_GetNumberString(frame.mData1, display_base, 3, bc_str, 8);
      snprintf(frame_str, sizeof(frame_str), "ExLongByteCount: %s", bc_str);
      break;

    case RffeAddressField:
      AnalyzerHelpers_GetNumberString(frame.mData1, display_base, 8, addr_str, 8);
      snprintf(frame_str, sizeof(frame_str), "Addr: %s", addr_str);
      break;

    case RffeAddressHiField:
      AnalyzerHelpers_GetNumberString(frame.mData1, display_base, 8, addr_str, 8);
      snprintf(frame_str, sizeof(frame_str), "AddrHi: %s", addr_str);
      break;

    case RffeAddressLoField:
      AnalyzerHelpers_GetNumberString(frame.mData1, display_base, 8, addr_str, 8);
      snprintf(frame_str, sizeof(frame_str), "AddrLo: %s", addr_str);
      break;

    case RffeShortDataField:
      AnalyzerHelpers_GetNumberString(frame.mData1, display_base, 7, data_str, 8);
      snprintf(frame_str, sizeof(frame_str), "DataShort: %s", data_str);
      break;

    case RffeDataField:
      AnalyzerHelpers_GetNumberString(frame.mData1, display_base, 8, data_str, 8);
      snprintf(frame_str, sizeof(frame_str), "Data: %s", data_str);
      break;

    case RffeParityField:
      if (frame.mData2 == 1) {
        AnalyzerHelpers_GetNumberString(frame.mData1, Decimal, 1, parity_str, 4);
        snprintf(frame_str, sizeof(frame_str), "DataParity: %s", parity_str);
      } else {
        AnalyzerHelpers_GetNumberString(frame.mData1, Decimal, 1, parityCmd_str, 4);
        snprintf(frame_str, sizeof(frame_str), "CmdParity: %s", parityCmd_str);
      }
      break;

    case RffeBusParkField:
      snprintf(frame_str, sizeof(frame_str), "BP");
      break;

    case RffeErrorCaseField:
    default:
      char number1_str[20];
      char number2_str[20];

      AnalyzerHelpers_GetNumberString(frame.mData1, Hexadecimal, 32, number1_str, 10);
      AnalyzerHelpers_GetNumberString(frame.mData2, Hexadecimal, 32, number2_str, 10);
      snprintf(frame_str, sizeof(frame_str), "Error: %s - %s", number1_str, number2_str);
      break;
  }

  AnalyzerResults_AddTabularText(&self->super, frame_str);
}

void RFFEAnalyzerResults_GeneratePacketTabularText(RFFEAnalyzerResults* self, U64 packet_id, DisplayBase display_base) {
  packet_id = packet_id;
  display_base = display_base;

  AnalyzerResults_ClearResultStrings(&self->super);
  AnalyzerResults_AddResultString(&self->super, "not supported yet");
}

void RFFEAnalyzerResults_GenerateTransactionTabularText(RFFEAnalyzerResults* self, U64 transaction_id, DisplayBase display_base) {
  transaction_id = transaction_id;
  display_base = display_base;

  AnalyzerResults_ClearResultStrings(&self->super);
  AnalyzerResults_AddResultString(&self->super, "not supported yet");
}

// 函数指针数组，用于封装函数
static const struct RFFEAnalyzerResults_Interface {
  RFFEAnalyzerResults* (*Create)(RFFEAnalyzer*, RFFEAnalyzerSettings*);
  void (*Destroy)(RFFEAnalyzerResults*);
  void (*GenerateBubbleText)(RFFEAnalyzerResults*, U64, Channel*, DisplayBase);
  void (*GenerateExportFile)(RFFEAnalyzerResults*, const char*, DisplayBase, U32);
  void (*GenerateFrameTabularText)(RFFEAnalyzerResults*, U64, DisplayBase);
  void (*GeneratePacketTabularText)(RFFEAnalyzerResults*, U64, DisplayBase);
  void (*GenerateTransactionTabularText)(RFFEAnalyzerResults*, U64, DisplayBase);
} RFFEAnalyzerResults_Interface = {
  RFFEAnalyzerResults_Create,
  RFFEAnalyzerResults_Destroy,
  RFFEAnalyzerResults_GenerateBubbleText,
  RFFEAnalyzerResults_GenerateExportFile,
  RFFEAnalyzerResults_GenerateFrameTabularText,
  RFFEAnalyzerResults_GeneratePacketTabularText,
  RFFEAnalyzerResults_GenerateTransactionTabularText
};


// 提供外部接口，方便其他模块调用
const struct RFFEAnalyzerResults_Interface* GetRFFEAnalyzerResultsInterface() {
  return &RFFEAnalyzerResults_Interface;
}
