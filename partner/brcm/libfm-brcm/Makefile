

FmDeamon:FmDaemon.cpp
	g++ -DFM_UDP_SERVER  $< -o $@


test:FmDaemon.cpp
	g++ $< -o $@

.PHONY:clean
clean:
	-@rm FmDeamon test
