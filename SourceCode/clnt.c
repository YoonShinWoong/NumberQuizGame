#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

// 매크로 상수
#define BUF_SIZE 1024
#define SOLO 1
#define COMP 2
#define USER_MAX 4

// 정보 구조체
typedef struct{
    int mode; // 모드
    int stage; // 단계
    char name[20]; // 닉네임
    int rank;
    int sock;
    int prob[5]; // 문제
}clnt_info;

int finish=0;
clnt_info cinfo;
void error_handling(char *message);
int main(int argc, char *argv[])
{
	int sock;
	char buf[BUF_SIZE];        
    char buf2[BUF_SIZE];
    
	int str_len;
	struct sockaddr_in serv_adr;

	if(argc!=4) {
		printf("Usage : %s <IP> <port> <NickName>\n", argv[0]);
		exit(1);
	}
	
    // SOCKET
	sock=socket(PF_INET, SOCK_STREAM, 0);   
	if(sock==-1)
		error_handling("socket() error");
	
    // CONNECT
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family=AF_INET;
	serv_adr.sin_addr.s_addr=inet_addr(argv[1]);
	serv_adr.sin_port=htons(atoi(argv[2]));
	
	if(connect(sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr))==-1)
		error_handling("connect() error!");
	else
		puts("Game Loading...........");
	
    
    // 모드 설정
    printf("-----------------UP & DOWN GAME----------------\n"); 
    printf("-----------------------------------------------\n");
    printf("WAHT MODE DO YOU WANT?(1: SOLO, 2: COMPETITION)\n");
    printf("-----------------------------------------------\n");
    printf("IF U NEED GUIDE ABOUT GAME,  3: GUID ABOUT GAME\n");
    printf("-----------------------------------------------\n");

    // 모드 입력
    while(1){
        printf("INSERT MODE(1:SOLO, 2:COMP, 3:GUIDE) : ");
        scanf("%d", &(cinfo.mode)); 
        fgetc(stdin);
        if(cinfo.mode == 3){
            printf("-----------------------------------------------------------------\n");
            printf("  This game is to match 5 random numbers 1 ~ 100    \n");
            printf("  If you submit smaller number than Answer Serv Reply \"UP\"\n");
            printf("  If you submit bigger number than Answer Serv Reply \"DOWN\"\n");
            printf("  And if you submit correct answer, then go to Next stage\n");
            printf("  In this Game, there are 5 stages\n");
            printf("  If you solve 5 stages, you win\n");
            printf("  In Competition mode, You must solve speedly than others\n");
            printf("  In Solo mode, you exercise However much\n");
            printf("-----------------------------------------------------------------\n");

            continue;
        }
        // 입력 예외처리
        else if(cinfo.mode !=SOLO && cinfo.mode != COMP){
            printf("ENTER 1~3\n");
            continue;
        }
        
        // 전송
        strcpy(cinfo.name, argv[3]);
        write(sock, &cinfo, sizeof(cinfo)); // 구조체 전송
        break;
    }
    
    // 대기 처리
    read(sock, buf, BUF_SIZE); // 대기 msg 읽기
    printf("%s", buf); 
    
    // 시작을 할 수 없다면
    if(!strcmp(buf, "Server Already Start GAME!\nYou Can't Start GAME!\n")){
        close(sock);
        return 0;
    }
    
    // 경쟁 모드일 경우 대기
    else if(cinfo.mode == COMP ){
        int people;
        // case 1. 방장
        if(!strcmp(buf, "\nYou are a ROOM MANAGER!\nHow many People do you Want?\nEnter(2~4): ")){
            scanf("%d", &people);
            fgetc(stdin); // \n
            fflush(stdin);
            
            while(people<2 || people >4){
                printf("Enter(2~4): ");
                scanf("%d", &people);
                fgetc(stdin);
                fflush(stdin);
            }
            
            buf[0] = (char)people;
            write(sock, buf, 1);
            
            read(sock, buf, BUF_SIZE);
            printf("%s",buf); // 대기
            
            read(sock, buf, BUF_SIZE);// 시작 대기
            printf("%s",buf);
        }
        // case 2. 대기의 경우
        else{
            read(sock, buf, BUF_SIZE);// 시작 대기
            printf("%s",buf);
        }
    }
    
    // 처리
	while(cinfo.stage != 5 && finish ==0) 
	{
        int solv;
        while(cinfo.stage != 5 && finish ==0){
            sprintf(buf, "<STAGE #%d> Input Number(# 1~100, 0 to quit): ", cinfo.stage+1);
            fputs(buf,stdout);
            //fgets(buf, BUF_SIZE, stdin);
            scanf("%d", &solv); 
            fgetc(stdin);
            fflush(stdin);

            // 숫자 입력 
            if(0<= solv && solv <=100)
                break;
            
            else
                printf("ENTER 1~100\n\n");
        }
        
        // 숫자 바꿔서 보내기
        buf[0]=(char)solv;
        
		if( solv == 0)
			break;

		write(sock, buf, strlen(buf));
		str_len=read(sock, buf, BUF_SIZE-1);
		buf[str_len]=0;
		printf("%s", buf);
        
        // stage check
        sprintf(buf2, "<ANSWER> %d Go Next Stage\n\n", solv);    
        if(!strcmp(buf2,buf))
            cinfo.stage++;
        
        if(cinfo.stage == 5){
            printf("<CLEAR> THANK YOU!\n");
            sprintf(buf, "finish");
            write(sock, buf, strlen(buf));
            break;
        }
    }                  
        
    // 경쟁 모드일 경우 기록 출력
    if(cinfo.mode == COMP && cinfo.stage == 5){
        str_len = read(sock, buf, BUF_SIZE-1);
        buf[str_len]=0;
        printf("%s",buf);
    }
    
	close(sock);
	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}