# keep ARCH and DEVICE in sync when compiling for a different controller (e.g. PMS150)
ARCH = pdk14
DEVICE = PFS154

# build and output directories will be created if necessary
BUILDDIR = build
OUTPUTDIR = output
OUTPUTNAME = label_$(DEVICE)
OUTPUT = $(OUTPUTDIR)/$(OUTPUTNAME)


SOURCES = main.c
OBJECTS = $(patsubst %.c,$(BUILDDIR)/%.rel,$(SOURCES))

COMPILE = sdcc -m$(ARCH) -c --std-sdcc11 --opt-code-size -D$(DEVICE) -I. -I../../pdk-includes -I../../easy-pdk-includes
LINK = sdcc -m$(ARCH)

#symbolic targets: all, sizes, burn, clean
all: $(OUTPUT).bin
	@stat -L --printf "Size of $(OUTPUTNAME).bin: %s bytes\n" $(OUTPUT).bin

# get info on the various segment sizes in the output
sizes: all
	@egrep '(ABS,CON)|(REL,CON)' $(OUTPUT).map

#burn target requires easypdkprog to be in $PATH, otherwise you have to execute easypdkprog manually
burn: all
	easypdkprog -n $(DEVICE) write $(OUTPUT).ihx

clean:
	rm -r -f $(BUILDDIR) $(OUTPUTDIR)

$(BUILDDIR)/%.rel: %.c
	@mkdir -p $(dir $@)
	$(COMPILE) -o $@ $<

$(OUTPUT).ihx: $(OBJECTS)
	@mkdir -p $(dir $(OUTPUT))
	$(LINK) --out-fmt-ihx -o $(OUTPUT).ihx $(OBJECTS)

$(OUTPUT).bin: $(OUTPUT).ihx
	makebin -p $(OUTPUT).ihx $(OUTPUT).bin

