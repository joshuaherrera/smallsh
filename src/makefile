#NAME = herrjosh
EXEC = smallsh

sh: $(EXEC).c 
	gcc -o $(EXEC) $(EXEC).c

noisySh: $(EXEC).c 
	gcc -Wall -Wextra -g -o $(EXEC) $(EXEC).c

runSh:
	./$(EXEC)

ShMemCheck:
	valgrind ./$(EXEC)  

clean:
	rm -f $(EXEC) *.o *~