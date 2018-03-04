CONTIKI_PROJECT =app-root 

all: $(CONTIKI_PROJECT)

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"
PROJECT_SOURCEFILES += slip-bridge.c

CONTIKI = ../../..
include $(CONTIKI)/Makefile.include