#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<pthread.h>
#include<dirent.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<ctype.h>
#include<time.h>
#include<signal.h>

#define BUF_SIZE 100
#define MAX_CLNT 256
#define MAX_ID 15		//최대 id, password 사이즈
#define MAX_LENGTH 100	//최대 client 수

void *handle_clnt(void *arg);
void error_handling(char *msg);
void server_init();		//서버 시작시 초기화
int login(int clnt_sock,char *clnt_id);	//로그인
int sign_up(int clnt_sock,char *clnt_id);	//회원가입
int mail_function(int clnt_sock,char *clnt_id); //메일 기능 메뉴
int mail_listopen(int clnt_sock,char *clnt_id, char mail_lit[][3][MAX_LENGTH],int* mail_count,char mail_folder[][MAX_LENGTH]);	//메일 출력
int mail_objectopen(int clnt_sock,char *clnt_id, char mail_lit[][3][MAX_LENGTH],int* mail_count,char mail_folder[][MAX_LENGTH]);	//메일 오픈
int mail_send(int clnt_sock,char *clnt_id, char mail_lit[][3][MAX_LENGTH],int* mail_count,char mail_folder[][MAX_LENGTH]);	//메일 송신
void sys_write(char *buf,int clnt_sock);	//[SYSTEM] 출력
void error_write(char *buf,int clnt_sock);	//[ERROR] 출력
void opt_write(char *buf,int clnt_sock);	//clnt opt 출력
void text_write(char *buf,int clnt_sock,int i); 	//text 출력
void sig_int(int signo);	//siagction 설치

pthread_mutex_t mutx;

char id[MAX_LENGTH][MAX_ID];
char password[MAX_LENGTH][MAX_ID];
int max_user=-1;

int main(int argc, char *argv[])
{
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	int clnt_adr_sz;
	pthread_t t_id;
	struct sigaction act;

	act.sa_flags=0;
	sigemptyset(&act.sa_mask);
	act.sa_handler = sig_int;
	sigaction(SIGINT,&act,0);

	if(argc!=2)
	{
		printf("Usage : %s <port>\n",argv[0]);
		exit(1);
	}
	server_init();
	pthread_mutex_init(&mutx,NULL);
	serv_sock=socket(PF_INET, SOCK_STREAM,0);

	memset(&serv_adr,0,sizeof(serv_adr));
	serv_adr.sin_family=AF_INET;
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_adr.sin_port=htons(atoi(argv[1]));

	if(bind(serv_sock, (struct sockaddr*)&serv_adr,sizeof(serv_adr))==-1)
		error_handling("bind() error");
	if(listen(serv_sock,10)==-1)
		error_handling("listen() error");
	while(1)
	{
		clnt_adr_sz=sizeof(clnt_adr);
		clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr,&clnt_adr_sz);
		pthread_create(&t_id,NULL,handle_clnt,(void*)&clnt_sock);
		pthread_detach(t_id);
	}
	close(serv_sock);
	return 0;
}
void *handle_clnt(void *arg)
{
	int clnt_sock=*((int*)arg);
	char clnt_id[MAX_ID];

	while(1){
		if(login(clnt_sock, clnt_id)==-1){
			opt_write("exit",clnt_sock);
			shutdown(SHUT_WR,clnt_sock);
			return NULL;
		}
		while(1){
			if(mail_function(clnt_sock, clnt_id)==-1){
				break;
			}
		}
	}

	close(clnt_sock);
	return NULL;
}
int login(int clnt_sock,char *clnt_id){
	char buf[BUF_SIZE]={0,};
	char send_temp[BUF_SIZE]={0,};
	char temp_id[MAX_ID];
	char temp_password[MAX_ID];
	int read_cnt=0;

	while(1){
		sleep(1);
		opt_write("clear",clnt_sock);
		sleep(1);
		sys_write("1 : Login\n",clnt_sock);
		sys_write("2 : Sign-Up \n",clnt_sock);
		sys_write("3 : Exit\n[COMMAND] : ",clnt_sock);
		read_cnt = read(clnt_sock,buf,sizeof(buf));
		buf[read_cnt-1]='\0';
		if(strcmp(buf,"1")==0){
			sys_write("ID : ",clnt_sock);
			read_cnt = read(clnt_sock,temp_id,sizeof(temp_id));
			temp_id[read_cnt-1]='\0';
			sys_write("PASSWORD : ",clnt_sock);
			read_cnt = read(clnt_sock,temp_password,sizeof(temp_password));
			temp_password[read_cnt-1]='\0';
			for(int i = 0; i<=max_user;i++){
				if(strcmp(id[i],temp_id)==0){
					if(strcmp(password[i],temp_password)==0){
						sprintf(buf,"Hello! %s\n",temp_id);
						sys_write(buf,clnt_sock);
						strcpy(clnt_id,temp_id);
						return 1;
					}
				}
			}
			error_write("Not valid ID or PASSWORD\n",clnt_sock);
		}
		else if(strcmp(buf,"2")==0){
			sign_up(clnt_sock, clnt_id);
		}
		else if(strcmp(buf,"3")==0){
			return -1;
		}
		else{
			error_write("1,2 or 3\n",clnt_sock);
		}
	}

	return 0;
}

int sign_up(int clnt_sock,char *clnt_id){
	char buf[BUF_SIZE];
	char temp_id[BUF_SIZE]={0,};
	char temp_password[BUF_SIZE]={0,};
	int read_cnt=0;
	FILE *fp;

	sleep(1);
	opt_write("clear",clnt_sock);
	sleep(1);
	sys_write("Sign-Up\t ID and PASSWORD must be less than 14 alphabet\n",clnt_sock);
	sys_write("ID : ",clnt_sock);
	read_cnt = read(clnt_sock,temp_id,sizeof(temp_id));
	temp_id[read_cnt-1]='\0';
	sys_write("PASSWORD : ",clnt_sock);
	read_cnt = read(clnt_sock,temp_password,sizeof(temp_password));
	temp_password[read_cnt-1]='\0';
	if(strlen(temp_id) > 14 || strlen(temp_password) > 14){
		error_write("ID and PASSWORD must be less than 14 alphabet\n",clnt_sock);
		return -1;
	}
	while(1){
		sprintf(buf,"Check ID : %s,  PASSWORD : %s (Y/N)? ",temp_id,temp_password);
		sys_write(buf,clnt_sock);
		read_cnt = read(clnt_sock,buf,sizeof(buf));
		buf[read_cnt-1]='\0';
		if(strcmp(buf,"Y")==0){
			pthread_mutex_lock(&mutx);
			for(int i=0;i<=max_user;i++){
				if(strcmp(temp_id,id[i])==0){
					sprintf(buf,"ID : %s is overlaped\n",temp_id);
					sys_write(buf,clnt_sock);
					pthread_mutex_unlock(&mutx);
					return -1;
				}
			}
			sprintf(buf,"%s%s",".//Server//",temp_id);
			mkdir(buf,0777);
			strcpy(id[++max_user],temp_id);
			strcpy(password[max_user],temp_password);
			sprintf(buf,"%s%s%s",".//Server//",temp_id,"//password.dat");
			fp=fopen(buf,"w+");
			fputs(temp_password,fp);
			fclose(fp);
			pthread_mutex_unlock(&mutx);
			return 1;
		}
		else if(strcmp(buf,"N")==0){
			return -1;
		}
		else{
			error_write("Y or N\n",clnt_sock);
		}
	}
	return 1;
}

int mail_function(int clnt_sock,char *clnt_id){
	char buf[BUF_SIZE];
	int read_cnt=0;
	FILE *fp;
	char mail_folder[MAX_LENGTH][MAX_LENGTH];
	char mail_list[MAX_LENGTH][3][MAX_LENGTH]={0,};
	int mail_count = -1;
	
	while(1){
		sleep(1);
		opt_write("clear",clnt_sock);
		sleep(1);
		sys_write("1 : Mail Open \n",clnt_sock);
		sys_write("2 : Mail Send \n",clnt_sock);
		sys_write("3 : Logout\n[COMMAND] : ",clnt_sock);
		read_cnt = read(clnt_sock,buf,sizeof(buf));
		buf[read_cnt-1]='\0';
		if(strcmp(buf,"1")==0){
			mail_listopen(clnt_sock,clnt_id, mail_list,&mail_count,mail_folder);
			mail_objectopen(clnt_sock, clnt_id,mail_list,&mail_count,mail_folder);
		}
		else if(strcmp(buf,"2")==0){
			mail_send(clnt_sock, clnt_id,mail_list,&mail_count,mail_folder);
		}
		else if(strcmp(buf,"3")==0){
			return -1;
		}
		else{
			error_write("1, 2 or 3\n",clnt_sock);
		}
		mail_count = -1;
	}

	return 0;
}

int mail_listopen(int clnt_sock,char *clnt_id, char mail_list[][3][MAX_LENGTH],int* mail_count,char mail_folder[][MAX_LENGTH]){	//메일 출력
	DIR *dir_ptr = NULL;
	struct dirent *file = NULL;
	FILE *fp;
	char file_temp[MAX_LENGTH]={0,};
	int i=0,count=0;
	char *char_pointer=0;

	sprintf(file_temp,"%s%s",".//Server//",clnt_id);
	if((dir_ptr = opendir(file_temp)) == NULL){
		error_handling("Not valid directory.\n");
		exit(1);
	}
	while((file = readdir(dir_ptr)) != NULL){
		if(strcmp(file->d_name,".") != 0 && strcmp(file->d_name,"..") !=0 && strcmp(file->d_name,"password.dat") != 0){
			strcpy(mail_folder[++(*mail_count)],file->d_name);
			for(i=0;i<=17;i++){
				if(i==17){
					strcpy(mail_list[*mail_count][0],file_temp);
					memset(file_temp,0,strlen(file_temp));
					count=0;
				}
				else if(mail_folder[*mail_count][i]=='$'){
					if(count==0 || count==1){
						file_temp[i]='/';
						count++;
					}
					else if(count==2){
						file_temp[i]=' ';
						count++;
					}
					else if(count==3){
						file_temp[i]=':';
						count++;
					}
				}
				else{
					file_temp[i]=mail_folder[*mail_count][i];
				}
			}
			for(i=17;i<=strlen(mail_folder[*mail_count]);i++){
				if(mail_folder[*mail_count][i]=='$'){
					strcpy(mail_list[*mail_count][1],file_temp);
					memset(file_temp,0,strlen(file_temp));
					count=0;
				}
				else if(i==strlen(mail_folder[*mail_count])){
					file_temp[count]=mail_folder[*mail_count][i];
					strcpy(mail_list[*mail_count][2],file_temp);
					count=0;
				}
				else{
					file_temp[count]=mail_folder[*mail_count][i];
					count++;
				}
			}
		}
	}
	sleep(1);
	opt_write("clear",clnt_sock);
	sleep(1);
	sys_write("     Date              Sender        Title\n",clnt_sock);
	for(i=0;i<=*mail_count;i++){
		sprintf(file_temp,"%d :  %s %-14s %s\n",i,mail_list[i][0],mail_list[i][1],mail_list[i][2]);
		sys_write(file_temp,clnt_sock);
	}

	return 0;
}
int mail_objectopen(int clnt_sock,char *clnt_id, char mail_lit[][3][MAX_LENGTH],int* mail_count,char mail_folder[][MAX_LENGTH]){

	char buf[BUF_SIZE];
	int read_cnt;
	int file_temp;
	int i=0;
	FILE *fp;

	sys_write("q or Q : Return to Menu\n[COMMAND] : ",clnt_sock);
	read_cnt = read(clnt_sock,buf,sizeof(buf));
		buf[read_cnt-1]='\0';

	if(strcmp(buf,"q")==0||strcmp(buf,"Q")==0){
	}
	else if(atoi(buf)<=*mail_count&&atoi(buf)>=0){
		sleep(1);
		opt_write("clear",clnt_sock);
		sleep(1);
		opt_write("clear",clnt_sock);
		sprintf(buf,"%s%s%s%s%s",".//Server//",clnt_id,"//",mail_folder[atoi(buf)],"//mail.txt");
		fp = fopen(buf,"r");
		while(!feof(fp)){
			fgets(buf, BUF_SIZE, fp);
			if(strcmp(buf,"\n")!=0)
				text_write(buf,clnt_sock,i++);
			memset(&buf,0,sizeof(buf));
		}
		sys_write("if Press any key, return back\n",clnt_sock);
		read(clnt_sock,buf,sizeof(buf));
	}
	else{
		error_write("Invaild Index\n",clnt_sock);
	}
	return 0;
}
int mail_send(int clnt_sock,char *clnt_id, char mail_lit[][3][MAX_LENGTH],int* mail_count,char mail_folder[][MAX_LENGTH]){

	char buf[BUF_SIZE];
	char send_id[MAX_ID];
	char send_title[BUF_SIZE];
	char send_dirname[BUF_SIZE];
	char dirroot[BUF_SIZE];
	int read_cnt;
	int file_temp;
	int i=0,count=0;
	FILE *fp;
	struct tm *send_time;
	const time_t t = time(NULL);

	send_time = localtime(&t);

	sys_write("Receiver : ",clnt_sock);
	read_cnt = read(clnt_sock,buf,sizeof(buf));
	buf[read_cnt-1]='\0';
	strcpy(send_id,buf);
	for(i=0;i<=max_user;i++){
		if(strcmp(send_id,id[i])==0)
			count++;
	}
	if(count <=0){
		error_write("Invalid ID\n",clnt_sock);
		return 0;
	}

	sys_write("Title : ",clnt_sock);
	read_cnt = read(clnt_sock,send_title,sizeof(send_title));
	send_title[read_cnt-1]='\0';

	sprintf(send_dirname,".//Server//%s//%d$%02d$%02d$%02d$%02d$%s$%s",send_id,send_time->tm_year+1900,send_time->tm_mon+1,send_time->tm_mday,send_time->tm_hour,send_time->tm_min,clnt_id,send_title);
	mkdir(send_dirname,0776);
	sprintf(send_dirname,"%s//mail.txt",send_dirname);

	sys_write("If write \"end\" send to receiver\nContents : \n",clnt_sock);

	fp = fopen(send_dirname,"w");
	while(1){
		read_cnt = read(clnt_sock,buf,sizeof(buf));
		buf[read_cnt]='\0';
		if(strcmp(buf,"end\n")==0){
			fclose(fp);
			break;
		}
		fputs(buf,fp);
	}
	return 0;
}
void sys_write(char *buf,int clnt_sock){
	char temp[BUF_SIZE]={0,};

	sprintf(temp,"%-10s %s","[SYSTEM]",buf);
	write(clnt_sock,temp,strlen(temp));
}

void error_write(char *buf,int clnt_sock){
	char temp[BUF_SIZE]={0,};

	sprintf(temp,"%-10s %s","[ERROR]",buf);
	write(clnt_sock,temp,strlen(temp));
}

void opt_write(char *buf,int clnt_sock){
	char temp[BUF_SIZE]={0,};

	sprintf(temp,"%s%s","*#205*#",buf);
	write(clnt_sock,temp,strlen(temp));
}

void text_write(char *buf,int clnt_sock,int i){
	char temp[BUF_SIZE]={0,};

	if(strlen(buf)!=0){
		sprintf(temp,"%3d: %s",i,buf);
		write(clnt_sock,temp,strlen(temp));
	}
}

void server_init(){
	DIR *dir_ptr = NULL;
	struct dirent *file = NULL;
	FILE *fp;
	//char root[]="./Server";
	char password_file[MAX_LENGTH]={0,};
	char password_temp[MAX_ID]={0,};

	if((dir_ptr = opendir(".//Server")) == NULL){
		error_handling("Not valid directory.\n");
		exit(1);
	}
	while((file = readdir(dir_ptr)) != NULL){
		if(strcmp(file->d_name,".") != 0 && strcmp(file->d_name,"..") != 0){
			strcpy(id[++max_user],file->d_name);
		}
	}
	for(int i=0;i<=max_user;i++){
		sprintf(password_file,"%s%s%s",".//Server//",id[i],"//password.dat");
		fp=fopen(password_file,"r");
		fgets(password_temp,MAX_ID,fp);
		strcpy(password[i],password_temp);
		fclose(fp);
	}
	closedir(dir_ptr);
}

void error_handling(char *msg)
{
	fputs(msg,stderr);
	fputc('\n',stderr);
	exit(1);
}

void sig_int(int signo){
	int i;
	char buf[BUF_SIZE]="Server is shutdown!";

	for(i=4;i<=MAX_CLNT;i++){
		error_write(buf,i);
		close(i);
	}
	exit(1);
}