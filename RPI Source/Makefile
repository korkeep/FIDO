CXX:=g++

CXXFLAGS:=
LDFLAGS:= -lwiringPi -lcjson -lssl -lcrypto -lcurl

fido: fidoclient.cpp ASM.o base64.o curlwrapper.o
	$(CXX) fidoclient.cpp ASM.o base64.o curlwrapper.o -o fido $(LDFLAGS)

ASM.o: ASM.cpp
	$(CXX) -c ASM.cpp -o ASM.o $(LDFLAGS)

base64.o: base64.cpp
	$(CXX) -c base64.cpp -o base64.o $(LDFLAGS)

curlwrapper.o: curlwrapper.cpp
	$(CXX) -c curlwrapper.cpp -o curlwrapper.o $(LDFLAGS)


clean:
	rm -r *.o *.core fido