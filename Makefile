all: clean make_mibib

make_mibib:
	@$(CC) $(CFLAGS) -Wall make_mibib.c -o make_mibib
	@chmod +x make_mibib

clean:
	@rm -rf make_mibib
