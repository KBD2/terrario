#! /usr/bin/make -f
#  Default Makefile for fxSDK add-ins. This file was probably copied there by
#  the [fxsdk] program.
#---

#
#  Configuration
#

include project.cfg

# Compiler flags
CFLAGSFX := $(CFLAGS) $(CFLAGS_FX) $(INCLUDE)
CFLAGSCG := $(CFLAGS) $(CFLAGS_CG) $(INCLUDE)

# Linker flags
LDFLAGSFX := $(LDFLAGS) $(LDFLAGS_FX)
LDFLAGSCG := $(LDFLAGS) $(LDFLAGS_CG)

# Dependency list generation flags
depflags = -MMD -MT $@ -MF $(@:.o=.d) -MP
# ELF to binary flags
BINFLAGS := -R .bss -R .gint_bss

# G1A and G3A generation flags
NAME_G1A ?= $(NAME)
NAME_G3A ?= $(NAME)
G1AF := -i "$(ICON_FX)" -n "$(NAME_G1A)" --internal="$(INTERNAL)"
G3AF := -n basic:"$(NAME_G3A)" -i uns:"$(ICON_CG_UNS)" -i sel:"$(ICON_CG_SEL)"

ifeq "$(TOOLCHAIN_FX)" ""
TOOLCHAIN_FX := sh3eb-elf
endif

ifeq "$(TOOLCHAIN_CG)" ""
TOOLCHAIN_CG := sh4eb-elf
endif

# fxconv flags
FXCONVFX := --fx --toolchain=$(TOOLCHAIN_FX)
FXCONVCG := --cg --toolchain=$(TOOLCHAIN_CG)

#
#  File listings
#

NULL   :=
TARGET := $(subst $(NULL) $(NULL),-,$(NAME))

ifeq "$(TARGET_FX)" ""
TARGET_FX := $(TARGET).g1a
endif

ifeq "$(TARGET_CG)" ""
TARGET_CG := $(TARGET).g3a
endif

ELF_FX := build-fx/$(shell basename -s .g1a $(TARGET_FX)).elf
BIN_FX := $(ELF_FX:.elf=.bin)

ELF_CG := build-cg/$(shell basename -s .g3a $(TARGET_CG)).elf
BIN_CG := $(ELF_CG:.elf=.bin)

# Source files
src       := $(wildcard src/*.[csS] \
                        src/*/*.[csS] \
                        src/*/*/*.[csS] \
                        src/*/*/*/*.[csS])
assets-fx := $(wildcard assets-fx/*/*)
assets-cg := $(wildcard assets-cg/*/*)

# Object files
obj-fx  := $(src:%=build-fx/%.o) \
           $(assets-fx:assets-fx/%=build-fx/assets/%.o)
obj-cg  := $(src:%=build-cg/%.o) \
           $(assets-cg:assets-cg/%=build-cg/assets/%.o)

# Additional dependencies
deps-fx := $(ICON_FX)
deps-cg := $(ICON_CG_UNS) $(ICON_CG_SEL)

# All targets
all :=
ifneq "$(wildcard build-fx)" ""
all += all-fx
endif
ifneq "$(wildcard build-cg)" ""
all += all-cg
endif

#
#  Build rules
#

all: $(all)

all-fx: $(TARGET_FX)
all-cg: $(TARGET_CG)

$(TARGET_FX): $(obj-fx) $(deps-fx)
	@ mkdir -p $(dir $@)
	$(TOOLCHAIN_FX)-gcc -o $(ELF_FX) $(obj-fx) $(CFLAGSFX) $(LDFLAGSFX)
	$(TOOLCHAIN_FX)-objcopy -O binary $(BINFLAGS) $(ELF_FX) $(BIN_FX)
	fxg1a $(BIN_FX) -o $@ $(G1AF)

$(TARGET_CG): $(obj-cg) $(deps-cg)
	@ mkdir -p $(dir $@)
	$(TOOLCHAIN_CG)-gcc -o $(ELF_CG) $(obj-cg) $(CFLAGSCG) $(LDFLAGSCG)
	$(TOOLCHAIN_CG)-objcopy -O binary $(BINFLAGS) $(ELF_CG) $(BIN_CG)
	mkg3a $(G3AF) $(BIN_CG) $@

# C sources
build-fx/%.c.o: %.c
	@ mkdir -p $(dir $@)
	$(TOOLCHAIN_FX)-gcc -c $< -o $@ $(CFLAGSFX) $(depflags)
build-cg/%.c.o: %.c
	@ mkdir -p $(dir $@)
	$(TOOLCHAIN_CG)-gcc -c $< -o $@ $(CFLAGSCG) $(depflags)

# Assembler sources
build-fx/%.s.o: %.s
	@ mkdir -p $(dir $@)
	$(TOOLCHAIN_FX)-gcc -c $< -o $@
build-cg/%.s.o: %.s
	@ mkdir -p $(dir $@)
	$(TOOLCHAIN_CG)-gcc -c $< -o $@

# Preprocessed assembler sources
build-fx/%.S.o: %.S
	@ mkdir -p $(dir $@)
	$(TOOLCHAIN_FX)-gcc -c $< -o $@ $(INCLUDE)
build-cg/%.S.o: %.S
	@ mkdir -p $(dir $@)
	$(TOOLCHAIN_CG)-gcc -c $< -o $@ $(INCLUDE)

# Images
build-fx/assets/img/%.o: assets-fx/img/%
	@ mkdir -p $(dir $@)
	fxconv --bopti-image $< -o $@ $(FXCONVFX) name:img_$(basename $*) $(IMG.$*)
build-cg/assets/img/%.o: assets-cg/img/%
	@ mkdir -p $(dir $@)
	fxconv --bopti-image $< -o $@ $(FXCONVCG) name:img_$(basename $*) $(IMG.$*)

# Fonts
build-fx/assets/fonts/%.o: assets-fx/fonts/%
	@ mkdir -p $(dir $@)
	fxconv -f $< -o $@ $(FXCONVFX) name:font_$(basename $*) $(FONT.$*)
build-cg/assets/fonts/%.o: assets-cg/fonts/%
	@ mkdir -p $(dir $@)
	fxconv -f $< -o $@ $(FXCONVCG) name:font_$(basename $*) $(FONT.$*)

# Binaries
build-fx/assets/bin/%.o: assets-fx/bin/%
	@ mkdir -p $(dir $@)
	fxconv -b $< -o $@ $(FXCONVFX) name:bin_$(basename $*) $(BIN.$*)
build-cg/assets/bin/%.o: assets-cg/bin/%
	@ mkdir -p $(dir $@)
	fxconv -b $< -o $@ $(FXCONVCG) name:bin_$(basename $*) $(BIN.$*)

#
#  Cleaning and utilities
#

# Dependency information
-include $(shell find build* -name *.d 2> /dev/null)
build-fx/%.d: ;
build-cg/%.d: ;
.PRECIOUS: build-fx build-cg build-fx/%.d build-cg/%.d %/

clean-fx:
	@ rm -rf build-fx/
clean-cg:
	@ rm -rf build-cg/

distclean-fx: clean-fx
	@ rm -f $(TARGET_FX)
distclean-cg: clean-cg
	@ rm -f $(TARGET_CG)

clean: clean-fx clean-cg

distclean: distclean-fx distclean-cg

install-fx: $(TARGET_FX)
	p7 send -f $<
install-cg: $(TARGET_CG)
	@ while [[ ! -h /dev/Prizm1 ]]; do sleep 0.25; done
	@ while ! mount /dev/Prizm1; do sleep 0.25; done
	@ rm -f /mnt/prizm/$<
	@ cp $< /mnt/prizm
	@ umount /dev/Prizm1
	@- eject /dev/Prizm1

.PHONY: all all-fx all-cg clean distclean install-fx install-cg
