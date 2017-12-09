
# Logiciel de localisation

Dépend de  [libpcap-dev](http://www.tcpdump.org/).

Ce compile avec Cmake.

		mkdir buildDir
		cd buildDir
		cmake ./..
		make 
		
Le script [setMonitor](./src/setMonitor.sh) permet de changer son interface nommé **wlan1** en
mode monitoring sur le canal 11, avec comme nouveau nom **mon7**.
		
