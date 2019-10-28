EXEC = CommandProxy
OBJS =  main_cmdProxy.o  cJSON.o s2j.o NetClient.o Log.o GlobalVar.o queue.o
SRC1  =  main_cmdProxy.c  cJSON.c s2j.c GlobalVar.c queue.c
SRC2  =  ../Common
CC = arm-linux-gnueabihf-gcc 
CFLAGS = -I.
CFLAGS += -I../Common
all:$(EXEC)

#$(EXEC):$(OBJS)
#	$(CC)  -o $@ $(OBJS) -lpthread -lm
$(EXEC):$(OBJS)
	$(CC)  -o $@ $(OBJS) -lpthread -lm
%.o:%.c
	$(CC)  -c $(CFLAGS) $< -o $@ -lpthread
%.o:$(SRC2)/%.c
	$(CC)  -c $(CFLAGS) $^ -o $@ -lpthread
clean:
	@rm -vf $(EXEC) *.o *~
