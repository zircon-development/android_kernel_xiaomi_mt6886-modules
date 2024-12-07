extra_symbols := $(abspath $(O)/../mt6886-modules/connectivity/conninfra/Module.symvers)
extra_symbols += $(abspath $(O)/../mt6886-modules/connectivity/common/Module.symvers)

$(info [wlan] WLAN_BUILD_COMMON=$(WLAN_BUILD_COMMON))

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) modules $(KBUILD_OPTIONS) KBUILD_EXTRA_SYMBOLS="$(extra_symbols)"

modules_install:
	$(MAKE) M=$(M) -C $(KERNEL_SRC) modules_install

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) clean
