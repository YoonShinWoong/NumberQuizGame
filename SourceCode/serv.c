#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <time.h>

// 매크로상수
#define BUF_SIZE 1024
#define SOLO 1
#define COMP 2
#define USER_MAX 4
#define GAMING 1
#define WAITING 0

// 정보 구조체
typedef struct{
    int mode; // 모드
    int stage; // 단계
    char name[20]; // 닉네임
    int rank; // 순위
    int sock; // 소켓
    int prob[5]; // 문제
}clnt_info;

clnt_info cinfo[BUF_SIZE];
int clnt_cnt=0, user_cnt; // 서버 현재 인원
int serv_state=0; // 서버 상태
int serv_mode=0; // 서버 동작중인 모드
int clnt_max=0; // 서버 최대 인원
int serv_rank=1; // 랭킹 기록
char rankName[5][20]; // 랭킹 네임

void error_handling(char *buf);
void makeProb(int * prob);
int main(int argc, char *argv[])
{
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	struct timeval timeout;
	fd_set reads, cpy_reads;

	socklen_t adr_sz;
	int fd_max, str_len, fd_num, i;
	char buf[BUF_SIZE];
    
    // ARGC ERROR
	if(argc!=2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}

    // SOCKET
	serv_sock=socket(PF_INET, SOCK_STREAM, 0);
    
    // BIND
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family=AF_INET;
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_adr.sin_port=htons(atoi(argv[1]));
	if(bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr))==-1)
		error_handling("bind() error");
    
    // LISTEN
	if(listen(serv_sock, 5)==-1)
		error_handling("listen() error");

    // FD 초기화
	FD_ZERO(&reads);
	FD_SET(serv_sock, &reads);
	fd_max=serv_sock;

    
    // 입출력 처리
	while(1)
	{
		cpy_reads=reads;
		timeout.tv_sec=5;
		timeout.tv_usec=5000;

		if((fd_num=select(fd_max+1, &cpy_reads, 0, 0, &timeout))==-1)
			break;
		
		if(fd_num==0)
			continue;

		for(i=0; i<fd_max+1; i++)
		{
			if(FD_ISSET(i, &cpy_reads))
			{
                // 추가 등록 
                // ** 게임 실행중이 아닐 때만 추가가능
				if(i==serv_sock)     // connection request!
				{
                    // 대기 모드 일때 
                    if(serv_state == WAITING){
                        adr_sz=sizeof(clnt_adr);
                        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);
                        FD_SET(clnt_sock, &reads);
                        if(fd_max<clnt_sock)
                            fd_max=clnt_sock;
                        printf("New User connectd...!\n");
                        cinfo[clnt_cnt].sock =clnt_sock; // 소켓정보 기록
                        clnt_cnt++; // clnt 증가
				    }
                    // 그외 거부
                    else{
                        ;
                    }
                }
                
                // 처리
                // 게임 실행중일 때는 게임 돌리기
				else    
				{
                    int j;
                    // 해당 유저 정보 위치 inx 찾기
                    for(j=0;j<clnt_cnt;j++)
                        if(cinfo[j].sock == i)
                            break;
                    
                    // 만약 처음 읽는 거라면 정보 읽기  
                    if(!strcmp(cinfo[j].name,"")){
                        clnt_info tempinfo;
                        read(i,&tempinfo, sizeof(tempinfo));

                        // 값 옮기기
                        strcpy(cinfo[j].name, tempinfo.name);
                        cinfo[j].mode = tempinfo.mode;
                        cinfo[j].stage = 0;
                        makeProb(cinfo[j].prob); // 문제만들기
// 서버 정답보기                        printf("%d %d %d %d %d \n", cinfo[j].prob[0],cinfo[j].prob[1], cinfo[j].prob[2], cinfo[j].prob[3], cinfo[j].prob[4] );
                        printf("New User %s has joined\n", cinfo[j].name);
                        user_cnt++;
                        
                        // 게임 모드 체크하기(1이면 바로 시작, 2로 들어오면 기존 것들 다 2인지 체크)
                        // 모드 기록
                        if(serv_mode == 0 ){
                            serv_mode = cinfo[j].mode; // 모드 기록
                            //printf("%d\n",serv_mode);
                            
                            if(cinfo[j].mode == SOLO){ // 바로 시작
                                sprintf(buf,"\n-----------------START GAME-----------------\n");
                                write(cinfo[j].sock, buf, sizeof(buf));
                            }
                            
                            // 경쟁모드 일 경우
                            if(cinfo[j].mode == COMP){
                                // 인원 수 기록받기
                                sprintf(buf,"\nYou are a ROOM MANAGER!\nHow many People do you Want?\nEnter(2~4): ");
                                write(cinfo[j].sock, buf, sizeof(buf));
                                read(cinfo[j].sock, buf, 1);
                                clnt_max = (int)buf[0];
                                sprintf(buf, "GAME Room has been created by %d people!\n", clnt_max);
                                printf("%s",buf);
                                sprintf(buf, "%s\nWAIT A MOMENT!\nNOW: %d/%d\n\n", buf,user_cnt,clnt_max);
                                write(cinfo[j].sock, buf, sizeof(buf));
                            }
                        }
                        
                        // 모드 체크
                        else{
                            // 솔로모드는 언제든 시작
                            if(serv_mode == SOLO && cinfo[j].mode == SOLO){ // 바로 시작
                                sprintf(buf,"\n-----------------START GAME-----------------\n");
                                write(cinfo[j].sock, buf, sizeof(buf));
                            }
                        
                            
                            // 현재 서버와 모드가 다른 경우 // 경쟁 모드이면 사람 수 넘은 경우
                            else if(serv_mode != cinfo[j].mode || user_cnt > clnt_max || (serv_mode == COMP && cinfo[j].mode == SOLO) ){
                                strcpy(buf, "Server Already Start GAME!\nYou Can't Start GAME!\n");
                                write(cinfo[j].sock, buf, BUF_SIZE); // 출력  
                                FD_CLR(cinfo[j].sock, &reads);
                                close(cinfo[j].sock);
                                printf("User %s has finished\n", cinfo[j].name); // 종료
                               
                                // cinfo 배열에서 해당 값 뺴기
                                int k;
                                for(k=0;k<clnt_cnt;k++)
                                    if(cinfo[k].sock == cinfo[j].sock)
                                        break;
                                while(k <clnt_cnt){
                                    cinfo[k] = cinfo[k+1];
                                    k++;
                                }
                                clnt_cnt--; // 개수 하나 줄이기
                                user_cnt--;
                            }
                            
                            // 경쟁 대기 중일경우
                            else if(serv_mode == COMP && cinfo[j].mode == COMP && user_cnt <= clnt_max){
                                printf("ROOM STATUS %d/%d!\n", user_cnt, clnt_max);
                                sprintf(buf, "WAIT A MOMENT!\nNOW: %d/%d\n\n", user_cnt, clnt_max);
                                write(cinfo[j].sock, buf, sizeof(buf));
                            }       
                            // 시작 할 수 있을경우
                            if(serv_mode == COMP && cinfo[j].mode == COMP && user_cnt == clnt_max){
                                sprintf(buf,"\n-------------------GET READY-------------------\n-------------------START GAME-------------------\n\n");
                                printf("%s\n",buf);
                                for(int cnt=0;cnt<clnt_max;cnt++)
                                    write(cinfo[cnt].sock, buf, sizeof(buf));
                            }
                        }
                    }

                    // 값 맞추기
                    else{
                        str_len=read(cinfo[j].sock, buf, BUF_SIZE);
                        
                        if(str_len==0)    // close request!
                        {
                            FD_CLR(cinfo[j].sock, &reads);
                            close(cinfo[j].sock);
                            printf("User %s has finished\n", cinfo[j].name); // 종료
                            clnt_cnt--;
                            if(clnt_cnt==0){
                                close(serv_sock);
                                return 0; //종료
                            }
                        }
                        
                        // 매칭
                        else
                        {
                            int solv = (int)buf[0];
                            if(cinfo[j].prob[cinfo[j].stage] == solv){
                                sprintf(buf, "<ANSWER> %d Go Next Stage\n\n", solv);
                                cinfo[j].stage++; // 다음 스테이지
                                
                                // // 클리어!
                                // if(cinfo[j].stage == 5){
                                //     sprintf(buf, "<ANSWER> %d\n",solv);
                                // }                                    
                            }
                            
                            else if(cinfo[j].prob[cinfo[j].stage] > solv)
                                sprintf(buf, "<UP> Bigger than %d\n\n", solv);
                            
                            else
                                sprintf(buf, "<DOWN> Smaller than %d\n\n", solv);

                            // 스테이지 맞추면 다음 스테이지
                            write(i, buf, strlen(buf));// echo!
                            
                            // 종료시 기록 하고 끝내기
                            if(cinfo[j].stage == 5 && cinfo[j].mode == COMP){
                                read(cinfo[j].sock, buf, BUF_SIZE);
                                
                                strcpy(rankName[serv_rank], cinfo[j].name);
                                sprintf(buf,"\n-----------------END GAME-----------------\n");
                                sprintf(buf,"%sUSER RANKING\n",buf);
                                for(int z=1;z<=serv_rank;z++)
                                    sprintf(buf, "%s#%d - %s\n",buf,z,rankName[z]);
                                serv_rank++; // 랭킹 하나 추가
                                sprintf(buf,"%s\n-----------------END GAME-----------------\n\n",buf);
                                write(cinfo[j].sock, buf, strlen(buf)); // 출력
                                
                                FD_CLR(cinfo[j].sock, &reads);
                                close(cinfo[j].sock);
                                printf("%s\n",buf);
                                printf("User %s has finished\n", cinfo[j].name); // 종료
                                clnt_cnt--;
                                if(clnt_cnt==0){
                                    close(serv_sock);
                                    return 0; //종료
                                }
                            }
                        }
                    }

				}
			}
		}
	}
	close(serv_sock);
	return 0;
}

// 에러처리
void error_handling(char *buf)
{
	fputs(buf, stderr);
	fputc('\n', stderr);
	exit(1);
}

// 문제 만들기
void makeProb(int * prob){
    srand(time(NULL));
    for(int i=0;i<5;i++){
        prob[i] = (rand()%100)+1; // 1~100
    }
    
}