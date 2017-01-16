CFLAGS = -Wall -std=c99
USERINCLUDES = user.h 
USEROBJECTS = user.o 
LSINCLUDES = topics.h
LSOBJECTS = ls.o topics.o
SSINCLUDES = 
SSOBJECTS = ss.o


all: $(USERINCLUDES) $(USEROBJECTS) $(LSINCLUDES) $(LSOBJECTS) $(SSINCLUDES) $(SSOBJECTS)
	gcc $(CFLAGS) -o user $(USEROBJECTS)
	gcc $(CFLAGS) -o ls $(LSOBJECTS)
	gcc $(CFLAGS) -o ss $(SSOBJECTS)

debug: $(USERINCLUDES) $(USEROBJECTS) $(LSINCLUDES) $(LSOBJECTS) $(SSINCLUDES) $(SSOBJECTS)
	gcc $(CFLAGS) -g -o user $(USEROBJECTS)
	gcc $(CFLAGS) -g -o ls $(LSOBJECTS)
	gcc $(CFLAGS) -g -o ss $(SSOBJECTS)

user: $(USERINCLUDES) $(USEROBJECTS)
	gcc $(CFLAGS) -o user $(USEROBJECTS)

ls: $(LSINCLUDES) $(LSOBJECTS)
	gcc $(CFLAGS) -o ls $(LSOBJECTS)

ss: $(SSINCLUDES) $(SSOBJECTS)
	gcc $(CFLAGS) -o ss $(SSOBJECTS)

clean: 	
	rm *.o
	rm user
	rm ls
	rm ss 
