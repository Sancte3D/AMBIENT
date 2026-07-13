# Component Links — Field Ambience

Pure extraction from the repo (schematic `Datasheet` properties + LCSC codes
in `kicad/jlc_bom.csv`). No fabricated URLs. The **LCSC page** always links
to the part's datasheet; the **Datasheet** column is the direct manufacturer
PDF where the schematic carries one (passives usually don't — use the LCSC
page). Regenerate: this list is derived from `kicad/jlc_bom.csv` + `kicad/*.kicad_sch`.

| Ref(s) | MPN | LCSC page | Direct datasheet |
|---|---|---|---|
| C_HSE1,C_HSE2 | CC0603JRNPO9BN270 | [C107045](https://www.lcsc.com/product-detail/C107045.html) | — |
| U3 | PCM5102APWR | [C107671](https://www.lcsc.com/product-detail/C107671.html) | [PDF](https://www.ti.com/lit/ds/symlink/pcm5102a.pdf) |
| SW_PWR | SSSS811101 | [C109335](https://www.lcsc.com/product-detail/C109335.html) | [PDF](https://wmsc.lcsc.com/wmsc/upload/file/pdf/v2/lcsc/1808041445_ALPSALPINE-SSSS811101_C109335.pdf) |
| D2 | SMAJ5.0A | [C113952](https://www.lcsc.com/product-detail/C113952.html) | — |
| U1 | STM32H743VIT6 | [C114409](https://www.lcsc.com/product-detail/C114409.html) | [PDF](https://www.st.com/resource/en/datasheet/stm32h743vi.pdf) |
| R_ILIM_IN | RC0603FR-071K2L | [C114605](https://www.lcsc.com/product-detail/C114605.html) | — |
| J6,J7 | 1x2 2.54mm pin header (the on-board connector) | [C124375](https://www.lcsc.com/product-detail/C124375.html) | — |
| J3 | 1x8 2.54mm pin header (the on-board connector; the LCD module itself plugs in separately, off-board, see §C / BOM_MASTER §5) | [C124383](https://www.lcsc.com/product-detail/C124383.html) | — |
| LED7,LED11G,LED12G,LED13G,LED14G,LED15G | KT-0603G (Hubei KENTO, pure green 525nm 0603, Vf 3.1V) | [C12624](https://www.lcsc.com/product-detail/C12624.html) | — |
| U_PWR | TPS22918DBVR | [C131941](https://www.lcsc.com/product-detail/C131941.html) | [PDF](https://www.ti.com/lit/ds/symlink/tps22918.pdf) |
| C2,C5,C6c,C7b,C8b,C10,C11,C12,C13,C14,C15,C16,C17,C_BOOST_HF,C_BOOT,C_CPVDD_HF,C_LDOO,C_NRST,C_PCA_VDD_HF,C_QSPI,C_SYS_HF,C_VDD1B,C_VDD2B,C_VDD3B,C_VDD4B,C_VDD5B,C_VDDA2,C_VREF2 | CC0603KRX7R9BB104 (Yageo, 50V X7R) | [C14663](https://www.lcsc.com/product-detail/C14663.html) | — |
| C9b,C_DET_J8,C_FLY,C_FLY_HP,C_HPVSS,C_HP_INL,C_HP_INR,C_PVDDR_HF,C_UPWR_IN,C_VCC,C_VDDA1,C_VNEG,C_VREF,C_VREF1,C_in_L,C_in_R | CL10A105KB8NNNC | [C15849](https://www.lcsc.com/product-detail/C15849.html) | — |
| C1,C6b,C7a,C8a,C_CPVDD_BULK,C_PCA_VDD,C_PWR_SW,C_QSPI2 | CL21A106KAYNNNE (Samsung, 25V X5R) | [C15850](https://www.lcsc.com/product-detail/C15850.html) | — |
| C_HPVDD,C_HP_VDD | CL10A225KP8NNNC | [C1607](https://www.lcsc.com/product-detail/C1607.html) | — |
| U8 | TPS61089RNR | [C165129](https://www.lcsc.com/product-detail/C165129.html) | [PDF](https://www.ti.com/lit/ds/symlink/tps61089.pdf) |
| J1 | TYPE-C-31-M-12 | [C165948](https://www.lcsc.com/product-detail/C165948.html) | [PDF](https://datasheet.lcsc.com/lcsc/1903211732_Korean-Hroparts-Elec-TYPE-C-31-M-12_C165948.pdf) |
| U4 | PAM8403DR-H | [C17337](https://www.lcsc.com/product-detail/C17337.html) | PAM8403H.PDF (Diodes Inc Rev 1-0, Nov 2012) |
| F1 | 1812L300/16GR | [C18198349](https://www.lcsc.com/product-detail/C18198349.html) | [PDF](https://www.littelfuse.com/media?resourcetype=datasheets&itemid=ce0d2bf7-3eb1-4cf6-9c8c-8d3d3a8b1f9a&filename=littelfuse-pptc-1812l-datasheet) |
| FB1,FB2 | BLM18AG601SN1D | [C19330](https://www.lcsc.com/product-detail/C19330.html) | — |
| EN1,EN2,EN3,EN4 | EC11E18244AU (ALPS EC11E, 18 Pulse, 36 Detents, mit Push-Switch, Flat-Shaft 20 mm) | [C202365](https://www.lcsc.com/product-detail/C202365.html) | [PDF](https://tech.alpsalpine.com/e/products/category/encorder/sub/01/series/ec11e/) |
| R_BOOT_SW,R_CHRG,R_ISET | 0603WAF1001T5E | [C21190](https://www.lcsc.com/product-detail/C21190.html) | — |
| LED6,LED11Y,LED12Y,LED13Y,LED14Y,LED15Y | KT-0603Y (Hubei KENTO, yellow 0603, Vf 2.4V) | [C2287](https://www.lcsc.com/product-detail/C2287.html) | — |
| R_ILIM | 0603WAF1743T5E | [C22890](https://www.lcsc.com/product-detail/C22890.html) | — |
| R_MIDI_REF,R_MIDI_TX | 0603WAF2200T5E (220R 0603) | [C22962](https://www.lcsc.com/product-detail/C22962.html) | — |
| R_FSW | 0603WAF3603T5E | [C23146](https://www.lcsc.com/product-detail/C23146.html) | — |
| R_LED6,R_LED7,R_LED8,R_LED9,R_LED10,R_LED11G,R_LED11Y,R_LED12G,R_LED12Y,R_LED13G,R_LED13Y,R_LED14G,R_LED14Y,R_LED15G,R_LED15Y | 0603WAF3900T5E | [C23151](https://www.lcsc.com/product-detail/C23151.html) | — |
| R24 | 0603WAF3902T5E | [C23153](https://www.lcsc.com/product-detail/C23153.html) | — |
| R4,R5 | 0603WAF4701T5E | [C23162](https://www.lcsc.com/product-detail/C23162.html) | — |
| R2,R3 | 0603WAF5101T5E | [C23186](https://www.lcsc.com/product-detail/C23186.html) | — |
| R_LO_L,R_LO_R | 0603WAF220JT5E | [C23345](https://www.lcsc.com/product-detail/C23345.html) | — |
| C_VCAP1,C_VCAP2 | CL10A225KO8NNNC | [C23630](https://www.lcsc.com/product-detail/C23630.html) | — |
| C_BULK2 | CL32A107MPVNNNE (Samsung, 100uF 10V X5R) | [C23742](https://www.lcsc.com/product-detail/C23742.html) | — |
| R_PWR_PD,R_VBUS_PD | 0603WAF1003T5E | [C25803](https://www.lcsc.com/product-detail/C25803.html) | — |
| R6,R7,R8,R9,R10,R11,R12,R13,R14,R15,R16,R17,R18,R20,R_BAT_DIV_BOT,R_BAT_DIV_TOP,R_BLK_PD,R_BOOT0,R_DET_J8,R_MUTE_PD,R_NRST,R_OE,R_SHDN_PD,R_TS,R_VBUS_SENSE,R_XSMT_PD | 0603WAF1002T5E | [C25804](https://www.lcsc.com/product-detail/C25804.html) | — |
| R23 | 0603WAF1213T5E | [C25809](https://www.lcsc.com/product-detail/C25809.html) | — |
| U6 | PCA9685PW,118 | [C2678753](https://www.lcsc.com/product-detail/C2678753.html) | [PDF](https://www.nxp.com/docs/en/data-sheet/PCA9685.pdf) |
| D1 | USBLC6-2SC6 | [C2687116](https://www.lcsc.com/product-detail/C2687116.html) | — |
| J9 | S2B-PH-SM4-TB(LF)(SN) | [C295747](https://www.lcsc.com/product-detail/C295747.html) | — |
| U9 | APS6404L-3SQN-SN | [C3028887](https://www.lcsc.com/product-detail/C3028887.html) | [PDF](https://www.lcsc.com/product-detail/C3028887.html) |
| SW6,SW7,SW8,SW9,SW10 | HX B3F-4055-Y | [C36498965](https://www.lcsc.com/product-detail/C36498965.html) | [PDF](https://jlcpcb.com/partdetail/C36498965) |
| L1 | SWPA6045S2R2NT | [C36500 @ JLC](https://jlcpcb.com/partdetail/C36500) (LCSC retail page is dead — part lives in the JLC assembly catalog) | — |
| SW1,SW2,SW3,SW4,SW5 | CPG135001D01 (Kailh Choc V1, red/linear) | [C400229](https://www.lcsc.com/product-detail/C400229.html) | [PDF](https://cdn-shop.adafruit.com/product-files/5113/CHOC+keyswitch_Kailh-CPG135001D01_C400229.pdf) |
| R_VOL_L,R_VOL_R | 0603WAF2002T5E | [C4184](https://www.lcsc.com/product-detail/C4184.html) | — |
| R_COMP | 0603WAF6201T5E | [C4260](https://www.lcsc.com/product-detail/C4260.html) | — |
| J8,J10 | PJ-320D (3.5mm TRS w/ switch) | [C431535](https://www.lcsc.com/product-detail/C431535.html) | — |
| F2 | SMD1812P260TF/16 | [C438899](https://www.lcsc.com/product-detail/C438899.html) | — |
| C_BULK | TPSE477K010R0100 (Kyocera AVX, Polymer-Tantal) | [C444831](https://www.lcsc.com/product-detail/C444831.html) | — |
| C9,C_BAT,C_BOOST_OUT,C_BOOST_OUT2,C_BOOST_OUT3,C_PVDDR,C_SYS1,C_VDD1A,C_VDD2A,C_VDD3A,C_VDD4A,C_VDD5A | CL21A226MAQNNNE | [C45783](https://www.lcsc.com/product-detail/C45783.html) | — |
| U5 | AP7361C-33Y5-13 | [C460397](https://www.lcsc.com/product-detail/C460397.html) | [PDF](https://www.mouser.de/datasheet/3/175/1/AP7361.pdf) |
| C_CHG_IN,C_LDO_IN,C_LDO_OUT | GRM188R61A475KE15D | [C46653](https://www.lcsc.com/product-detail/C46653.html) | — |
| U2 | MCP23017-E/SS | [C506653](https://www.lcsc.com/product-detail/C506653.html) | [PDF](https://ww1.microchip.com/downloads/en/DeviceDoc/20001952C.pdf) |
| U7 | BQ24074RGTR | [C54313](https://www.lcsc.com/product-detail/C54313.html) | [PDF](https://www.ti.com/lit/ds/symlink/bq24074.pdf) |
| C5b,C_BAT_FILT,C_COMP | 0603B103K500NT | [C57112](https://www.lcsc.com/product-detail/C57112.html) | — |
| Y1 | ABLS-8.000MHZ-B4-T | [C596838](https://www.lcsc.com/product-detail/C596838.html) | [PDF](https://abracon.com/Resonators/ABLS.pdf) |
| U11 | TPA6132A2RTER | [C69901](https://www.lcsc.com/product-detail/C69901.html) | [PDF](https://www.ti.com/lit/ds/symlink/tpa6132a2.pdf) |
| SW_BOOT | TS-1088-AR02016 | [C720477](https://www.lcsc.com/product-detail/C720477.html) | — |
| Q2 | 2N7002,215 | [C8545](https://www.lcsc.com/product-detail/C8545.html) | — |
| D3 | SS34 | [C8678](https://www.lcsc.com/product-detail/C8678.html) | — |
| LED_CHRG | XL-1608UOC-06 | [C965800](https://www.lcsc.com/product-detail/C965800.html) | — |
| LED8,LED9,LED10 | XL-1608UWC-04 (warm-white 0603) | [C965808](https://www.lcsc.com/product-detail/C965808.html) | — |
