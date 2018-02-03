
CONTIKI_PROJECT =radio-sender radio-receiver 

all: $(CONTIKI_PROJECT)
CONTIKI_CONF_WITH_ORPL_BITMAP = 1
	
APPS += udp-fragment
APPS += netsynch task-schedule correct-time node-function

CONTIKI = ../../..
include $(CONTIKI)/Makefile.include
