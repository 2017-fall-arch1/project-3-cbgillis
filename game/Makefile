
CPU 			= msp430g2553
CFLAGS 			= -mmcu=${CPU} -Os -I../h
LDFLAGS 		= -L../lib -L/opt/ti/msp430_gcc/include/

CC 			= msp430-elf-gcc
AS 			= msp430-elf-gcc -mmcu=${CPU} -c

all:game.elf

game.elf: ${COMMON_OBJECTS} pong.o wdt_handler.o scoreSet.o buzzer.o
		${CC} ${CFLAGS} ${LDFLAGS} -o $@ $^ -lTimer -lLcd -lShape -lCircle -lp2sw

load: game.elf
	mspdebug rf2500 "prog $^"

clean: rm -f *.o *.elf *.c~ *~ *.s~
