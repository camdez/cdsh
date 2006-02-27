OUTFILE = cdsh

$(OUTFILE) : cdsh.c
	$(CC) cdsh.c -o $(OUTFILE)

clean:
	$(RM) *.o *~ $(OUTFILE)

.PHONY : clean
