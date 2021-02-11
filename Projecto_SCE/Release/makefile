################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ..\makefile.init

RM := -del 

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include libraries\rhio-LIS2HH12\src\subdir.mk
-include libraries\esp32_can-master\subdir.mk
-include libraries\can_common-master\subdir.mk
-include libraries\Wire\src\subdir.mk
-include libraries\SPI\src\subdir.mk
-include core\core\libb64\subdir.mk
-include core\core\subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(INO_DEPS)),)
-include $(INO_DEPS)
endif
ifneq ($(strip $(PDE_DEPS)),)
-include $(PDE_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ..\makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
AR += \
D:\SCE\workspace_sloeber\Projecto_SCE\Release\arduino.ar \

ELF += \
Projecto_SCE.elf \

HEX += \
Projecto_SCE.hex \


# All Target
all: Projecto_SCE

# Tool invocations
Projecto_SCE: $(HEX)
	@echo 'Building target: $@'
	@echo 'Printing size:'
	"D:\SCE\Sloeber\arduinoPlugin\packages\esp32\tools\xtensa-esp32-elf-gcc\1.22.0-80-g6c4433a-5.2.0/bin/xtensa-esp32-elf-size" -A "D:\SCE\workspace_sloeber\Projecto_SCE\Release/Projecto_SCE.elf"
	@echo 'Finished building target: $@'
	@echo ' '

D:\SCE\workspace_sloeber\Projecto_SCE\Release\arduino.ar: $(AR_OBJ)
	@echo 'Starting archiver'
	"D:\SCE\Sloeber\arduinoPlugin\packages\esp32\tools\xtensa-esp32-elf-gcc\1.22.0-80-g6c4433a-5.2.0/bin/xtensa-esp32-elf-ar" cru     D:\SCE\workspace_sloeber\Projecto_SCE\Release/arduino.ar $(AR_OBJ)
	@echo 'Finished building: $@'
	@echo ' '

Projecto_SCE.elf: $(LINK_OBJ) $(AR)
	@echo 'Starting combiner'
	"D:\SCE\Sloeber\arduinoPlugin\packages\esp32\tools\xtensa-esp32-elf-gcc\1.22.0-80-g6c4433a-5.2.0/bin/xtensa-esp32-elf-gcc" -nostdlib "-LD:\SCE\Sloeber\/arduinoPlugin/packages/esp32/hardware/esp32/1.0.4/tools/sdk/lib" "-LD:\SCE\Sloeber\/arduinoPlugin/packages/esp32/hardware/esp32/1.0.4/tools/sdk/ld" -T esp32_out.ld -T esp32.common.ld -T esp32.rom.ld -T esp32.peripherals.ld -T esp32.rom.libgcc.ld -T esp32.rom.spiram_incompatible_fns.ld -u ld_include_panic_highint_hdl -u call_user_start_cpu0 -Wl,--gc-sections -Wl,-static -Wl,--undefined=uxTopUsedPriority  -u __cxa_guard_dummy -u __cxx_fatal_exception  -Wl,--start-group   $(LINK_OBJ)   D:\SCE\workspace_sloeber\Projecto_SCE\Release/arduino.ar  -lgcc -lesp32 -lphy -lesp_http_client -lmbedtls -lrtc -lesp_http_server -lbtdm_app -lspiffs -lbootloader_support -lmdns -lnvs_flash -lfatfs -lpp -lnet80211 -ljsmn -lface_detection -llibsodium -lvfs -ldl_lib -llog -lfreertos -lcxx -lsmartconfig_ack -lxtensa-debug-module -lheap -ltcpip_adapter -lmqtt -lulp -lfd -lfb_gfx -lnghttp -lprotocomm -lsmartconfig -lm -lethernet -limage_util -lc_nano -lsoc -ltcp_transport -lc -lmicro-ecc -lface_recognition -ljson -lwpa_supplicant -lmesh -lesp_https_ota -lwpa2 -lexpat -llwip -lwear_levelling -lapp_update -ldriver -lbt -lespnow -lcoap -lasio -lnewlib -lconsole -lapp_trace -lesp32-camera -lhal -lprotobuf-c -lsdmmc -lcore -lpthread -lcoexist -lfreemodbus -lspi_flash -lesp-tls -lwpa -lwifi_provisioning -lwps -lesp_adc_cal -lesp_event -lopenssl -lesp_ringbuf -lfr  -lstdc++ -Wl,--end-group -Wl,-EL -o "D:\SCE\workspace_sloeber\Projecto_SCE\Release/Projecto_SCE.elf"
	@echo 'Finished building: $@'
	@echo ' '

Projecto_SCE.hex: $(ELF)
	@echo 'Do all objcopy commands'
	${A.RECIPE.OBJCOPY.EEP.PATTERN}
	${A.RECIPE.OBJCOPY.HEX.PATTERN}
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(C_UPPER_DEPS)$(PDE_DEPS)$(C_DEPS)$(AR)$(CC_DEPS)$(AR_OBJ)$(C++_DEPS)$(LINK_OBJ)$(CXX_DEPS)$(ASM_DEPS)$(HEX)$(INO_DEPS)$(SIZEDUMMY)$(S_UPPER_DEPS)$(ELF)$(CPP_DEPS)
	-@echo ' '

.PHONY: all clean dependents

-include ..\makefile.targets
