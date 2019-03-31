serial:
	gcc serial_editor.c

parallel:
	gcc parallel_editor2.c -pthread

clean:
	rm -f output_open output_parallel output_refresh output_save output_serial output_syntax

results:
	cat output_open output_refresh output_save output_syntax > output_parallel
