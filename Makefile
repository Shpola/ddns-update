ddns_update:
	sudo gcc -c ddns.c
	sudo gcc -c ini.c
	sudo gcc -o ddns_update  ddns.o ini.o -lcurl   
	sudo rm ddns.o
	sudo rm ini.o
	
	


