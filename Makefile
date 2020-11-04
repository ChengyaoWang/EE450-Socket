all:
	gcc server_a.c -o server_a
	gcc server_b.c -o server_b
	gcc aws.c -o aws
	gcc client.c -o client
	gcc monitor.c -o monitor

sample:

	gcc ./sample/sample_tcp_client.c -o ./sample/sample_tcp_client
	gcc ./sample/sample_tcp_server.c -o ./sample/sample_tcp_server
	gcc ./sample/sample_udp_client.c -o ./sample/sample_udp_client
	gcc ./sample/sample_udp_server.c -o ./sample/sample_udp_server

clean:
	rm -rf *.o server_a server_b aws
	rm -rf ./sample/*.o 
	rm -rf ./sample/sample_tcp_client ./sample/sample_tcp_server 
	rm -rf ./sample/sample_udp_client ./sample/sample_udp_server