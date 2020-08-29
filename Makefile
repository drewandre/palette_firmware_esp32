#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

NAME = palette_firmware
VERSION := $(shell cat $(PWD)/version.txt)
PROJECT_NAME := $(NAME)_$(VERSION)
$(info Set binary name to $(PROJECT_NAME))

include $(IDF_PATH)/make/project.mk
# include $(ADF_PATH)/project.mk