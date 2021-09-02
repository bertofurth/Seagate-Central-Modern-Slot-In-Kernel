ifeq ($(CONFIG_CNS3XXX_HIGH_PHYS_OFFSET),y)
   zreladdr-y   := 0x20008000
params_phys-y   := 0x20000100
initrd_phys-y   := 0x20C00000
else
   zreladdr-y   := 0x00008000
params_phys-y   := 0x00000100
initrd_phys-y   := 0x00C00000
endif