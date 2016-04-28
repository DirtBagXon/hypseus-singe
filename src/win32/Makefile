# sub Makefile

LOCAL_OBJS = hypseus.res

all:	${LOCAL_OBJS}

%.res: %.rc
	$(CC_MSG)
	$(SILENT)$(WINDRES) $< -O coff -o $@

clean:
	$(SILENT)rm -f ${LOCAL_OBJS}
