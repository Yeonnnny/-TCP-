#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include "tcp_defines.h"

int count = 0;

// 서버(Server)
// 클라이언트의 요청을 받음
// 즉, 송신된 데이터의 오류 확인 후 ACK / NAK 응답을 보내는 역할을 함

// 이진수를 출력하는 함수
void print_binary(uint32_t n, int bits) {
    for (int i = bits - 1; i >= 0; i--) {
        printf("%c", (n & (1 << i)) ? '1' : '0');
    }
}


// 데이터 검증 함수 (매개 변수 : 받은 데이터(16비트) / 반환값 : 데이터와 다항식 나눗셈의 나머지 (6비트))
uint32_t verify_received_data(uint32_t data) {

        // 1) 받은 데이터의 크기에 맞게 다항식 왼쪽으로 이동
        uint32_t poly = POLYNOMIAL << (DATA_BITS-1);
        uint32_t check_bit = 1 << (DATA_BITS + POLY_BITS -2); // 최상위 비트 확인을 위한 체크 비트

        // 2) 받은 데이터와 다항식 비교 연산
        for(int i = 0 ; i<DATA_BITS ; i++){
                // 최상위 비트가 1이면 XOR 연산 수행
                if( data & check_bit) data^=poly;
                poly >>= 1; // 다항식 오른쪽으로 이동
                check_bit >>= 1; // 데이터의 다음 비트를 검사하기 위해 마스크 이동
        }
	// 3) 나머지 반환 (0이 아닌 경우 오류)
        return data;
}



int main (void)
{
	int server_socket;
	int client_socket;
	socklen_t client_addr_size;
	struct sockaddr_in server_addr; // 서버 주소 : 송신 주소
	struct sockaddr_in client_addr; // 클라이언트 주소 : 수신 주소

 	// 서버 소켓 생성
	server_socket = socket(PF_INET, SOCK_STREAM, 0);
	if(server_socket == -1){
		printf("server socket 생성 실패\n ");
		exit(1);
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(5000); // 포트 설정
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	// 소켓에 주소 할당
	if(bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1){
		printf("bind() 실행 실패\n");
		close(server_socket);
		exit(1);
	}

	// 연결 요청
	if(listen(server_socket, 5) == -1){
		printf("listen() 실행 실패\n");
		close(server_socket);
		exit(1);
	}


	while(1){
		// 연결 수락 여부 확인
		client_addr_size = sizeof(client_addr);
		client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_size);
		if(client_socket == -1){
			printf("클라이언트 연결 수락 실패\n");
			continue;
		}

		// 클라이언트 연결 성공 시
		for(int i = 0; i < NUM_DATA; i++){
			uint32_t received_data = 0;
			ssize_t bytes_received = read(client_socket, &received_data, sizeof(received_data)); 

			// 요청온 데이터 읽기
			if(bytes_received == -1){
				printf("데이터 수신 실패\n");
				close(client_socket);
				break;
			}else if(bytes_received != sizeof(received_data)){
				printf("데이터 수신 크기 오류. 받은 바이트 수 : %zd\n", bytes_received);
				close(client_socket);
				break;
			}


			// 네트워크 바이트 순서 -> 호스트 바이트 순서 변환
			received_data = ntohl(received_data);

	                printf("[%d 번 데이터] 받은 데이터: ", i);
        	        print_binary(received_data, 16);
                	printf("\n");

			char response[BUFF_SIZE] = {0}; // 버퍼 초기화

			// 데이터 검증
			uint32_t result = verify_received_data(received_data);

                	printf("검증 결과: ");
                	print_binary(result, 6);
                	printf("\n");


			if(result == 0){
				printf("데이터 검증 성공 (오류X)\n");
				strcpy(response, "ACK");
			}else{
				printf("데이터 검증 실패 (오류O)\n");
				strcpy(response, "NAK");
			}

			// 클라이언트 응답 전송
			if(write(client_socket, response, strlen(response)+1) == -1){
				printf("응답 전송 실패");
				break;
			}

			printf("\n");
		} // end for(;NUM_DATA;)

		close(client_socket); // 클라이언트 소켓 종료

	} // end while(1)

	close(server_socket);
	return 0 ;

} // end main


