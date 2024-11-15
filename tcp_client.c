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


// 클라이언트(Client)
// 서버에 요청을 보냄
// 즉, 다항코드로 만들어진 데이터를 송신함


// 이진수를 출력하는 함수
void print_binary(uint32_t n, int bits) {
    for (int i = bits - 1; i >= 0; i--) {
        printf("%c", (n & (1 << i)) ? '1' : '0');
    }
}


// 체크섬 생성 함수 (매개 변수 : 전송할 데이터 (10비트) / 반환 값 : checksum (6비트))
uint32_t calculate_checksum(uint32_t data){
	// 1) 데이터 크기에 맞게 다항식을 왼쪽으로 이동시킴
	uint32_t poly = POLYNOMIAL << (DATA_BITS-1);
	uint32_t check_bit = 1 << (DATA_BITS + POLY_BITS -2); // 최상위 비트 확인을 위한 비트

	// 2) 전송 데이터를 다항식의 최고 차수만큼 왼쪽으로 이동시켜 나눗셈 연산 준비
	data <<= (POLY_BITS-1);

	// 3) 전송데이터와 다항식 비교 (나눗셈 연산)
	for(int i = 0; i < DATA_BITS ; i++){
		// 최상위 비트가 1이면 XOR 연산
		if(data & check_bit) data ^= poly;
		poly >>= 1; // 다항식을 오른쪽으로 한 비트 이동
		check_bit >>= 1; // 데이터의 다음 비트를 검사하기 위해 체크비트 이동
	}
	// 4) 나머지 반환 (체크섬)
	return data;
}

// 비트 변경하는 함수
uint32_t change_bit (uint32_t data, int position){
	return data^(1<<position); // position 위치의 비트를 XOR 연산으로 반전시킴
}


int main (void)
{
	int client_socket;		// 클라이언트 소켓
	struct sockaddr_in server_addr;	// 통신할 서버
	char buff[BUFF_SIZE]; 		// 응답 버퍼

	// 소켓 생성
	client_socket = socket(PF_INET, SOCK_STREAM, 0);
	if(client_socket == -1){
		printf("client socket 생성 실패\n");
		exit(1);
	}

	// 서버 주소 설정
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(5000);
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	// 서버에 연결
	if(connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1){
		printf("접속 실패\n");
		close(client_socket);
		exit(1);
	}


	// 서버 연결 성공


	// 랜덤 데이터 생성 및 전송
	srand(time(NULL));
	for(int i = 0; i < NUM_DATA ; i++){

		printf("[%d 번 데이터]\n",i);

		// 1) 10비트 랜덤 데이터 생성 (0-1023(2^10-1) 범위)
		uint32_t data = rand() & 0x3FF;
        	printf("전송 데이터: ");
        	print_binary(data, DATA_BITS);
        	printf(" | ");

		// 2) 체크섬 생성
		uint32_t checksum = calculate_checksum(data);
       		printf("체크섬: ");
        	print_binary(checksum, POLY_BITS-1);
        	printf(" -> ");

		// 3) 송신 데이터 준비 (데이터 + 체크섬)
		uint32_t transmission_data = (data<<(POLY_BITS-1)) | checksum;

		// * 특정 데이터에 대한 비트 변경 함 (데이터 검증 에러 유발) *
		if(i%3==0){ // 3의 배수 번째인 경우 비트 임의로 변경
			transmission_data = change_bit(transmission_data, i); // 송신 데이터의 특정 비트 변경
		}

        	print_binary(transmission_data, DATA_BITS + POLY_BITS-1);
        	printf("\n");

		// 4) 데이터 전송
		transmission_data = htonl(transmission_data); // 호스트 순서-> 네트워크 바이트 순서로 변경
		if(write(client_socket, &transmission_data, sizeof(transmission_data))==-1){
			printf("데이터 전송 실패 \n");
			close(client_socket);
			exit(1);
		}

		// 5) 서버 응답 수신
		memset(buff, 0, sizeof(buff));
		if(read(client_socket, buff, BUFF_SIZE-1) == -1){
			printf("서버 응답 수신 실패");
			close(client_socket);
			exit(1);
		}

		// 6) 서버 응답 출력
		printf(">> 서버 응답 : %s\n\n", buff);

	} // end for(;NUM_DATA;)

	// 소켓 닫기
	close(client_socket);
	return 0;

} // end main

