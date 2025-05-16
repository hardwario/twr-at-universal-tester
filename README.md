<a href="https://www.hardwario.com/"><img src="https://www.hardwario.com/ci/assets/hw-logo.svg" width="200" alt="HARDWARIO Logo" align="right"></a>

# Firmware Skeleton for HARDWARIO TOWER UNIVERSAL TESTER

[![Travis](https://travis-ci.org/hardwario/twr-at-universal-tester.svg?branch=master)](https://travis-ci.org/hardwario/twr-at-universal-tester)
[![Release](https://img.shields.io/github/release/hardwario/twr-at-universal-tester.svg)](https://github.com/hardwario/twr-at-universal-tester/releases)
[![License](https://img.shields.io/github/license/hardwario/twr-at-universal-tester.svg)](https://github.com/hardwario/twr-at-universal-tester/blob/master/LICENSE)
[![Twitter](https://img.shields.io/twitter/follow/hardwario_en.svg?style=social&label=Follow)](https://twitter.com/hardwario_en)

This repository contains firmware at universal tester.

## Commands
```
> AT$HELP
ATI Request product information
AT+CLAC List all available AT commands
AT$ADC Read ADC value
AT$ADCV Read ADC voltage
AT$GPIOC Configure GPIO pin
AT$GPIOG Get GPIO pin value
AT$GPIOS Set GPIO pin value
AT$I2CMR I2C memory read
AT$I2CMW I2C memory write
AT$I2CSCAN Scan I2C bus
AT$RELAY Control relay
AT$REBOOT Reboot
AT$HELP This help
OK
```

## I2C Scan

```
> AT$I2CSCAN=0
$SCAN: 0x19,0x49
OK
```

## GPIO
Set value
for example: On / Off led on Core module
```
> AT$GPIOS=18,1
OK
> AT$GPIOS=18,0
OK
```

Read value
```
> AT$GPIOG=19
$GPIOG: 0
OK
> AT$GPIOG=19
$GPIOG: 1
OK
```


## License

This project is licensed under the [MIT License](https://opensource.org/licenses/MIT/) - see the [LICENSE](LICENSE) file for details.

---

Made with &#x2764;&nbsp; by [**HARDWARIO a.s.**](https://www.hardwario.com/) in the heart of Europe.
