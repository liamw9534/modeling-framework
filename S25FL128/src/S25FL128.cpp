#include "S25FL128.h"

#include <functional>
#include <iostream>
#include <cassert>
#include <cstring>

extern "C" {
#include <stdio.h>
}

#define OP_BYTE_OR_PAGE_PROGRAM 			(0x02)
#define OP_READ_ARRAY_03 					(0x03)
#define OP_READ_ARRAY_0B 					(0x0b)
#define OP_WRITE_ENABLE 					(0x06)
#define OP_WRITE_DISABLE 					(0x04)
#define OP_BLOCK_ERASE_256KB 				(0xd8)
#define OP_CHIP_ERASE_60 					(0x60)
#define OP_CHIP_ERASE_C7 					(0xc7)
#define OP_READ_STATUS_REGISTER_BYTE_1 		(0x05)
#define OP_READ_STATUS_REGISTER_BYTE_2 		(0x35)
#define OP_READ_MANUFACTURER_AND_DEVICE_ID 	(0x9f)
#define OP_READ_DEVICE_ID 					(0x90)

#define MANUFACTURER_ID (0x1f)
#define DEVICE_ID_PART_1 (0x85)
#define DEVICE_ID_PART_2 (0x01)
#define DEVICE_CODE (0x13)
#define STATUS_REGISTER_WEL_MASK (1U << 1)


#define MAX_FREQUENCY (104000000)

S25FL128::S25FL128() :
        should_stop_(false),
        status_register1_(0),
        status_register2_(0) {
    int cs_pin_number = GetPinNumber("cs");
    int sck_pin_number = GetPinNumber("sck");
    int si_pin_number = GetPinNumber("si");
    int so_pin_number = GetPinNumber("so");
    wp_pin_number_ = GetPinNumber("wp");

    SpiSlaveConfig spi_config;
    spi_config.mosi_pin_number = si_pin_number;
    spi_config.miso_pin_number = so_pin_number;
    spi_config.ss_pin_number = cs_pin_number;
    spi_config.sclk_pin_number = sck_pin_number;
    spi_config.supported_spi_modes = SPI_MODE_0 | SPI_MODE_3;
    spi_config.max_frequency = MAX_FREQUENCY;
    spi_config.bit_order = MSB_FIRST;

    spi_slave_ = CreateSpiSlave(spi_config);
    MemReset();
}

void S25FL128::Main() {
    while (!should_stop_) {
        uint8_t opcode = 0;

        spi_slave_->WaitForMasterTransmit();

        if (spi_slave_->Transmit(&opcode, nullptr, 1) == 0) {
            continue;
        }

        switch (opcode) {
            case (OP_READ_ARRAY_0B): {
                ReadArray(true);
                break;
            }

            case (OP_READ_ARRAY_03): {
                ReadArray(false);
                break;
            }

            case (OP_BYTE_OR_PAGE_PROGRAM): {
                ByteOrPageProgram();
                break;
            }

            case (OP_WRITE_ENABLE): {
                WriteEnable();
                break;
            }

            case (OP_WRITE_DISABLE): {
                WriteDisable();
                break;
            }

            case (OP_BLOCK_ERASE_256KB): {
                BlockErase(1U << 18);
                break;
            }

            case (OP_CHIP_ERASE_60): {
                ChipErase();
                break;
            }

            case (OP_CHIP_ERASE_C7): {
                ChipErase();
                break;
            }

            case (OP_READ_STATUS_REGISTER_BYTE_1): {
                ReadStatusRegister(STATUS_REGISTER_BYTE_1);
                break;
            }

            case (OP_READ_STATUS_REGISTER_BYTE_2): {
                ReadStatusRegister(STATUS_REGISTER_BYTE_2);
                break;
            }

            case (OP_READ_MANUFACTURER_AND_DEVICE_ID): {
                ReadManufacturerAndDeviceId();
                break;
            }

            case (OP_READ_DEVICE_ID): {
                ReadDeviceId();
                break;
            }

            default: {
                throw std::logic_error("S25FL128: Unsupported opcode " + std::to_string(opcode));
            }
        }
    }
}

void S25FL128::Stop() {
    should_stop_ = true;
}

void S25FL128::ReadArray(bool with_dummy_byte) {
    int address = ReadAddress();
    if (address < 0) return;

    //printf("RD(%08x)\n", address);

    if (with_dummy_byte) {
        if (ReadDummyBytes(1) != 1 || !spi_slave_->IsSsActive()) return;
    }

    bool done = false;
    while (!done) {
        size_t length = spi_slave_->Transmit(nullptr, &memory_[address], MEMORY_SIZE - address);
        if (length == 0) {
            return;
        }
        address = (address + 1) % (int) MEMORY_SIZE;
        done = !spi_slave_->IsSsActive();
    }
}

void S25FL128::ByteOrPageProgram() {
    if (!WelStatus()) {
        throw std::logic_error("S25FL128: Trying to program the device while WEL bit in status register is 0");
    }
    int address = ReadAddress();

    //printf("PP(%08x)\n", address);

    bool done = false;
    while (!done) {
        uint8_t byte = 0;
        size_t length = spi_slave_->Transmit(&byte, nullptr, 1);
        if (length == 0) {
            return;
        }
        memory_[address] = byte;
        //printf("byte[%u] = %02x\n", address, byte);
        address++;
        done = !spi_slave_->IsSsActive();
    }

    SetWel(0);
}

int S25FL128::ReadAddress() {
    uint8_t data[3];
    if (spi_slave_->Transmit(data, nullptr, 3) != 3 || !spi_slave_->IsSsActive()) {
        return -1;
    }

    int address = ((data[0] << 16) | (data[1] << 8) | data[2]);
    if (address >= MEMORY_SIZE || address < 0)
    	std::logic_error("S25FL128: Address out of range: " + std::to_string(address));

    return address;
}

size_t S25FL128::ReadDummyBytes(size_t num) {
    uint8_t* data = (new uint8_t[num]);
    return spi_slave_->Transmit(data, nullptr, num);
}

void S25FL128::SetWel(bool enable) {
    if (enable) {
        status_register1_ |= STATUS_REGISTER_WEL_MASK;
    } else {
        status_register1_ &= ~STATUS_REGISTER_WEL_MASK;
    }
}

bool S25FL128::WelStatus() {
    return (status_register1_ & STATUS_REGISTER_WEL_MASK) != 0;
}

void S25FL128::BlockErase(size_t block_size) {
    if (!WelStatus()) {
        throw std::logic_error("S25FL128: Trying to erase block while WEL bit in status register is 0");
    }

    int address = ReadAddress();
    if (address == -1)
        return;

    //printf("BE(%08x,bs=%08x)\n", address, (unsigned int)block_size);

    address = ((unsigned int) block_size) * (address / (unsigned int) block_size);

    IgnoreUntilSsInactive();

    memset(memory_ + address, 0xff, block_size);
    SetWel(true);
}

void S25FL128::ChipErase() {
    if (!WelStatus()) {
        throw std::logic_error("S25FL128: Trying to erase chip while WEL bit in status register is 0");
    }
    //printf("CE\n");
    IgnoreUntilSsInactive();
    MemReset();
}

void S25FL128::MemReset() {
    memset(memory_, 0xff, MEMORY_SIZE);
}

void S25FL128::WriteEnable() {
    while (spi_slave_->IsSsActive()) {
        ReadDummyBytes(1);
    };
    SetWel(true);
}

void S25FL128::WriteDisable() {
    while (spi_slave_->IsSsActive()) {
        ReadDummyBytes(1);
    };
    SetWel(false);
}

void S25FL128::IgnoreUntilSsInactive() {
    while (spi_slave_->IsSsActive()) {
        ReadDummyBytes(1);
    };
}

void S25FL128::ReadStatusRegister(S25FL128::StatusRegisterByte byte) {
    uint8_t data = byte == STATUS_REGISTER_BYTE_1 ? status_register1_ : status_register2_;

    while (spi_slave_->IsSsActive()) {
        spi_slave_->Transmit(nullptr, &data, 1);
    }
}

void S25FL128::ReadManufacturerAndDeviceId() {
    static uint8_t response[3] = {MANUFACTURER_ID, DEVICE_ID_PART_1, DEVICE_ID_PART_2};
    spi_slave_->Transmit(nullptr, response, sizeof(response));
    IgnoreUntilSsInactive();
}

void S25FL128::ReadDeviceId() {
    static uint8_t response[2] = {MANUFACTURER_ID, DEVICE_CODE};
    int i = 0;
    ReadDummyBytes(3);
    while (spi_slave_->IsSsActive()) {
        spi_slave_->Transmit(nullptr, &response[i], 1);
        i = (i + 1) % 2;
    }
}
