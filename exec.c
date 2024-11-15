#include <stdio.h>
#include <stdint.h>

#define POLYNOMIAL 0b1010101	// 생성 다항식 G(x) = x^6+x^4+x^2+1
#define POLY_BITS 7	// 다항식 비트 수
#define DATA_BITS 10	// 전송 데이터 비트 수


// 이진수 출력 함수
void print_bin(uint32_t n, int bits){
	for (int i = bits-1; i>=0; i--){
		printf("%c", (n&(1<<i)) ?'1':'0');
	}
}


// 체크섬 생성 함수
uint32_t calculate_checksum(uint32_t data){
	// 데이터 크기에 맞게 다항식을 왼쪽으로 이동시킴
	uint32_t poly = POLYNOMIAL << (DATA_BITS -1);
	uint32_t check_bit = 1 << (DATA_BITS + POLY_BITS -2); // 최상위 비트 확인을 위한 마스크

	// 전송 데이터를 왼쪽으로 다항식 최고 차수만큼 이동시켜 나눗셈 연산 준비
	data <<= (POLY_BITS-1);

	// 다항식 나눗셈 수행
	for(int i = 0; i<DATA_BITS; i++){
		// 최상위 비트가 1이면 XOR 연산 수행
		if(data & check_bit){
			data ^= poly;
		}
		// 다항식을 오른쪽으로 한 비트 이동
		poly >>= 1;
		check_bit >>= 1; // 데이터의 다음 비트를 검사하기 위해 마스크 이동
	}

	// 나머지 반환 (체크섬)
	return data;
}


// 데이터 검증 함수
uint32_t verify_received_data(uint32_t data) {
	// 1) 받은 데이터 크기 확인
	if( (data<<(DATA_BITS + POLY_BITS -1)) != 0 ){ // 데이터가 16비트가 아님
		printf("데이터의 길이가 올바르지 않음\n");
		exit(1);
	}

	// 2) 받은 데이터의 크기에 맞게 다항식 왼쪽으로 이동
	uint32_t poly = POLYNOMIAL << (DATA_BITS-1);
	uint32_t check_bit = 1<<(DATA_BITS + POLY_BITS -2); // 최상위 비트 확인을 위한 비트

	// 3) 받은 데이터와 다항식 비교 연산
	for(int i = 0 ; i<DATA_BITS ; i++){
		// 최상위 비트가 1이면 XOR 연산 수행
		if( data & check_bit) data^=poly;
		poly >>= 1; // 다항식 오른쪽으로 이동
		check_bit >>= 1; // 데이터의 다음 비트를 검사하기 위해 마스크 이동
	}
	return data; // 나머지 반환 (0이 아닌 경우 오류)
}
