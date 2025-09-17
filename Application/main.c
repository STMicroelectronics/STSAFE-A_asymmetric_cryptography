/**
 ******************************************************************************
 * \file    		main.c
 * \author  		CS application team
 ******************************************************************************
 *           			COPYRIGHT 2022 STMicroelectronics
 *
 * This software is licensed under terms that can be found in the LICENSE file in
 * the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "Drivers/rng/rng.h"
#include "Drivers/uart/uart.h"
#include "stselib.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

/* Defines -------------------------------------------------------------------*/
#define PRINT_RESET "\x1B[0m"
#define PRINT_CLEAR_SCREEN "\x1B[1;1H\x1B[2J"
#define PRINT_RED "\x1B[31m"   /* Red */
#define PRINT_GREEN "\x1B[32m" /* Green */
#define RANDOM_SIZE 32

/* STDIO redirect */
#if defined(__GNUC__) && !defined(__ARMCC_VERSION)
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#define GETCHAR_PROTOTYPE int __io_getchar()
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#define GETCHAR_PROTOTYPE int fgetc(FILE *f)
#endif /* __GNUC__ */
PUTCHAR_PROTOTYPE {
    uart_putc(ch);
    return ch;
}
GETCHAR_PROTOTYPE {
    return uart_getc();
}

void apps_terminal_init(uint32_t baudrate) {
    (void)baudrate;
    /* - Initialize UART for example output log (baud 115200)  */
    uart_init(115200);
    /* Disable I/O buffering for STDOUT stream*/
    setvbuf(stdout, NULL, _IONBF, 0);
    /* - Clear terminal */
    printf(PRINT_RESET PRINT_CLEAR_SCREEN);
}

void apps_print_hex_buffer(uint8_t *buffer, uint16_t buffer_size) {
    uint16_t i;
    for (i = 0; i < buffer_size; i++) {
        if (i % 16 == 0) {
            printf(" \n\r ");
        }
        printf(" 0x%02X", buffer[i]);
    }
}

void apps_randomize_buffer(uint8_t *pBuffer, uint16_t buffer_length) {
    for (uint16_t i = 0; i < buffer_length; i++) {
        *(pBuffer + i) = (rng_generate_random_number() & 0xFF);
    }
}

int main(void) {
    stse_ReturnCode_t stse_ret = STSE_API_INVALID_PARAMETER;
    stse_Handler_t stse_handler;
    uint8_t generated_key_slot_number = 1;
    stse_ecc_key_type_t generated_key_slot_type = STSE_ECC_KT_NIST_P_256;
    uint8_t generated_public_key[stse_ecc_info_table[generated_key_slot_type].public_key_size];
    uint8_t challenge[RANDOM_SIZE];
    uint8_t signature[stse_ecc_info_table[generated_key_slot_type].signature_size];

    /* - Initialize Terminal */
    apps_terminal_init(115200);

    /* - Print Example instruction on terminal */
    printf(PRINT_CLEAR_SCREEN);
    printf("----------------------------------------------------------------------------------------------------------------");
    printf("\n\r-                                  STSAFE-A120 asymmetric cryptography                                         -");
    printf("\n\r----------------------------------------------------------------------------------------------------------------");
    printf("\n\r- This example illustrates how to generate a NIST-P256 key pair through STSAFE-A120 slot number 1              -");
    printf("\n\r- and use it to generate and verify a signature generated over a challenge using the key pair                  -");
    printf("\n\r----------------------------------------------------------------------------------------------------------------");

    /* ## Initialize STSAFE-A1xx device handler */
    stse_ret = stse_set_default_handler_value(&stse_handler);
    if (stse_ret == STSE_OK) {
        printf("\n\n\r - stse_set_default_handler_value : success");
    } else {
        printf("\n\n\r - stse_set_default_handler_value ERROR : 0x%04X\n\r", stse_ret);
        while (1)
            ;
    }

    stse_handler.device_type = STSAFE_A120;
    stse_handler.io.busID = 1; // Needed to use expansion board I2C

    printf("\n\r - Initialize target STSAFE-A120");
    stse_ret = stse_init(&stse_handler);
    if (stse_ret == STSE_OK) {
        printf("\n\n\r - stse_init : success");
    } else {
        printf("\n\n\r - stse_init ERROR : 0x%04X\n\r", stse_ret);
        while (1)
            ;
    }

    /* ## Generate key pair in slot 1 */
    stse_ret = stse_generate_ecc_key_pair(&stse_handler, generated_key_slot_number, generated_key_slot_type, 255, generated_public_key);
    if (stse_ret == STSE_OK) {
        printf("\n\n\r - stse_generate_ecc_key_pair : success");
    } else {
        printf("\n\n\r ## stse_generate_ecc_key_pair ERROR : 0x%04X", stse_ret);
        while (1)
            ;
    }
    printf("\n\r\t o public key : ");
    apps_print_hex_buffer(generated_public_key, sizeof(generated_public_key));

    /* ## Generate challenge to be signed */
    printf("\n\n\r\t o Plain-text challenge :");
    apps_randomize_buffer(challenge, sizeof(challenge));
    apps_print_hex_buffer(challenge, sizeof(challenge));

    /* ## Generate signature over challenge using target STSAFE-A120 */
    stse_ret = stse_ecc_generate_signature(&stse_handler, generated_key_slot_number, generated_key_slot_type, challenge, sizeof(challenge), signature);
    if (stse_ret == STSE_OK) {
        printf("\n\n\r - stse_ecc_generate_signature : success");
    } else {
        printf("\n\n\r - stse_ecc_generate_signature ERROR : 0x%04X", stse_ret);
        while (1)
            ;
    }
    printf("\n\r\t o signature : ");
    apps_print_hex_buffer(signature, sizeof(signature));

    /* ## Verify signature using target STSAFE-A120 */
    stse_ret = stse_platform_ecc_verify(generated_key_slot_type, generated_public_key, challenge, sizeof(challenge), signature);
    if (stse_ret == STSE_OK) {
        printf(PRINT_GREEN "\n\n\r - stse_platform_ecc_verify : signature validated");
    } else {
        printf(PRINT_RED "\n\n\r - stse_platform_ecc_verify ERROR : 0x%04X (invalid signature)", stse_ret);
    }

    printf(PRINT_RESET "\n\r\n\r*#*# STMICROELECTRONICS #*#*\n\r");

    while (1)
        ;
}
