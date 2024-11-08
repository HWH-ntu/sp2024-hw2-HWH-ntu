all: friend
friend: friend.c hw2.h
	gcc friend.c -o friend
	// compile your friend
clean:
	rm -v friend
	// remove any executable or unnecessary file according to submit format
