# Wokwi HDC2010 Chip

Simulates support for the HDC2010 chip.

## Pin names

| Name | Description              |
| ---- | ------------------------ |
| IN   | Input signal             |
| OUT  | Output (inverted) signal |
| GND  | Ground                   |
| VCC  | Supply voltage           |

## Usage

To use this chip in your project, include it as a dependency in your `diagram.json` file:

```json
  "dependencies": {
    "chip-hdc2010": "github:ci4rail/wokwi-hdc2010-chip@1.0.0"
  }
```

Then, add the chip to your circuit by adding a `chip-inverter` item to the `parts` section of diagram.json:

```json
  "parts": {
    ...,
    { "type": "chip-hdc2010", "id": "chip1" }
  },
```


