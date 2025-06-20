# greenpak-configurator
An app that controls an SLG46826 chip by following a simple script

This purrpose of this project is to enable reconfiguration of SLG46826 chip (theoretically, others can fit as well) from pc by proxying the commands through ESP32-C3-super-mini board.

Clearly, the app consists of two parts:
1. Cli app running on PC that parses the script and transmits it as a sequence of 2-byte commands to the esp.
2. FreeRTOS app running on the esp that receives the commands from pc, prepares the necessary data and then patches the project in the chip (currently, writes/reads to/from its volatile registers) through I2c protocol.

For a clearer picture, please, consult the diagram below:

![gls - Page 1 (1)](https://github.com/user-attachments/assets/45df6037-737f-42b6-9720-fdd8b168ae97)

Attached you can find two GreenPAK projects for SLG46826 that only make sense for GreenPAK Training Board #2 that features 4 controllable LEDs.
