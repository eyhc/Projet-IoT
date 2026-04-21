# name of your application
APPLICATION = lora-disaster-chat

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= RIOT
#RIOTBASE ?= ${HOME}/github/RIOT-OS/RIOT

# Path to my boards
EXTERNAL_BOARD_DIRS ?= RIOT-wyres/boards
# If no BOARD is found in the environment, use this default:
BOARD ?= wyres-base

# include $(RIOTBASE)/tests/Makefile.tests_common

FEATURES_REQUIRED += periph_eeprom

USEPKG += cayenne-lpp

USEMODULE += od
USEMODULE += ps
USEMODULE += shell
USEMODULE += shell_cmds_default
#USEMODULE += shell_commands
USEMODULE += fmt
USEMODULE += ztimer_sec
USEMODULE += ztimer
USEMODULE += ztimer_msec
USEMODULE += xtimer
USEMODULE += lis2dh12_i2c
USEMODULE += lps22hb

DRIVER ?= sx1272
# use SX1276 by default
USEMODULE += $(DRIVER)

# Fix the problem of the size of the debugging files
CFLAGS_DBG = 

CFLAGS += -DISR_STACKSIZE=1024U
#CFLAGS += -DSX127X_STACKSIZE=1024U
CFLAGS += -DTHREAD_STACKSIZE_DEFAULT=4096U

include $(RIOTBASE)/Makefile.include
