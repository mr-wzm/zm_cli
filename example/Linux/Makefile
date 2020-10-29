TARGET = zm_cli
#CC = gcc
CFLAGS = -Wall -g

DIRS = ../../source

INC = -I ../../source

OBJS = main.o 
OBJS += zm_cli.o 
OBJS += zm_printf.o


$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(INC) -Tmain.lds
	$(shell if [ ! -d build ];then \
   	mkdir -p build; \
	fi;)
	#mv *.o build
	mv $(TARGET) build
	@echo Create $(TARGET) OK!
main.o : main.c ${DIRS}/zm_cli.h
	$(CC) $(CFLAGS) -c $< -o $@ $(INC)
zm_cli.o : ${DIRS}/zm_cli.c ${DIRS}/zm_config.h ${DIRS}/zm_section.h ${DIRS}/zm_common.h ${DIRS}/zm_cli.h
	$(CC) $(CFLAGS) -c $< -o $@ -Tmain.lds
zm_printf.o : ${DIRS}/zm_printf.c ${DIRS}/zm_printf.h ${DIRS}/zm_config.h ${DIRS}/zm_common.h
	$(CC) $(CFLAGS) -c $< -o $@

clean: 
	rm *.o
	rm $(TARGET)
	rm -rf build
