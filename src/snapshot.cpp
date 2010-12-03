
// ep128emu -- portable Enterprise 128 emulator
// Copyright (C) 2003-2010 Istvan Varga <istvanv@users.sourceforge.net>
// http://sourceforge.net/projects/ep128emu/
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#include "ep128emu.hpp"
#include "z80/z80.hpp"
#include "memory.hpp"
#include "ioports.hpp"
#include "dave.hpp"
#include "nick.hpp"
#include "vm.hpp"
#include "ep128vm.hpp"
#include "ide.hpp"

namespace Ep128 {

  void Ep128VM::saveState(Ep128Emu::File& f)
  {
    ioPorts.saveState(f);
    memory.saveState(f);
    nick.saveState(f);
    dave.saveState(f);
    z80.saveState(f);
    {
      Ep128Emu::File::Buffer  buf;
      buf.setPosition(0);
      buf.writeUInt32(0x01000004);      // version number
      buf.writeByte(memory.getPage(0));
      buf.writeByte(memory.getPage(1));
      buf.writeByte(memory.getPage(2));
      buf.writeByte(memory.getPage(3));
      buf.writeByte(memoryWaitMode & 3);
      buf.writeUInt32(uint32_t(cpuFrequency));
      buf.writeUInt32(uint32_t(daveFrequency));
      buf.writeUInt32(uint32_t(nickFrequency));
      buf.writeUInt32(uint32_t(waitCycleCnt));
      buf.writeUInt32(uint32_t(videoMemoryLatency));
      buf.writeUInt32(uint32_t(videoMemoryLatency_M1));
      buf.writeUInt32(uint32_t(videoMemoryLatency_IO));
      buf.writeBoolean(memoryTimingEnabled);
      buf.writeInt64(cpuCyclesRemaining + 1L);  // +1 for compatibility
      buf.writeInt64(daveCyclesRemaining + 1L);
      buf.writeBoolean(spectrumEmulatorEnabled);
      for (int i = 0; i < 4; i++)
        buf.writeByte(spectrumEmulatorIOPorts[i]);
      buf.writeByte(cmosMemoryRegisterSelect);
      buf.writeInt64(prvRTCTime);
      for (int i = 0; i < 64; i++)
        buf.writeByte(cmosMemory[i]);
      f.addChunk(Ep128Emu::File::EP128EMU_CHUNKTYPE_VM_STATE, buf);
    }
  }

  void Ep128VM::saveMachineConfiguration(Ep128Emu::File& f)
  {
    Ep128Emu::File::Buffer  buf;
    buf.setPosition(0);
    buf.writeUInt32(0x01000002);        // version number
    buf.writeUInt32(uint32_t(cpuFrequency));
    buf.writeUInt32(uint32_t(daveFrequency));
    buf.writeUInt32(uint32_t(nickFrequency));
    buf.writeUInt32(uint32_t(waitCycleCnt));
    buf.writeUInt32(uint32_t(videoMemoryLatency));
    buf.writeUInt32(uint32_t(videoMemoryLatency_M1));
    buf.writeUInt32(uint32_t(videoMemoryLatency_IO));
    buf.writeBoolean(memoryTimingEnabled);
    f.addChunk(Ep128Emu::File::EP128EMU_CHUNKTYPE_VM_CONFIG, buf);
  }

  void Ep128VM::recordDemo(Ep128Emu::File& f)
  {
    // turn off tape motor, stop any previous demo recording or playback,
    // and reset keyboard state
    remoteControlState = 0x00;
    setTapeMotorState(false);
    dave.setTapeInput(0, 0);
    stopDemo();
    for (int i = 0; i < 128; i++)
      dave.setKeyboardState(i, 0);
    // floppy and IDE emulation are disabled while recording or playing demo
    currentFloppyDrive = 0xFF;
    floppyDrives[0].reset();
    floppyDrives[1].reset();
    floppyDrives[2].reset();
    floppyDrives[3].reset();
    ideInterface->reset(0);
    z80.closeAllFiles();
    // save full snapshot, including timing and clock frequency settings
    saveMachineConfiguration(f);
    saveState(f);
    demoBuffer.clear();
    demoBuffer.writeUInt32(0x00020009); // version 2.0.9
    demoFile = &f;
    isRecordingDemo = true;
    setCallback(&demoRecordCallback, this, true);
    demoTimeCnt = 0U;
  }

  void Ep128VM::stopDemo()
  {
    stopDemoPlayback();
    stopDemoRecording(true);
  }

  bool Ep128VM::getIsRecordingDemo()
  {
    if (demoFile != (Ep128Emu::File *) 0 && !isRecordingDemo)
      stopDemoRecording(true);
    return isRecordingDemo;
  }

  bool Ep128VM::getIsPlayingDemo() const
  {
    return isPlayingDemo;
  }

  // --------------------------------------------------------------------------

  void Ep128VM::loadState(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
    if (!(version >= 0x01000000 && version <= 0x01000004)) {
      buf.setPosition(buf.getDataSize());
      throw Ep128Emu::Exception("incompatible ep128 snapshot version");
    }
    remoteControlState = 0x00;
    setTapeMotorState(false);
    stopDemo();
    snapshotLoadFlag = true;
    // reset floppy and IDE emulation, as the state of these is not saved
    currentFloppyDrive = 0xFF;
    floppyDrives[0].reset();
    floppyDrives[1].reset();
    floppyDrives[2].reset();
    floppyDrives[3].reset();
    ideInterface->reset(3);
    z80.closeAllFiles();
    try {
      uint8_t   p0, p1, p2, p3;
      p0 = buf.readByte();
      p1 = buf.readByte();
      p2 = buf.readByte();
      p3 = buf.readByte();
      pageTable[0] = p0;
      memory.setPage(0, p0);
      pageTable[1] = p1;
      memory.setPage(1, p1);
      pageTable[2] = p2;
      memory.setPage(2, p2);
      pageTable[3] = p3;
      memory.setPage(3, p3);
      memoryWaitMode = buf.readByte() & 3;
      setMemoryWaitTiming();
      uint32_t  tmpCPUFrequency = buf.readUInt32();
      uint32_t  tmpDaveFrequency = buf.readUInt32();
      uint32_t  tmpNickFrequency = buf.readUInt32();
      if (version >= 0x01000003) {
        (void) buf.readUInt32();        // waitCycleCnt (ignored)
        (void) buf.readUInt32();        // videoMemoryLatency (ignored)
        if (version >= 0x01000004) {
          (void) buf.readUInt32();      // videoMemoryLatency_M1 (ignored)
          (void) buf.readUInt32();      // videoMemoryLatency_IO (ignored)
        }
      }
      else if (version < 0x01000002) {
        (void) buf.readUInt32();        // was videoMemoryLatency
      }
      (void) buf.readBoolean();         // memoryTimingEnabled (ignored)
      int64_t   tmp[2];
      tmp[0] = buf.readInt64();
      if (version < 0x01000002)
        (void) buf.readInt64();         // was cpuSyncToNickCnt
      tmp[1] = buf.readInt64();
      if (tmpCPUFrequency == cpuFrequency &&
          tmpDaveFrequency == daveFrequency &&
          tmpNickFrequency == nickFrequency) {
        cpuCyclesRemaining = tmp[0] - 1L;       // -1 for compatibility
        daveCyclesRemaining = tmp[1] - 1L;
      }
      else {
        cpuCyclesRemaining = -1L;
        daveCyclesRemaining = -1L;
      }
      if (version >= 0x01000001) {
        ioPorts.writeDebug(0x44, uint8_t(buf.readBoolean()) << 7);
        for (int i = 0; i < 4; i++)
          spectrumEmulatorIOPorts[i] = buf.readByte();
        cmosMemoryRegisterSelect = buf.readByte();
        prvRTCTime = buf.readInt64();
        for (int i = 0; i < 64; i++)
          cmosMemory[i] = buf.readByte();
      }
      else {
        // loading snapshot saved by old version without Spectrum emulator
        // and real time clock support
        ioPorts.writeDebug(0x44, 0x00);
        for (int i = 0; i < 4; i++)
          spectrumEmulatorIOPorts[i] = 0xFF;
        resetCMOSMemory();
      }
      if (buf.getPosition() != buf.getDataSize())
        throw Ep128Emu::Exception("trailing garbage at end of "
                                  "ep128 snapshot data");
    }
    catch (...) {
      this->reset(true);
      throw;
    }
  }

  void Ep128VM::loadMachineConfiguration(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
    if (!(version >= 0x01000000 && version <= 0x01000002)) {
      buf.setPosition(buf.getDataSize());
      throw Ep128Emu::Exception("incompatible ep128 "
                                "machine configuration format");
    }
    try {
      setCPUFrequency(buf.readUInt32());
      setSoundClockFrequency(buf.readUInt32());
      setVideoFrequency(buf.readUInt32());
      if (version >= 0x01000001) {
        uint32_t  tmpWaitCycleCnt = buf.readUInt32();
        uint32_t  tmpVideoMemoryLatency = buf.readUInt32();
        uint32_t  tmpVideoMemoryLatency_M1 = tmpVideoMemoryLatency - 2461U;
        uint32_t  tmpVideoMemoryLatency_IO = tmpVideoMemoryLatency + 2461U;
        if (version >= 0x01000002) {
          tmpVideoMemoryLatency_M1 = buf.readUInt32();
          tmpVideoMemoryLatency_IO = buf.readUInt32();
        }
        waitCycleCnt = int(tmpWaitCycleCnt);
        videoMemoryLatency = int(tmpVideoMemoryLatency);
        videoMemoryLatency_M1 = int(tmpVideoMemoryLatency_M1);
        videoMemoryLatency_IO = int(tmpVideoMemoryLatency_IO);
        updateTimingParameters();
      }
      else {
        (void) buf.readUInt32();        // was videoMemoryLatency
      }
      setEnableMemoryTimingEmulation(buf.readBoolean());
      if (buf.getPosition() != buf.getDataSize())
        throw Ep128Emu::Exception("trailing garbage at end of "
                                  "ep128 machine configuration data");
    }
    catch (...) {
      this->reset(true);
      throw;
    }
  }

  void Ep128VM::loadDemo(Ep128Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
#if 0
    if (version != 0x00020009) {
      buf.setPosition(buf.getDataSize());
      throw Ep128Emu::Exception("incompatible ep128 demo format");
    }
#endif
    (void) version;
    // turn off tape motor, stop any previous demo recording or playback,
    // and reset keyboard state
    remoteControlState = 0x00;
    setTapeMotorState(false);
    dave.setTapeInput(0, 0);
    stopDemo();
    for (int i = 0; i < 128; i++)
      dave.setKeyboardState(i, 0);
    // floppy and IDE emulation are disabled while recording or playing demo
    currentFloppyDrive = 0xFF;
    floppyDrives[0].reset();
    floppyDrives[1].reset();
    floppyDrives[2].reset();
    floppyDrives[3].reset();
    ideInterface->reset(0);
    // initialize time counter with first delta time
    demoTimeCnt = buf.readUIntVLen();
    isPlayingDemo = true;
    setCallback(&demoPlayCallback, this, true);
    // copy any remaining demo data to local buffer
    demoBuffer.clear();
    demoBuffer.writeData(buf.getData() + buf.getPosition(),
                         buf.getDataSize() - buf.getPosition());
    demoBuffer.setPosition(0);
  }

  class ChunkType_Ep128VMConfig : public Ep128Emu::File::ChunkTypeHandler {
   private:
    Ep128VM&  ref;
   public:
    ChunkType_Ep128VMConfig(Ep128VM& ref_)
      : Ep128Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_Ep128VMConfig()
    {
    }
    virtual Ep128Emu::File::ChunkType getChunkType() const
    {
      return Ep128Emu::File::EP128EMU_CHUNKTYPE_VM_CONFIG;
    }
    virtual void processChunk(Ep128Emu::File::Buffer& buf)
    {
      ref.loadMachineConfiguration(buf);
    }
  };

  class ChunkType_Ep128VMSnapshot : public Ep128Emu::File::ChunkTypeHandler {
   private:
    Ep128VM&  ref;
   public:
    ChunkType_Ep128VMSnapshot(Ep128VM& ref_)
      : Ep128Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_Ep128VMSnapshot()
    {
    }
    virtual Ep128Emu::File::ChunkType getChunkType() const
    {
      return Ep128Emu::File::EP128EMU_CHUNKTYPE_VM_STATE;
    }
    virtual void processChunk(Ep128Emu::File::Buffer& buf)
    {
      ref.loadState(buf);
    }
  };

  class ChunkType_DemoStream : public Ep128Emu::File::ChunkTypeHandler {
   private:
    Ep128VM&  ref;
   public:
    ChunkType_DemoStream(Ep128VM& ref_)
      : Ep128Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_DemoStream()
    {
    }
    virtual Ep128Emu::File::ChunkType getChunkType() const
    {
      return Ep128Emu::File::EP128EMU_CHUNKTYPE_DEMO_STREAM;
    }
    virtual void processChunk(Ep128Emu::File::Buffer& buf)
    {
      ref.loadDemo(buf);
    }
  };

  void Ep128VM::registerChunkTypes(Ep128Emu::File& f)
  {
    ChunkType_Ep128VMConfig   *p1 = (ChunkType_Ep128VMConfig *) 0;
    ChunkType_Ep128VMSnapshot *p2 = (ChunkType_Ep128VMSnapshot *) 0;
    ChunkType_DemoStream      *p3 = (ChunkType_DemoStream *) 0;

    try {
      p1 = new ChunkType_Ep128VMConfig(*this);
      f.registerChunkType(p1);
      p1 = (ChunkType_Ep128VMConfig *) 0;
      p2 = new ChunkType_Ep128VMSnapshot(*this);
      f.registerChunkType(p2);
      p2 = (ChunkType_Ep128VMSnapshot *) 0;
      p3 = new ChunkType_DemoStream(*this);
      f.registerChunkType(p3);
      p3 = (ChunkType_DemoStream *) 0;
    }
    catch (...) {
      if (p1)
        delete p1;
      if (p2)
        delete p2;
      if (p3)
        delete p3;
      throw;
    }
    ioPorts.registerChunkType(f);
    z80.registerChunkType(f);
    memory.registerChunkType(f);
    dave.registerChunkType(f);
    nick.registerChunkType(f);
  }

}       // namespace Ep128

