# Toolchain configuration
CC := clang
AS := $(CC)
LD := $(CC)

# User parameters
TARGET ?= lvgl_fb
SILENT ?= 1
LINKER_FILE ?=
MAK_FILE ?= drivers/lvgl/lvgl.mk

# Silent mode handling
ifeq ($(SILENT), 1)
    AT := @
else
    AT :=
endif

# Build directories
BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
DEP_DIR := $(BUILD_DIR)/dep

# Source directories and files
SRC_DIRS += src drivers/fb_display
SRC_FILES +=

AS_DIRS +=
AS_FILES +=

INCLUDE_DIRS += include drivers/fb_display drivers/lvgl/demos

USER_DEFINES += LV_CONF_INCLUDE_SIMPLE LV_REFRESH_TIME=10

TARGET_PLATFORM := arm-linux-gnueabihf

# Compiler flags
OPTIMIZE_LEVEL := -O0
CFLAGS := --target=$(TARGET_PLATFORM) --static -g -Wall -Wundef $(OPTIMIZE_LEVEL) $(addprefix -D,$(USER_DEFINES))
ASFLAGS := --target=$(TARGET_PLATFORM) $(addprefix -D,$(USER_DEFINES))

# Linker flags
ifeq ($(CC),$(LD))
# if we use CC as linker, parameter may be some different
LDFLAGS := --target=$(TARGET_PLATFORM) --static -g $(OPTIMIZE_LEVEL) -Wl,-Map=$(TARGET).map
ifneq ($(wildcard $(LINKER_FILE)),)
    LDFLAGS += -Wl,-T,$(LINKER_FILE)
else
    $(warning Linker file not found: $(LINKER_FILE))
endif

else
LDFLAGS := -g $(OPTIMIZE_LEVEL) -Map=$(TARGET).map
ifneq ($(wildcard $(LINKER_FILE)),)
    LDFLAGS += -T $(LINKER_FILE)
else
    $(warning Linker file not found: $(LINKER_FILE))
endif

endif # Linker Same as CC

# File collection
C_SOURCES := $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.c))
ALL_C_SRCS := $(SRC_FILES) $(C_SOURCES)

AS_SOURCES := $(foreach dir,$(AS_DIRS),$(wildcard $(dir)/*.s) $(wildcard $(dir)/*.S))
ALL_AS_SRCS := $(AS_SOURCES) $(AS_FILES)

####################################################################
# For Lvgl sources
LVGL_PATH := drivers/lvgl
####################################################################

# Include additional makefile
ifneq ($(MAK_FILE),)
    include $(MAK_FILE)
else
    $(warning No additional makefile defined)
endif

####################################################################
# For Lvgl sources
ALL_C_SRCS += $(CSRCS)
ALL_AS_SRCS += $(ASRCS)

ASFLAGS += $(AFLAGS)
####################################################################

# Object files with directory structure preserved
C_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(ALL_C_SRCS))
ASM_OBJS := $(patsubst %.s,$(OBJ_DIR)/%.o,$(filter %.s,$(ALL_AS_SRCS)))
ASM_OBJS += $(patsubst %.S,$(OBJ_DIR)/%.o,$(filter %.S,$(ALL_AS_SRCS)))

OBJS := $(C_OBJS) $(ASM_OBJS)

# Dependency files
C_DEP_FILES := $(patsubst %.c,$(DEP_DIR)/%.d,$(ALL_C_SRCS))
AS_DEP_FILES := $(patsubst %.s,$(DEP_DIR)/%.d,$(filter %.s,$(ALL_AS_SRCS)))
AS_DEP_FILES += $(patsubst %.S,$(DEP_DIR)/%.d,$(filter %.S,$(ALL_AS_SRCS)))
DEP_FILES := $(C_DEP_FILES) $(AS_DEP_FILES)

# Create directory list
DIRECTORIES := $(sort $(BUILD_DIR) $(OBJ_DIR) $(DEP_DIR) \
               $(dir $(OBJS)) $(dir $(DEP_FILES)))

# Target definition
TARGET_NAME := $(TARGET).elf

# Main targets
.PHONY: all clean distclean DEBUG_SHOW

all: $(TARGET_NAME)
	@echo
	@echo "###################################################"
	@echo "Compile Done! Target: $(TARGET_NAME)"
	@echo "###################################################"

# Enhanced DEBUG_SHOW target
DEBUG_SHOW:
	@echo "======================== BUILD CONFIGURATION ================================="
	@echo "TARGET: $(TARGET)"
	@echo "SILENT MODE: $(SILENT)"
	@echo "LINKER FILE: $(LINKER_FILE)"
	@echo "ADDITIONAL MAKEFILE: $(MAK_FILE)"
	@echo "DEBUG LEVEL: $(OPTIMIZE_LEVEL)"
	@echo
	@echo "COMPILER: $(CC)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo
	@echo "ASSEMBLER: $(AS)"
	@echo "ASFLAGS: $(ASFLAGS)"
	@echo
	@echo "LINKER: $(LD)"
	@echo "LDFLAGS: $(LDFLAGS)"
	@echo
	@echo "SOURCE DIRS: $(SRC_DIRS)"
	@echo "C SOURCES: $(words $(ALL_C_SRCS)) files"
	@echo "ASM SOURCES: $(words $(ALL_AS_SRCS)) files"
	@echo "INCLUDE DIRS: $(INCLUDE_DIRS)"
	@echo "USER DEFINES: $(USER_DEFINES)"
	@echo "======================================================================="

# Linking
$(TARGET_NAME): $(OBJS) | $(DIRECTORIES) DEBUG_SHOW
	@echo "Linking $@..."
	$(AT)$(LD) $(LDFLAGS) $(OBJS) -o $@

# C compilation with dependency generation
$(OBJ_DIR)/%.o: %.c | $(DIRECTORIES)
	@echo "Compiling $<..."
	@mkdir -p $(dir $@) $(dir $(DEP_DIR)/$*.d)
	$(AT)$(CC) $(CFLAGS) $(addprefix -I,$(INCLUDE_DIRS)) \
		-MMD -MF $(DEP_DIR)/$*.d -MT $@ -c $< -o $@

# Assembly compilation
$(OBJ_DIR)/%.o: %.s | $(DIRECTORIES)
	@echo "Assembling $<..."
	@mkdir -p $(dir $@)
	$(AT)$(AS) $(ASFLAGS) $(addprefix -I,$(INCLUDE_DIRS)) -c $< -o $@

$(OBJ_DIR)/%.o: %.S | $(DIRECTORIES)
	@echo "Assembling $<..."
	@mkdir -p $(dir $@)
	$(AT)$(AS) $(ASFLAGS) $(addprefix -I,$(INCLUDE_DIRS)) -c $< -o $@

# Directory creation
$(DIRECTORIES):
	$(AT)mkdir -p $@

# Include dependencies
ifneq ($(DEP_FILES),)
-include $(DEP_FILES)
endif

# Clean targets
clean:
	@echo "Cleaning build artifacts..."
	$(AT)rm -rf $(BUILD_DIR)
	@echo "Clean Done!"

distclean: clean
	@echo "Cleaning distribution artifacts..."
	$(AT)find . -name "*.elf" -type f -delete 2>/dev/null
	$(AT)find . -name "*.map" -type f -delete 2>/dev/null
	@echo "Dist Clean Done!"
