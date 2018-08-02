all:
	g++ src/bme280.cpp -I src/include -c -o src/bme280.o
	g++ src/measure.cpp src/bme280.o -I src/include -o measure

clean:
	rm -f src/*.o
	rm -f measure
