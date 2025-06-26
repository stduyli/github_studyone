#include "RFFEUtil.h"

typedef enum {
    RFFETypeExtWrite,
    RFFETypeReserved,
    RFFETypeMasterRead,
    RFFETypeMasterWrite,
    RFFETypeMasterHandoff,
    RFFETypeInterrupt,
    RFFETypeExtRead,
    RFFETypeExtLongWrite,
    RFFETypeExtLongRead,
    RFFETypeNormalWrite,
    RFFETypeNormalRead,
    RFFETypeWrite0,
} RffeCommandFieldType;

RffeCommandFieldType decodeRFFECmdFrame(U8 cmd) {
    if (cmd < 0x10) {
        return RFFETypeExtWrite;
    } else if ((cmd >= 0x10) && (cmd < 0x1C)) {
        return RFFETypeReserved;
    } else if (cmd == 0x1C) {
        return RFFETypeMasterRead;
    } else if (cmd == 0x1D) {
        return RFFETypeMasterWrite;
    } else if (cmd == 0x1E) {
        return RFFETypeMasterHandoff;
    } else if (cmd == 0x1F) {
        return RFFETypeInterrupt;
    } else if ((cmd >= 0x20) && (cmd < 0x30)) {
        return RFFETypeExtRead;
    } else if ((cmd >= 0x30) && (cmd < 0x38)) {
        return RFFETypeExtLongWrite;
    } else if ((cmd >= 0x38) && (cmd < 0x40)) {
        return RFFETypeExtLongRead;
    } else if ((cmd >= 0x40) && (cmd < 0x60)) {
        return RFFETypeNormalWrite;
    } else if ((cmd >= 0x60) && (cmd < 0x80)) {
        return RFFETypeNormalRead;
    } else {
        return RFFETypeWrite0;
    }
}

// Note:  This returns the number of bytes expected MINUS one
// (This is due to how command lengths are encoded in the RFFE Commands
U8 byteCount(U8 cmd) {
    if (cmd < 0x10) { // ExtWr
        return (cmd & 0x0F);
    } else if ((cmd >= 0x10) && (cmd < 0x1C)) { // Rsvd
        return 0;
    } else if ((cmd >= 0x1C) && (cmd < 0x1E)) { // Master Rd/Wr
        return 1;                                 // 2 bytes
    } else if (cmd == 0x1E) {                   // Master Handoff
        return 0;                                 // 1 byte
    } else if (cmd == 0x1F) {                   // Interrupt - non-byte sized length, handle as special case
        return 0;
    } else if ((cmd >= 0x20) && (cmd < 0x30)) { // ExtRd
        return (cmd & 0x0F);
    } else if ((cmd >= 0x30) && (cmd < 0x40)) { // Long Rd/Wr
        return (cmd & 0x07);
    } else if ((cmd >= 0x40) && (cmd < 0x80)) { // Rd/Wr
        return 0;
    } else { // Write0
        return 0;
    }
}

// Take a 32 bit unsigned so we can handle SA+Cmd
bool CalcParity(U64 val) {
    U8 c = 1;
    U64 v = val;

    // Add one to the bit count until we have zeroed
    // out all the bits in the passed val.  Initialized
    // to '1' for odd parity
    while (v) {
        c += 1;
        v &= (v - 1);
    }
    return (c & 0x1);
}
