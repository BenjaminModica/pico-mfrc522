/**
 * Test Project for the MFRC522 RFID reader
 * 
 * The MFRC522 has the capability to perform a digital self test. The self test is started by
 * using the following procedure:
 * 1. Perform a soft reset.
 * 2. Clear the internal buffer by writing 25 bytes of 00h and implement the Config
 * command.
 * 3. Enable the self test by writing 09h to the AutoTestReg register.
 * 4. Write 00h to the FIFO buffer.
 * 5. Start the self test with the CalcCRC command.
 * 6. The self test is initiated.
 * 7. When the self test has completed, the FIFO buffer contains the following 64 bytes:
 * 
 * FIFO buffer byte values for MFRC522 version 1.0:
 * 00h, C6h, 37h, D5h, 32h, B7h, 57h, 5Ch,
 * C2h, D8h, 7Ch, 4Dh, D9h, 70h, C7h, 73h,
 * 10h, E6h, D2h, AAh, 5Eh, A1h, 3Eh, 5Ah,
 * 14h, AFh, 30h, 61h, C9h, 70h, DBh, 2Eh,
 * 64h, 22h, 72h, B5h, BDh, 65h, F4h, ECh,
 * 22h, BCh, D3h, 72h, 35h, CDh, AAh, 41h,
 * 1Fh, A7h, F3h, 53h, 14h, DEh, 7Eh, 02h,
 * D9h, 0Fh, B5h, 5Eh, 25h, 1Dh, 29h, 79h
 * FIFO buffer byte values for MFRC522 version 2.0:
 * 00h, EBh, 66h, BAh, 57h, BFh, 23h, 95h,
 * D0h, E3h, 0Dh, 3Dh, 27h, 89h, 5Ch, DEh,
 * 9Dh, 3Bh, A7h, 00h, 21h, 5Bh, 89h, 82h,
 * 51h, 3Ah, EBh, 02h, 0Ch, A5h, 00h, 49h,
 * 7Ch, 84h, 4Dh, B3h, CCh, D2h, 1Bh, 81h,
 * 5Dh, 48h, 76h, D5h, 71h, 061h, 21h, A9h,
 * 86h, 96h, 83h, 38h, CFh, 9Dh, 5Bh, 6Dh,
 * DCh, 15h, BAh, 3Eh, 7Dh, 95h, 03Bh, 2Fh
*/

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

#define LED_PIN 25
#define RESET_PIN 20

const uint8_t PROGMEM[] = {
	0x00, 0xEB, 0x66, 0xBA, 0x57, 0xBF, 0x23, 0x95,
	0xD0, 0xE3, 0x0D, 0x3D, 0x27, 0x89, 0x5C, 0xDE,
	0x9D, 0x3B, 0xA7, 0x00, 0x21, 0x5B, 0x89, 0x82,
	0x51, 0x3A, 0xEB, 0x02, 0x0C, 0xA5, 0x00, 0x49,
	0x7C, 0x84, 0x4D, 0xB3, 0xCC, 0xD2, 0x1B, 0x81,
	0x5D, 0x48, 0x76, 0xD5, 0x71, 0x61, 0x21, 0xA9,
	0x86, 0x96, 0x83, 0x38, 0xCF, 0x9D, 0x5B, 0x6D,
	0xDC, 0x15, 0xBA, 0x3E, 0x7D, 0x95, 0x3B, 0x2F
};

//Registers
static const uint8_t CommandReg = 0x01 << 1; //LSB not used in registers, every register address is shifted
static const uint8_t FIFODataReg = 0x09 << 1;
static const uint8_t FIFOLevelReg = 0x0A << 1;
static const uint8_t AutoTestReg = 0x36 << 1;
static const uint8_t VersionReg = 0x37 << 1;

//Commands
static const uint8_t SoftReset = 0x0F;
static const uint8_t Mem = 0x01;
static const uint8_t EnableSelfTest = 0x09;
static const uint8_t CalcCRC = 0x03;

void reg_write(spi_inst_t *spi, const uint cs, const uint8_t reg, const uint8_t data);
void reg_read(spi_inst_t *spi, const uint cs, const uint8_t reg, uint8_t *buf);

static inline void cs_select(const uint cs);
static inline void cs_deselect(const uint cs);

uint8_t self_test();

int main() {
    gpio_init(LED_PIN);
    gpio_init(RESET_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_set_dir(RESET_PIN, GPIO_OUT);

    const uint cs_pin = 17;
    const uint sck_pin = 18;
    const uint mosi_pin = 19;
    const uint miso_pin = 16;

    stdio_init_all();

    gpio_put(RESET_PIN, 0);
    sleep_ms(1000);
    gpio_put(RESET_PIN, 1);

    gpio_init(cs_pin);
    gpio_set_dir(cs_pin, GPIO_OUT);
    gpio_put(cs_pin, 1);

    spi_init(spi0, 1000000);

    //Test att ändra MODE!
    spi_set_format(spi0, 8, 0, 0, SPI_MSB_FIRST);

    gpio_set_function(sck_pin, GPIO_FUNC_SPI);
    gpio_set_function(mosi_pin, GPIO_FUNC_SPI);
    gpio_set_function(miso_pin, GPIO_FUNC_SPI);

    sleep_ms(5000);
    printf("Test Started\n\r");

    reg_write(spi0, cs_pin, CommandReg, SoftReset);
    sleep_ms(50); //Allow the chip to reset

    uint8_t version = 0x0A;
    reg_read(spi0, cs_pin, VersionReg, &version);
    printf("Version: %x\n\r", version);

    char test_result = self_test(cs_pin, sck_pin, mosi_pin, miso_pin);

    printf("Test Result: %d\n\r", test_result);

    while (true) {
        sleep_ms(1000);
    }
}

uint8_t self_test(const uint cs_pin, const uint sck_pin, const uint mosi_pin, const uint miso_pin) {
    //Perform a soft reset
    reg_write(spi0, cs_pin, CommandReg, SoftReset);
    sleep_ms(50); //Allow the chip to reset
    printf("Soft reset complete\n\r");

    //Clear the internal buffer by writing 25 bytes of 00h (and implement the config command)??.
    reg_write(spi0, cs_pin, FIFOLevelReg, 0x80); //FIFOLevel indicates nbr of bytes in FIFO buffer. MSB = 1 -> Flush

    for (int i = 0; i < 25; i++) {
        reg_write(spi0, cs_pin, FIFODataReg, 0x00);
    }
    reg_write(spi0, cs_pin, CommandReg, Mem); //Transfer the 25 bytes from FIFO to internal buffer
    printf("Clearing of internal buffer complete\n\r");

    //Enable the self test by writing 09h to the AutoTestReg register.
    reg_write(spi0, cs_pin, AutoTestReg, EnableSelfTest);
    printf("Self test enable complete\n\r");

    //Write 00h to the FIFO buffer.
    reg_write(spi0, cs_pin, FIFODataReg, 0x00);
    printf("Written 0x00 to FIFO buffer\n\r");

    //Start the self test with the CalcCRC command.
    reg_write(spi0, cs_pin, CommandReg, CalcCRC);
    printf("Started self test with the CRC command\n\r");

    //The self test is initiated. Wait for completion
    bool self_test_complete = false;
    while (!self_test_complete) {
        uint8_t buf = 0;
        reg_read(spi0, cs_pin, FIFOLevelReg, &buf);
        //printf("%x\n\r", buf);
        if (buf >= 64) {
            self_test_complete = true;
        }
    }
    printf("Self test completed\n\r");

    //Stop calculating CRC with idle command???

    //When the self test has completed, the FIFO buffer contains the version bytes.
    uint8_t result[64];
    for (int i = 0; i < 64; i++) {
        uint8_t buf = 0;
        reg_read(spi0, cs_pin, FIFODataReg, &buf);
        result[i] = buf;
    }
    printf("Bytes copied to result array\n\r");

    //Check if the received bytes correspond to the versions.
    reg_write(spi0, cs_pin, AutoTestReg, 0x00); //Disable self-test
    printf("Disabled self test\n\r");

    printf("Self test result:\n\r");
    for (uint8_t i = 0; i < 64; i++) {
        printf("%x, ", result[i]);
        if (i % 10 == 0) printf("\n\r");
    } printf("\n\r");

    for (uint8_t i = 0; i < 64; i++) {
		if (result[i] != PROGMEM[i]) {
			return -1;
		}
	}

    return 0;

}

void reg_write(spi_inst_t *spi, const uint cs, const uint8_t reg, const uint8_t data) {
    uint8_t msg[2];
    msg[0] = 0x00 | reg; //First bit defines the mode. MSB = 0 -> write 
    msg[1] = data;

    cs_select(cs);
    spi_write_blocking(spi, msg, 2);
    cs_deselect(cs);
}

void reg_read(spi_inst_t *spi, const uint cs, const uint8_t reg, uint8_t *buf) {
    const uint8_t msg = 0x80 | reg; //First bit defines the mode. MSB = 1 -> read

    cs_select(cs);
    spi_write_blocking(spi, &msg, 1);
    spi_read_blocking(spi, 0, buf, 1);
    cs_deselect(cs);
}

static inline void cs_select(const uint cs) {
    asm volatile("nop \n nop \n nop");
    gpio_put(cs, 0); // Active low
    asm volatile("nop \n nop \n nop");
}
static inline void cs_deselect(const uint cs) {
    asm volatile("nop \n nop \n nop");
    gpio_put(cs, 1);
    asm volatile("nop \n nop \n nop");
}
