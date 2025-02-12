// Wokwi Custom Chip - For information and examples see:
// https://link.wokwi.com/custom-chips-alpha
//
// SPDX-License-Identifier: MIT
// Copyright (C) 2022 Uri Shaked / wokwi.com

#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REG_SIZE 0x100

const int BASE_ADDRESS = 0x40;

typedef enum {
  IDLE,
  DATA
} chip_i2c_state_t;

typedef struct {
  chip_i2c_state_t state;
  pin_t pin_addr;
  pin_t pin_drdy;
  uint8_t address_register;
  uint8_t regs[REG_SIZE];
  uint32_t temp_attr;
  uint32_t hum_attr;
  timer_t meas_timer;
} chip_state_t;


static bool on_i2c_connect(void *user_data, uint32_t address, bool connect);
static uint8_t on_i2c_read(void *user_data);
static bool on_i2c_write(void *user_data, uint8_t data);
static void on_i2c_disconnect(void *user_data);


static uint8_t get_addr(chip_state_t *chip) {
  uint8_t address = BASE_ADDRESS;
  address |= pin_read(chip->pin_addr);
  return address;
}

static bool is_readonly(uint8_t address) {
  if (address >= 0x4 && address <= 0xf) {
    return false;
  }
  return true;
}
static uint16_t temp2raw(float temp) {
  return (uint16_t)((temp + 40) * 65536 / 165);
}

static uint16_t hum2raw(float hum) {
  return (uint16_t)(hum * 65536 / 100);
}

static void on_meas_timer(void *user_data) {
  chip_state_t *chip = user_data;
  float temp = attr_read_float(chip->temp_attr);
  float hum = attr_read_float(chip->hum_attr);
  printf("Measuring: temp=%f, hum=%f\n", temp, hum);
  uint16_t temp_raw = temp2raw(temp);
  uint16_t hum_raw = hum2raw(hum);
  chip->regs[0x0] = temp_raw & 0xff;
  chip->regs[0x1] = temp_raw >> 8;
  chip->regs[0x2] = hum_raw & 0xff;
  chip->regs[0x3] = hum_raw >> 8;
  chip->regs[0x0f] &= ~0x1; // Clear the measurement bit
  pin_write(chip->pin_drdy, 1);
}

static void write_meas_reg(chip_state_t *chip, uint8_t data) {
  if (data & 0x1) {
    printf("Starting measurement\n");
    timer_start(chip->meas_timer, 660, false);
    pin_write(chip->pin_drdy, 0);
  } 
}

void chip_init(void) {
  chip_state_t *chip = malloc(sizeof(chip_state_t));
  chip->pin_addr = pin_init("ADDR", INPUT);
  chip->pin_drdy = pin_init("DRDY", OUTPUT);
  uint8_t address = get_addr(chip);

  chip->temp_attr = attr_init_float("temperature", 25.0);
  chip->hum_attr = attr_init_float("humidity", 50.0);
  chip->meas_timer = timer_init(&(timer_config_t){
    .user_data = chip,
    .callback = on_meas_timer,
  });

  chip->address_register = 0;
  memset(chip->regs, 0, REG_SIZE);
  chip->regs[0xfc] = 0x49;
  chip->regs[0xfd] = 0x54;
  chip->regs[0xfe] = 0xd0;
  chip->regs[0xff] = 0x07;

  chip->state = IDLE;
  const i2c_config_t i2c_config = {
    .user_data = chip,
    .address = address,
    .scl = pin_init("SCL", INPUT),
    .sda = pin_init("SDA", INPUT),
    .connect = on_i2c_connect,
    .read = on_i2c_read,
    .write = on_i2c_write,
    .disconnect = on_i2c_disconnect, // Optional
  };
  i2c_init(&i2c_config);

  // The following message will appear in the browser's DevTools console:
  printf("Hello from HDC2010 at address %x\n", address);
}

bool on_i2c_connect(void *user_data, uint32_t address, bool connect) {
  printf("I2C %s at address %x\n", connect ? "connect" : "disconnect", address);
  return true; /* Ack */
}

uint8_t on_i2c_read(void *user_data) {
  chip_state_t *chip = user_data;
  uint8_t data = chip->regs[chip->address_register];
  printf("Reading from register %x: %x\n", chip->address_register, data);
  chip->address_register = (chip->address_register + 1) & 0xff;
  return data;
}

bool on_i2c_write(void *user_data, uint8_t data) {
  chip_state_t *chip = user_data;
  switch(chip->state)
  {
    case IDLE:
      printf("Writing to address register: %x\n", data);
      chip->address_register = data;
      chip->state = DATA;
    break;
    case DATA:
      if (is_readonly(chip->address_register)) {
        return false; // Nack
      }
      printf("Writing to register %x: %x\n", chip->address_register, data);
      chip->regs[chip->address_register] = data;

      switch(chip->address_register){
        case 0x0f:
          write_meas_reg(chip, data);
        break;
      }      
      chip->address_register = (chip->address_register + 1) & 0xff;
    break;
    default:
      printf("error");
    break;
  }
    return true; // Ack
}

void on_i2c_disconnect(void *user_data) {
  chip_state_t *chip = user_data;
  chip->state = IDLE;
}