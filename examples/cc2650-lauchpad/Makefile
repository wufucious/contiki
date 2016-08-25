DEFINES+=PROJECT_CONF_H=\"project-conf.h\"
CONTIKI_PROJECT = hello-world ccn-demo-mote1 ccn-demo-mote2

all: $(CONTIKI_PROJECT)

CONTIKI_WITH_IPV6 = 1
CONTIKI_WITH_RIME = 1

V=1
SMALL =0

APPS += ccn-lite

CONTIKI = ../..
include $(CONTIKI)/Makefile.include
