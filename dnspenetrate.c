/*
 * dnspenetrate.c
 * Copyright (C) 2002   Robert Marcin Nowotniak <rob@submarine.ath.cx>
 * Pon 22 Lip 14:27:33 2002
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <termios.h>
#include <math.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <netdb.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#define PROGRAMNAME		"dnspenetrate"
#define PROGVERION		"0.1"
#define EMAIL				"robert@nowotniak.com"

// Kody steruj±ce konsoli
#define BOLD				"\e[1m"
#define NORMAL				"\e[0m"
#define BLINK				"\e[5m"
#define RED					"\e[1;31m"
#define GREEN				"\e[1;32m"
#define YELLOW				"\e[1;33m"
#define AQUA				"\e[0;36m"


extern char** environ;
char* hostname;
char* ip;
char** adresy=(char**)NULL; // U¿ywane tylko, je¶li wiêcej ni¿ jeden
int interactive=1;

union __odpowiedz {
	struct ___odpH{
		char blad;
		ns_type Type;
		int size;
	} h;
	struct __dane
	{
		struct ___odpH h;
		char **RRs;
	} dane;
};

void usage()
{
	printf("U¿ycie: %s [-hVvc] <host>\n",PROGRAMNAME);
	exit(EXIT_SUCCESS);
}

void BLAD(const char* f)
{
	fprintf(stderr,"B³±d w funkcji %s\n %s\n",f,strerror(errno));
	exit(EXIT_FAILURE);
}

void* MALLOC(size_t size)
{
	void* tmp;

	tmp=malloc(size);
	if(!tmp){
		fprintf(stderr,"B³±d podczas przydzielania pamiêci...\n");
		fprintf(stderr,"B³±d: %s\n",strerror(errno));
		exit(EXIT_FAILURE);
	}
	return(tmp);
}


int wypisz(char *fmt, ...)
{
	va_list ap;
	char *fmt2,*ptr;
	int n=0;
	int result;

	if( ! interactive )
	{
		ptr=fmt2=(char*)MALLOC(strlen(fmt)+1);

		while( fmt[n] )
		{
			if( fmt[n] != '\e' )
				*ptr++=fmt[n];
			else
				do
					++n;
				while ( fmt[n] != 'm' );
			++n;
		}
		*ptr='\0';

		fmt=fmt2;
		va_start(ap,fmt);
		result=vprintf(fmt,ap);
		va_end(ap);

		return result;
	}

	va_start(ap,fmt);
	result=vprintf(fmt,ap);
	va_end(ap);

	return result;
}



struct proto_family {
	int	number;
	char*	name;
} families[]=
{
	{0,"AF_UNSPEC"},
	{1,"AF_UNIX"},
	{2,"AF_INET"},
	{28,"AF_INET6"},
	{(int)NULL,(char*)NULL}
};

char* getfamilybynumber(int addrtype)
{
	int n;
	for(n=0;families[n].name;n++)
		if(families[n].number==addrtype)
			return(families[n].name);
	return("AF_UNKNOWN");
}

struct TLD
{
	char*		tld;
	char*		opis;
} tlds[]=
{
	{"ac","Ascension Island"},
	{"ad","Andorra"},
	{"ae","United Arab Emirates"},
	{"af","Afghanistan"},
	{"ag","Antigua and Barbuda"},
	{"ai","Anguilla"},
	{"al","Albania"},
	{"am","Armenia"},
	{"an","Netherlands Antilles"},
	{"ao","Angola"},
	{"aq","Antarctica"},
	{"ar","Argentina"},
	{"as","American Samoa"},
	{"at","Austria"},
	{"au","Australia"},
	{"aw","Aruba"},
	{"az","Azerbaijan"},
	{"ba","Bo¶nia i Hercegowina"},
	{"bb","Barbados"},
	{"bd","Bangladesz"},
	{"be","Belgia"},
	{"bf","Burkina Faso"},
	{"bg","Bu³garia"},
	{"bh","Bahrain"},
	{"bi","Burundi"},
	{"bj","Benin"},
	{"bm","Bermuda"},
	{"bn","Brunei Darussalam"},
	{"bo","Boliwia"},
	{"br","Brazylia"},
	{"bs","Bahamas"},
	{"bt","Bhutan"},
	{"bv","Bouvet Island"},
	{"bw","Botswana"},
	{"by","Bia³oru¶"},
	{"bz","Belize"},
	{"ca","Canada"},
	{"cc","Cocos Islands (Keeling)"},
	{"cd","Demokratyczna Republika Konga"},
	{"cf","Republika ¦rodkowej Afryki"},
	{"cg","Republika Konga"},
	{"ch","Szwajcaria"},
	{"ci","Cote d'Ivoire (Ivory Coast)"},
	{"ck","Wysky Cook'a"},
	{"cl","Chile"},
	{"cm","Kamerun"},
	{"cn","Chiny"},
	{"co","Kolumnia"},
	{"com","Organizacja Komercyjna"},
	{"cr","Costa Rica"},
	{"cs","Czechos³owacja"},
	{"cu","Kuba"},
	{"cv","Cap Verde"},
	{"cx","Wyspy Bo¿ego Narodzenia"},
	{"cy","Cypr"},
	{"cz","Czechy"},
	{"de","Niemcy"},
	{"dj","Djibouti"},
	{"dk","Dania"},
	{"dm","Dominika"},
	{"do","Dominican Republic"},
	{"dz","Algeria"},
	{"ec","Ekwador"},
	{"edu","Instytucja Naukowa"},
	{"ee","Estonia"},
	{"eg","Egipt"},
	{"eh","Zachodnia Sahara"},
	{"er","Eritrea"},
	{"es","Hiszpania"},
	{"et","Etiopia"},
	{"fi","Finlandia"},
	{"fj","Fiji"},
	{"fk","Falkland Islands"},
	{"fm","Mikronezja"},
	{"fo","Faroe Islands"},
	{"fr","Francja"},
	{"ga","Gabon"},
	{"gd","Grenada"},
	{"ge","Georgia"},
	{"gf","French Guiana"},
	{"gg","Guernsey"},
	{"gh","Ghana"},
	{"gi","Gibraltar"},
	{"gl","Greenland"},
	{"gm","Gambia"},
	{"gn","Gwinea"},
	{"gov","Organizacja Rz±dowa (USA)"},
	{"gp","Guadeloupe"},
	{"gq","Equatorial Guinea"},
	{"gr","Grecja"},
	{"gs","South Georgia and the South Sandwich Islands"},
	{"gt","Gwatemala"},
	{"gu","Guam"},
	{"gw","Guinea-Bissau"},
	{"gy","Guyana"},
	{"hk","Hong Kong"},
	{"hm","Heard and McDonald Islands"},
	{"hn","Honduras"},
	{"hr","Chorwacja"},
	{"ht","Haiti"},
	{"hu","Wêgry"},
	{"id","Indonezia"},
	{"ie","Irlandia"},
	{"il","Izrael"},
	{"im","Isle of Man"},
	{"in","Indie"},
	{"int","Organizacja Miêdzynarodowa"},
	{"io","British Indian Ocean Territory"},
	{"iq","Irak"},
	{"ir","Iran"},
	{"is","Islandia"},
	{"it","W³ochy"},
	{"je","Jersey"},
	{"jm","Jamajka"},
	{"jo","Jordan"},
	{"jp","Japonia"},
	{"ke","Kenia"},
	{"kg","Kyrgyzstan"},
	{"kh","Kambod¿a"},
	{"ki","Kiribati"},
	{"km","Comoros"},
	{"kn","Saint Kitts and Nevis"},
	{"kp","Korea, Democratic People's Republic"},
	{"kr","Korea, Republic of"},
	{"kw","Kuweit"},
	{"ky","Cayman Islands"},
	{"kz","Kazakhstan"},
	{"la","Lao People's Democratic Republic"},
	{"lb","Lebanon"},
	{"lc","Saint Lucia"},
	{"li","Liechtenstein"},
	{"lk","Sri Lanka"},
	{"lr","Liberia"},
	{"ls","Lesotho"},
	{"lt","Lithuania"},
	{"lu","Luxembourg"},
	{"lv","Latvia"},
	{"ly","Libyan Arab Jamahiriya"},
	{"ma","Morocco"},
	{"mc","Monaco"},
	{"md","Moldova, Republic of"},
	{"mg","Madagascar"},
	{"mh","Marshall Islands"},
	{"mil","Organizacja Rz±doway (Departament Obrony USA)"},
	{"mk","Macedonia, Former Yugoslav Republic"},
	{"ml","Mali"},
	{"mm","Myanmar"},
	{"mn","Mongolia"},
	{"mo","Macau"},
	{"mp","Northern Mariana Islands"},
	{"mq","Martinique"},
	{"mr","Mauritani"},
	{"ms","Montserrat"},
	{"mt","Malta"},
	{"mu","Mauritius"},
	{"mv","Maldives"},
	{"mw","Malawi"},
	{"mx","Meksyk"},
	{"my","Malezia"},
	{"mz","Mozambique"},
	{"na","Namibia"},
	{"nc","Nowa Kaledonia"},
	{"ne","Niger"},
	{"net","Organizacja Sieciowa"},
	{"nf","Norfolk Island"},
	{"ng","Nigeria"},
	{"ni","Nicaragua"},
	{"nl","Holandia"},
	{"no","Norwegia"},
	{"np","Nepal"},
	{"nr","Nauru"},
	{"nt","Neutral Zone"},
	{"nu","Niue"},
	{"nz","Nowa Zelandia"},
	{"om","Oman"},
	{"org","Organizja Niedochodowa"},
	{"pa","Panama"},
	{"pe","Peru"},
	{"pf","French Polynesia"},
	{"pg","Papua New Guinea"},
	{"ph","Filipiny"},
	{"pk","Pakistan"},
	{"pl","Polska"},
	{"pm","St. Pierre and Miquelon"},
	{"pn","Pitcairn Island"},
	{"pr","Puerto Rico"},
	{"ps","Palestinian Territories"},
	{"pt","Portugalia"},
	{"pw","Palau"},
	{"py","Paraguay"},
	{"qa","Qatar"},
	{"re","Reunion Island"},
	{"ro","Romania"},
	{"ru","Rosja"},
	{"rw","Rwanda"},
	{"sa","Arabia Saudyjska"},
	{"sb","Solomon Islands"},
	{"sc","Seychelles"},
	{"sd","Sudan"},
	{"se","Szwecja"},
	{"sg","Singapore"},
	{"sh","St. Helena"},
	{"si","S³owenia"},
	{"sj","Svalbard and Jan Mayen Islands"},
	{"sk","S³owacja"},
	{"sl","Sierra Leone"},
	{"sm","San Marino"},
	{"sn","Senegal"},
	{"so","Somalia"},
	{"sr","Suriname"},
	{"sv","El Salvador"},
	{"st","Sao Tome and Principe"},
	{"sy","Syrian Arab Republic"},
	{"sz","Swaziland"},
	{"tc","Turks and Caicos Islands"},
	{"td","Chad"},
	{"tf","French Southern Territories"},
	{"tg","Togo"},
	{"tj","Tajikistan"},
	{"tk","Tokelau"},
	{"tm","Turkmenistan"},
	{"tn","Tunezja"},
	{"to","Tonga"},
	{"tp","East Timor"},
	{"tr","Turcja"},
	{"tt","Trinidad and Tobago"},
	{"tv","Tuvalu"},
	{"tw","Taiwan"},
	{"tz","Tanzania"},
	{"ua","Ukraina"},
	{"ug","Uganda"},
	{"uk","Wielka Brytania"},
	{"um","US Minor Outlying Islands"},
	{"us","Stany Zjednoczone"},
	{"uy","Urugway"},
	{"uz","Uzbekistan"},
	{"va","Holy See (City Vatican State)"},
	{"vc","Saint Vincent and the Grenadines"},
	{"ve","Wenezuela"},
	{"vg","Virgin Islands (British)"},
	{"vi","Virgin Islands (USA)"},
	{"vn","Vietnam"},
	{"vu","Vanuatu"},
	{"wf","Wallis and Futuna Islands"},
	{"ws","Zachodnia Samoa"},
	{"ye","Yemen"},
	{"yt","Mayotte"},
	{"yu","Jugos³awia"},
	{"za","Afryka Po³udniowa"},
	{"zm","Zambia"},
	{"zw","Zimbabwe"},
	{"aero","Transport Lotniczy"},
	{"addr","Strefa zwrotna"},
	{"biz","Organizacja biznesowa"},
	{"coop","Organizacja wspó³pracuj±ca (Cooperative)"},
	{"info","Domena ogólnego u¿ytku"},
	{"museum","Muzea"},
	{"name","Osoba prywatna"},
	{"pro","Ksiêgowi / Prawnicy / Lekarze (Accountants, lawyers, and physicians)"},
	{(char*)NULL,(char*)NULL}
};

char* CheckTld(const char* domena)
{
	int n=0;
	char *tld;
	char tmp[domena?strlen(domena)+1:1];

	if(!domena)
		return("Brak.");

	strncpy(tmp,domena,strlen(domena)+1);

	tld=rindex(tmp,'.');
	if(!tld)
		return("Brak.");
	else if( strlen(tld) == 1 )  /* domena =~ /.*\.tld\.$/ */
	{
		*tld='\0';
		tld=rindex(tmp,'.');
		if ( strlen(tld) == 1 )
			return("Nieprawid³owa nazwa domenowa.");
	}

	tld++;

	do
		if(!strncasecmp(tlds[n].tld,tld,strlen(tld)>9?9:strlen(tld)))
			return(tlds[n].opis);
	while(tlds[++n].tld);
	return("Nieznany.");
}

/*

struct hostent*
DupHostent(struct hostent* hn1, struct hostenv* hn2)
// Skrupulatnie kopiuje zawarto¶æ struktury hn2 do hn1 i zwraca hn1
// Je¶li hn1 == NULL, to alokuje pamiêæ samodzielnie, kopiuje i zwraca adres
{
	int memsum,aliassum,addrsum;
	int n;

	memsum=sizeof(struct hostent)+strlen(hn2->h_name)+1;
	elsum=0;

	for(n=0;hn2->h_aliases;n++){
		memsum+=strlen(hn2->h_aliases[n]);
		++aliassum;
	}
	for(n=0;hn2->h_addr_list;n++){
		memsum+=strlen(hn2->h_addr_list[n]);
		++addrsum;
	}
	memsum+=4*(addrsum+aliassum);

	if(!hn1)
	{
		hn1=(struct hostent*)MALLOC(memsum);
		(struct hostent)*hn1=(struct hostent)*hn2;
		hn1->h_aliases=hn1+sizeof(struct hostent);
		hn1->h_addr_list=hn1+sizeof(struct hostent)+sizeof(char**);
		*(hn1->h_aliases)=hn1+sizeof(struct hostent)+2*sizeof(char**);
		*(hn1->h_addr_list)=hn1+sizeof(struct hostent)+2*sizeof(char**)+4*aliassum;
		for(n=0;n<aliassum;n++)
			hn1->h_aliases[n]=hn2->h_aliases[n];

	}

}

*/



int isip(const char* cos)    // Zwraca niezero, je¶li 'cos' to prawid³owy adres IP(v4)
{
	char *ptr,*ptr0;
	char tmp[strlen(cos)+1];
	int dots=-1;
	int n;

	strncpy(tmp,cos,strlen(cos)+1);

	ptr=tmp;
	for(dots=-1; ( ptr0 = strsep((char**)&ptr,".") ) ;dots++)
	{
		if( !strcmp(ptr0,"") || dots>3 )
			return(0);
		for(n=0;n<strlen(ptr0);n++)
			if(!isdigit(ptr0[n]))
				return 0;
		if( !(0 <= atoi(ptr0) && atoi(ptr0) <=255) )
			return 0;
	}

	if(dots==3)
		return(1);
	else
		return(0);
}


char* rozwiaz_ip(const char* ip)
{
	struct in_addr a;
	struct hostent* hn;
	char* ptr;

	inet_aton(ip,&a);
	hn=gethostbyaddr((char*)&a,4,AF_INET);
	if(!hn)
		return((char*)NULL);

	if(!hn->h_name)
		return((char*)NULL);
	if(hn->h_name==ip)
		return((char*)NULL);

	ptr=MALLOC(strlen(hn->h_name)+1);
	return(strncpy(ptr,hn->h_name,strlen(hn->h_name)+1));
}

char* rozwiaz_hostname(const char* hostname)
{
	struct hostent* hn;
	char *ptr,*tmp;

	hn=gethostbyname2(hostname,AF_INET);
	if(!hn)
		return((char*)NULL);

	tmp=inet_ntoa(*(struct in_addr*)(hn->h_addr));
	if(!tmp)
		return(NULL);
	ptr=MALLOC(strlen(tmp)+1);
	return(strncpy(ptr,tmp,strlen(tmp)+1));
}

void WypiszAdresy(const char* hostname, const char* ip)
{
	struct hostent* hn;
	char* str,*str2=NULL;
	char** tmp;
	struct in_addr *ptr0,*ptr;
	int n,sum;

	if(hostname)
	{
		hn=gethostbyname2(hostname,AF_INET);
		if(hn && hn->h_addr_list[1])
		{
			for(n=0,sum=0;hn->h_addr_list[n];n++)
				sum++;
			sum++;
			tmp=adresy=(char**)MALLOC( sum*(16+sizeof(char*)) + sizeof(char*) );
			for(n=0;hn->h_addr_list[n];n++,tmp++){
				*tmp = (char*)(adresy)+4*sum+16*n;
				strncpy(*tmp,inet_ntoa(*(struct in_addr*)hn->h_addr_list[n]),16);
			}
			*tmp = '\0';




			// Kopiowanie tablicy adresów, bo gethostbyname()
			// nie mo¿na dwa razy NA RAZ wywo³aæ
			for(sum=0,n=0;hn->h_addr_list[n];n++)
				++sum;
			ptr0=ptr=(struct in_addr*)MALLOC(sum*sizeof(struct in_addr));

			for(n=0;n<sum;n++)
				*ptr++=*(struct in_addr*)hn->h_addr_list[n];

			wypisz(AQUA " Adresy IP:\n" NORMAL);
			for(n=0;n<sum;n++)
			{
				str=inet_ntoa(ptr0[n]);
				str2=rozwiaz_ip(str);
				if(str2){
					printf("  %s (%s)\n",str,str2);
					if(strncasecmp(str2,hostname,strlen(hostname)+1))
						printf("      [Niespójne mapowanie zwrotne]\n");
					free(str2);
				}else
					printf("  %s\n",str);
			}
			free(ptr0);
		}else{
			str2=rozwiaz_ip(ip);
			if(str2){
				wypisz(AQUA " Adres IP:" NORMAL " %s (%s)\n",ip,str2);
				if(strncasecmp(str2,hostname,strlen(hostname)+1))
					printf("   [Niespójne mapowanie zwrotne]\n");
				free(str2);
			}else
				wypisz(AQUA " Adres IP:" NORMAL " %s\n",ip);
		}
	}else{
		str2=rozwiaz_ip(ip);
		if(str2){
			wypisz(AQUA " Adres IP:" NORMAL " %s (%s)\n",ip,str2);
			if(strncasecmp(str2,hostname,strlen(hostname)+1))
				printf("   [Niespójne mapowanie zwrotne]\n");
			free(str2);
		}else
			wypisz(AQUA " Adres IP:" NORMAL " %s\n",ip);
	}
}

void WypiszAliasy(const char* hostname, const char* ip)
{
	struct in_addr a;
	struct hostent* hn;
	int n;

	if(!ip && !hostname)
		return;

	if(hostname)
	{
		hn=gethostbyname2(hostname,AF_INET);
		if(hn)
			if(hn->h_aliases[1] || (hn->h_aliases[0] && strcmp(hn->h_aliases[0],hostname)))
			{
				wypisz(AQUA" Aliasy:\n"NORMAL);
				for(n=0;hn->h_aliases[n];n++)
					printf("  %s\n",hn->h_aliases[n]);
				return;
			}
	}

	if(ip)
	{
		inet_aton(ip,&a);
		hn=gethostbyaddr((char*)&a,sizeof(struct in_addr),AF_INET);
		if(!hn)
			return;

		if(hn)
			if(hn->h_aliases[1] || (hn->h_aliases[0] && strcmp(hn->h_aliases[0],ip)))
			{
				wypisz(AQUA" Aliasy:\n"NORMAL);
				for(n=0;hn->h_aliases[n];n++)
					printf("  %s\n",hn->h_aliases[n]);
			}
	}
}

union __odpowiedz*
ZapytajSerwer(ns_type Type,const char* zapytanie)
// Ta funkcja powinna korzystaæ z res_query(), zamiast
// nie³adnie, z zewnêtrznego programu... (host)
{
#define HOSTPROG 	"/usr/bin/host"
#define BUFSIZE	150
#define MAXRR		10
#define MAXRRLEN	100
	char *hostAV[]={"host","-t","",(char*)zapytanie,NULL};
	char Bufor[MAXRR][BUFSIZE]={{'\0'}};
	char **gptr; // ogólny wska¼nik na wska¼nik na char
	char fstr[strlen(zapytanie)+1];
	char template[BUFSIZE],rr[MAXRRLEN];
	pid_t pid;
	int err,status,n,pri;
	int rozmiar=0,wpisy=-1;
	int fd[2];
	union __odpowiedz* odp;
	FILE* INPUT;


	if(!zapytanie || !*zapytanie)
		return(NULL);

	switch(Type){
		case ns_t_mx:
			hostAV[2]="mx";
			break;
		case ns_t_ns:
			hostAV[2]="ns";
			break;
		case ns_t_cname:
			hostAV[2]="cname";
			break;
		case ns_t_ptr:
			hostAV[2]="ptr";
			break;
		default:
			return(NULL);
	}

	if( pipe(fd) )
		BLAD("pipe()");
	pid=fork();
	if(pid<0)
		BLAD("fork()");
	if(!pid)
	{
		// Podproces
		close(1);
#ifndef DEBUG
		close(2);
#endif
		close(fd[0]);
		dup(fd[1]);
		execve(HOSTPROG,hostAV,environ);
		fprintf(stderr,"B³±d podczas próby skorzystania z programu %s:\n",HOSTPROG);
		BLAD("execve()");
	}
	close(fd[1]);
	INPUT=fdopen(fd[0],"r");
	if(!INPUT)
		BLAD("fdopen()");

	n=0;
	do{
		fgets(Bufor[n],BUFSIZE-1,INPUT);
		if(ferror(INPUT))
			BLAD("fgets()");
		if( *Bufor[n] && Bufor[n][strlen(Bufor[n])-1]=='\n')
			Bufor[n][strlen(Bufor[n])+1]='\0';
		rozmiar+=strlen(Bufor[n])+1+sizeof(char*);
		++wpisy;
	} while( !feof(INPUT) && ++n<MAXRR );
	++wpisy;
	if(fclose(INPUT))
		BLAD("fclose()");

	err=wait(&status);
	if( err<0 )
		BLAD("wait()");
	else if ( err != pid )
	{
		fprintf(stderr,"Krytyczny b³±d.\nProces potomny mia³ inny PID, ni¿ proces który zosta³ stworzony. (?!?)\n");
		exit(EXIT_FAILURE);
	}
	if(WIFEXITED(status))
		switch(WEXITSTATUS(status)){
			case 1:		// Nie ma takiego hosta ?
			case 69:
				return(NULL);
			case 0:		// Dane pobrane
				break;
			case 2:		// Nieprawid³owe zapytanie / klasa
			default:
				fprintf(stderr,"B³±d: Nie uda³o siê nawi±zaæ wspó³pracy z programem %s.\n",HOSTPROG);
				exit(EXIT_FAILURE);
		}
	else{
		fprintf(stderr,"B³±d podczas próby skorzystania z programu %s.\n",HOSTPROG);
		exit(EXIT_FAILURE);
	}

	rozmiar+=sizeof(union __odpowiedz);
	odp=(union __odpowiedz*)MALLOC(rozmiar);
	odp->h.blad=NOERROR;
	odp->h.Type=Type;
	odp->h.size=rozmiar;
	gptr=odp->dane.RRs=(char**)((char*)odp+sizeof(union __odpowiedz));
	*gptr=(char*)gptr+wpisy*sizeof(char*);


	strncpy(fstr,zapytanie,strlen(zapytanie)+1);
	if(strlen(fstr)>1 && fstr[strlen(fstr)-1]=='.')
		fstr[strlen(fstr)-1]='\0';

	switch(Type){
		case ns_t_mx:
// Ró¿ne wersje 'host' ró¿nie podaj± tê informacjê
			// Wersja BSD
			snprintf(template,BUFSIZE,"%s mail is handled (pri=%%d) by %%s",fstr);
			for( n=0,err=1 ; n<MAXRR && Bufor[n][0] ; n++ ){
				if( sscanf(Bufor[n],template,&pri,rr) != 2 )
					continue;
				err=0;
				snprintf(*gptr,BUFSIZE,"%d %s",pri,rr);
				*(gptr+1)=*gptr+strlen(*gptr)+1;
				++gptr;
			}
			*gptr='\0';
			// Wersja z (w miare nowej dystrybucji) Linuxa
			if( err ){
				snprintf(template,BUFSIZE,"%s MX %%d %%s",fstr);
				gptr=odp->dane.RRs;
				bzero((void*)gptr,wpisy*sizeof(char*));
				*gptr=(char*)gptr+wpisy*sizeof(char*);
				for( n=0,err=1 ; n<MAXRR && Bufor[n][0] ; n++ ){
					if( sscanf(Bufor[n],template,&pri,rr) != 2 )
						continue;
					err=0;
					snprintf(*gptr,BUFSIZE,"%d %s",pri,rr);
					*(gptr+1)=*gptr+strlen(*gptr)+1;
					++gptr;
				}
				*gptr='\0';
			}
			if(err)
				return(NULL);
			break;
		case ns_t_ns:
			// Wersja BSD
			snprintf(template,BUFSIZE,"%s name server %%s",fstr);
			for( n=0,err=1 ; n<MAXRR && Bufor[n][0] ; n++ ){
				if( sscanf(Bufor[n],template,rr) != 1 )
					continue;
				err=0;
				snprintf(*gptr,BUFSIZE,"%s",rr);
				*(gptr+1)=*gptr+strlen(*gptr)+1;
				++gptr;
			}
			*gptr='\0';
			// Wersja z (w miare nowej dystrybucji) Linuxa
			if( err ){
				snprintf(template,BUFSIZE,"%s NS %%s",fstr);
				*gptr=(char*)gptr+wpisy*sizeof(char*);
				bzero((void*)gptr,wpisy*sizeof(char*));
				*gptr=(char*)gptr+wpisy*sizeof(char*);
				for( n=0,err=1 ; n<MAXRR && Bufor[n][0] ; n++ ){
					if( sscanf(Bufor[n],template,rr) != 1 )
						continue;
					err=0;
					snprintf(*gptr,BUFSIZE,"%s",rr);
					*(gptr+1)=*gptr+strlen(*gptr)+1;
					++gptr;
				}
				*gptr='\0';
			}
			if(err)
				return(NULL);
			break;
		case ns_t_cname:
			// Wersja BSD
			snprintf(template,BUFSIZE,"%s is a nickname for %%s",fstr);
			for( n=0,err=1 ; n<MAXRR && Bufor[n][0] ; n++ ){
				if( sscanf(Bufor[n],template,rr) != 1 )
					continue;
				err=0;
				snprintf(*gptr,BUFSIZE,"%s",rr);
				*(gptr+1)=*gptr+strlen(*gptr)+1;
				++gptr;
			}
			*gptr='\0';
			// Wersja z (w miare nowej dystrybucji) Linuxa
			if( err ){
				*gptr=(char*)gptr+wpisy*sizeof(char*);
				snprintf(template,BUFSIZE,"%s CNAME %%s",fstr);
				bzero((void*)gptr,wpisy*sizeof(char*));
				*gptr=(char*)gptr+wpisy*sizeof(char*);
				for( n=0,err=1 ; n<MAXRR && Bufor[n][0] ; n++ ){
					if( sscanf(Bufor[n],template,rr) != 1 )
						continue;
					err=0;
					snprintf(*gptr,BUFSIZE,"%s",rr);
					*(gptr+1)=*gptr+strlen(*gptr)+1;
					++gptr;
				}
				*gptr='\0';
			}
			if(err)
				return(NULL);
			break;
		case ns_t_ptr:
			// Wersja BSD
			snprintf(template,BUFSIZE,"%s domain name pointer %%s",fstr);
			for( n=0,err=1 ; n<MAXRR && Bufor[n][0] ; n++ ){
				if( sscanf(Bufor[n],template,rr) != 1 )
					continue;
				err=0;
				snprintf(*gptr,BUFSIZE,"%s",rr);
				*(gptr+1)=*gptr+strlen(*gptr)+1;
				++gptr;
			}
			*gptr='\0';
			// Wersja z (w miare nowej dystrybucji) Linuxa
			if( err ){
				*gptr=(char*)gptr+wpisy*sizeof(char*);
				snprintf(template,BUFSIZE,"%s PTR %%s",fstr);
				bzero((void*)gptr,wpisy*sizeof(char*));
				*gptr=(char*)gptr+wpisy*sizeof(char*);
				for( n=0,err=1 ; n<MAXRR && Bufor[n][0] ; n++ ){
					if( sscanf(Bufor[n],template,rr) != 1 )
						continue;
					err=0;
					snprintf(*gptr,BUFSIZE,"%s",rr);
					*(gptr+1)=*gptr+strlen(*gptr)+1;
					++gptr;
				}
				*gptr='\0';
			}
			if(err)
				return(NULL);
			break;
		default:
			return(NULL);
	}
	return odp;

}

void WypiszNameservery(const char* hostname)
{
	union __odpowiedz* p;
	int n=0;
	char *ptr,*rev,*aip;

	if(!hostname)
		return;

	p=ZapytajSerwer(ns_t_ns,hostname);
	wypisz(AQUA" Strefa "NORMAL BOLD"%s"NORMAL" oddelegowana",hostname);
	if(p)
	{
		printf(" na:\n");
		while((ptr=p->dane.RRs[n++])!=NULL)
		{
			if(!isip(ptr))
			{
				aip=rozwiaz_hostname(ptr);
				if(!aip){
					printf("   %s (nie wskazuje na ¿aden adres)\n",ptr);
					continue;
				}
				rev=rozwiaz_ip(aip);
				if(!rev)
					printf("   %s (%s [brak wpisu])\n",ptr,aip);
				else if(!strcasecmp(rev,ptr))
					printf("   %s (%s)\n",ptr,aip);
				else{
					printf("   %s (%s [%s])\n",ptr,aip,rev);
					printf("      [Niespójne mapowanie zwrotne]\n");
				free(aip);
				if(rev)
					free(rev);
				}
			}
			else{
				rev=rozwiaz_ip(aip);
				printf("   %s (%s)\n",ptr,rev?rev:"brak wpisu");
				free(rev);
			}
		}
	}else{
		wypisz(": " GREEN "NIE" NORMAL "\n");
	}


	free(p);
}

void baner(void)
{
	wypisz(">>>>"YELLOW"%s"NORMAL">>>>v"YELLOW"%s"NORMAL">>>>"YELLOW "Robert Nowotniak" NORMAL">>>>\n",PROGRAMNAME,PROGVERION);
}

void WypiszMXy(const char* hostname,const char* ip)
{
	union __odpowiedz* p;
	char *ptr,*mx,*aip,*rev;
	int n=0,pri;

	if(!hostname)
		return;

	p=ZapytajSerwer(ns_t_mx,hostname);
	wypisz(AQUA " Poczta" NORMAL " adresowana na *@" BOLD "%s" NORMAL " trafia na:\n",hostname);
	if(p)
		while( (ptr=p->dane.RRs[n++]) )
		{
			pri=atoi(ptr);
			mx=index(ptr,' ')+1;
			if(!isip(ptr))
			{
				aip=rozwiaz_hostname(mx);
				if(!aip){
					printf("   %d %s (nie wskazuje na ¿aden adres)\n",pri,mx);
					continue;
				}
				rev=rozwiaz_ip(aip);
				if(!rev){
					printf("   %d %s (%s [brak wpisu])\n",pri,mx,aip);
					free(aip);
					continue;
				}
				if( !strcasecmp(mx,rev) )
					printf("   %d %s (%s)\n",pri,mx,aip);
				else{
					printf("   %d %s (%s [%s])\n",pri,mx,aip,rev);
					printf("      [Niespójne mapowanie zwrotne]\n");
				}
			}
			else
				printf("   %d %s\n",pri,mx);
			free(aip);
			free(rev);
		}
	else
		printf("   %s\n",hostname);

	free(p);
}

void SprawdzRev(const char* ip)
{
	char *ptr,*ptr0,*host1,*host2;
	char tmp[strlen(ip)+1];
	char cidr[strlen(ip)+1+3]; // ...+ /24
	char adr[4][4];
	char rev[strlen(ip)+1+14]; // ...+ .in-addr.arpa.
	int n=3,m;
	union __odpowiedz *p,*r;
#define MAXCHECKED 20
	static char **already_checked=NULL;
	static char **chkd_ptr=NULL;


	if(!isip(ip))
		return;

	strncpy(tmp,ip,strlen(ip)+1);
	ptr0=ptr=tmp;
	while( (ptr=strsep((char**)&ptr0,".")) )
		strncpy(adr[n--],ptr,4);
	rev[0]='\0';
	for(n=0;n<4;n++)
	{
		strncat(rev,adr[n],3);
		strncat(rev,".",1);
	}
	strcat(rev,"in-addr.arpa.");

	if(already_checked)
		if(chkd_ptr==already_checked+MAXCHECKED-1)
			chkd_ptr=already_checked;
		else{
			for(n=0;already_checked[n];n++)
				if(!strncasecmp(already_checked[n],rev,strlen(already_checked[n])))
					return;
			if(!*chkd_ptr)
				*chkd_ptr=(char*)MALLOC(30);
			strncpy(*chkd_ptr,rev,30);
			++chkd_ptr;
		}
	else{
		chkd_ptr=already_checked=(char**)MALLOC(MAXCHECKED*sizeof(char*));
		bzero(already_checked,MAXCHECKED*sizeof(char*));
		*chkd_ptr=(char*)MALLOC(30);
		strncpy(*chkd_ptr++,rev,30);
	}

	p=ZapytajSerwer(ns_t_cname,rev);
	if(p)
	{
		wypisz(" Wpis w " AQUA "strefie zwrotnej" NORMAL " dla " BOLD "%s" NORMAL " jest aliasem (CNAME) do:\n",rev);
		for(n=0;p->dane.RRs[n];n++)
		{
			printf("     %s ",p->dane.RRs[n]);
			r=ZapytajSerwer(ns_t_ptr,p->dane.RRs[n]);
			if(!r){
				printf("(brak wpisu PTR)\n");
				continue;
			}
			for(m=0;r->dane.RRs[m];m++)
				wypisz("(PTR " BOLD "%s" NORMAL ") ",r->dane.RRs[m]);
			printf("\n");
			free(r);
		}
		free(p);
		return;
	}
	strncpy(cidr,ip,strlen(ip));
	ptr=index(cidr,'.')+1;
	ptr=index(ptr,'.')+1;
	ptr=index(ptr,'.')+1;
	*ptr='0';
	*(ptr+1)='\0';
	ptr=index(rev,'.')+1;

	if(chkd_ptr==already_checked+MAXCHECKED-1)
		chkd_ptr=already_checked;
	else{
		for(n=0;already_checked[n];n++)
			if(!strncasecmp(already_checked[n],ptr,26))
				return;
		if(!*chkd_ptr)
			*chkd_ptr=(char*)MALLOC(30);
		strncpy(*chkd_ptr,ptr,26);
		++chkd_ptr;
	}

	p=ZapytajSerwer(ns_t_ns,ptr);
	if(p)
	{
		strcat(cidr,"/24");
		wypisz(AQUA " Strefa zwrotna" NORMAL " dla " BOLD "%s" NORMAL " (%s) jest utrzymywana na:\n",ptr,cidr);
		for(n=0;p->dane.RRs[n];n++)
		{
			printf("   %s ",p->dane.RRs[n]);
			if(!isip(p->dane.RRs[n])){
				host1=rozwiaz_hostname(p->dane.RRs[n]);
				if(!host1){
					printf("(nie wskazuje na ¿aden adres)\n");
					continue;
				}
				printf("(%s",host1);
				host2=rozwiaz_ip(host1);
				free(host1);
				if(!host2){
					printf(" [brak wpisu])\n");
					continue;
				}
				if(strcasecmp(p->dane.RRs[n],host2)){
					printf(" [%s])\n",host2);
					printf("      [Niespójne mapowanie zwrotne]\n");
				}else
					printf(")\n");
				free(host2);
			}else{
				host1=rozwiaz_ip(p->dane.RRs[n]);
				if(!host1){
					printf("[brak wpisu])\n");
					continue;
				}
				printf("[%s])\n",host1);
				free(host1);
			}
		}
		return;
	}

	ptr=index(cidr,'.')+1;
	ptr=index(ptr,'.')+1;
	*ptr='\0';
	strcat(cidr,"0.0");
	ptr=index(rev,'.')+1;
	ptr=index(ptr,'.')+1;


	if(chkd_ptr==already_checked+MAXCHECKED-1)
		chkd_ptr=already_checked;
	else{
		for(n=0;already_checked[n];n++)
			if(!strncasecmp(already_checked[n],ptr,22))
				return;
		if(!*chkd_ptr)
			*chkd_ptr=(char*)MALLOC(30);
		strncpy(*chkd_ptr,ptr,22);
		++chkd_ptr;
	}


	p=ZapytajSerwer(ns_t_ns,ptr);
	if(p)
	{
		strcat(cidr,"/16");
		wypisz(AQUA " Strefa zwrotna" NORMAL " dla " BOLD "%s" NORMAL " (%s) jest utrzymywana na:\n",ptr,cidr);
		for(n=0;p->dane.RRs[n];n++)
			printf("   %s\n",p->dane.RRs[n]);
		free(p);
		return;
	}

	ptr=index(cidr,'.')+1;
	*ptr='\0';
	strcat(cidr,"0.0.0");
	ptr=index(rev,'.')+1;
	ptr=index(ptr,'.')+1;
	ptr=index(ptr,'.')+1;

	if(chkd_ptr==already_checked+MAXCHECKED-1)
		chkd_ptr=already_checked;
	else{
		for(n=0;already_checked[n];n++)
			if(!strncasecmp(already_checked[n],ptr,22))
				return;
		if(!*chkd_ptr)
			*chkd_ptr=(char*)MALLOC(30);
		strncpy(*chkd_ptr,ptr,22);
		++chkd_ptr;
	}


	p=ZapytajSerwer(ns_t_ns,ptr);
	if(p)
	{
		strcat(cidr,"/8");
		wypisz(AQUA " Strefa zwrotna" NORMAL " dla " BOLD "%s" NORMAL " (%s) jest utrzymywana na:\n",ptr,cidr);
		for(n=0;p->dane.RRs[n];n++)
			printf("   %s\n",p->dane.RRs[n]);
		free(p);
		return;
	}
}

void SprawdzCzyAlias(const char* host)
{
	union __odpowiedz* p;
	int n;
	char *ptr,*ptr2,*ptr3;

	if(isip(host))
		return;

	p=ZapytajSerwer(ns_t_cname,hostname);
	if(p)
	{
		wypisz(BOLD " %s" NORMAL " jest "AQUA"aliasem"NORMAL" do:\n",hostname);
		for(n=0;(ptr=p->dane.RRs[n]);n++)
			if(isip(ptr))
				wypisz(" adresu ip ("BOLD"nieprawid³owe"NORMAL"): %s\n",ptr);
			else{
				printf("   %s ",ptr);
				ptr2=rozwiaz_hostname(ptr);
				if(!ptr2){
					printf("(nie wskazuje na ¿aden adres.)\n");
					continue;
				}
				printf("(%s ",ptr2);
				ptr3=rozwiaz_ip(ptr2);
				free(ptr2);
				if(!ptr3) {
					printf("[brak wpisu.])\n");
					continue;
				}
				printf("[%s])\n",ptr3);
				if(strcasecmp(ptr3,ptr))
					printf("      [Niespójne mapowanie zwrotne]\n");
				free(ptr3);
			}
	}
	free(p);
}

void ___ops(int param)
{
	raise(SIGSEGV);
}

void ooops(int param)
{
	char *home;

	fprintf(stderr,
BLINK RED
"OOPS... Segmentation Fault!\n"
NORMAL RED
"Wyst±pi³ b³±d segmentacji pamiêci. Ten program jest jeszcze\n"
"we wczesnej wersji rozwojowej i je¶li chcesz pomóc, to wy¶lij\n"
"informacjê o b³êdzie na adres:\n"
YELLOW EMAIL RED
"\nNapisz, jakie dane wej¶ciowe doprowadzi³y do takiego stanu.\n\n" NORMAL);
	home=getenv("HOME");
	fprintf(stderr,"Zrzucanie rdzenia w ");
	if(home){
		fprintf(stderr,"%s...\n",home);
		chdir(home);
	}else
		fprintf(stderr,"bierz±cym katalogu...\n");
	signal(SIGSEGV,SIG_DFL);
	signal(SIGALRM,___ops);
	alarm(1);
}

void LiczCIDR(const char* param)
{ // ( Bezklasowy Routing Miêdzydomenowy )
	char *cidr,*adres,*bitstr,*tmpptr;
	int bity,adr[4];
	unsigned long int binIP,binIP2,binBity;
	int n;

	if(!param)
		return;
	adres=cidr=(char*)MALLOC(strlen(param)+1);
	strcpy(cidr,param);
	bitstr=index(cidr,'/');
	if(!bitstr || !*(bitstr+1)){
		free(cidr);
		fprintf(stderr,"Prawid³owy format to: <Adres IP>/<0-32>\n");
		exit(EXIT_FAILURE);
	}
	*bitstr++='\0';
	for(n=0,tmpptr=bitstr;*tmpptr;tmpptr++)
		if(!isdigit(*tmpptr)){
			fprintf(stderr,"Prawid³owy format to: <Adres IP>/<0-32>\n");
			exit(EXIT_FAILURE);
		}
	bity=atol(bitstr);
	if(!isip(adres) || 0>bity || bity>32){
		fprintf(stderr,"Prawid³owy format to: <Adres IP>/<0-32>\n");
		exit(EXIT_FAILURE);
	}

	for(n=0,tmpptr=adres;n<4 && tmpptr!=(char*)0x1;tmpptr=index(tmpptr,'.')+1,n++)
		adr[n]=atol(tmpptr);
	for(binIP=0,n=3;n>=0;n--)
		binIP+=adr[n]<<(8*(3-n));

	binBity=0;
	for(n=1;n<=32;n++){
		binBity<<=1;
		binBity+=n<=bity?1:0;
	}

	binIP&=binBity;

	binBity=0;
	for(n=1;n<=32-bity;n++){
		binBity<<=1;
		binBity++;
	}

	binIP2=binIP+binBity;

	wypisz(BOLD " %s" NORMAL AQUA "/" NORMAL BOLD "%s" NORMAL " to przedzia³ adresów:\n",adres,bitstr);
	wypisz(AQUA "    Od:" NORMAL " %d.%d.%d.%d       ",	(int)(binIP>>24)&0xFF,
																			(int)(binIP>>16)&0xFF,
																			(int)(binIP>>8)&0xFF,
																			(int)binIP&0xFF);
	wypisz(AQUA "Do: " NORMAL "%d.%d.%d.%d\n",				(int)(binIP2>>24)&0xFF,
																			(int)(binIP2>>16)&0xFF,
																			(int)(binIP2>>8)&0xFF,
																			(int)binIP2&0xFF);


	exit(EXIT_SUCCESS);

}

void SprawdzWersjeNameservera(const char* nameserver)
{
	union u
	{
		HEADER h;
#ifndef MAXPACKET
#define MAXPACKET 8192
#endif
		u_char buf[MAXPACKET];
	} qwbuf,ansbuf;
	u_char *ptr,*koniec;
	int  qs,n;
	char junk[13];
	char *ns;
	int saved_nscount=_res.nscount;
	struct sockaddr_in saved_nss[_res.nscount];

	if(!nameserver || !*nameserver)
		return;

	if(!isip(nameserver))
	{
		ns=rozwiaz_hostname(nameserver);
		if(!ns || !*ns){
			fprintf(stderr,"%s nie wskazuje na ¿aden adres.\n",nameserver);
			exit(EXIT_FAILURE);
		}
	}else
		ns=(char*)nameserver;

	for(n=0;n<_res.nscount;n++)
		saved_nss[n]=_res.nsaddr_list[n];

	_res.nscount=1;
	_res.nsaddr.sin_addr.s_addr=inet_addr(ns);


	n=res_mkquery(QUERY,"version.bind",C_CHAOS,ns_t_txt,NULL,0,NULL,qwbuf.buf,sizeof(qwbuf));
	if(n<0)
		BLAD("res_mkquery()");

	n=res_send(qwbuf.buf,n,ansbuf.buf,sizeof(ansbuf));
	if(n<0)
		BLAD("res_send()");

	koniec=ansbuf.buf+n;
	qs=ntohs(ansbuf.h.qdcount);

	ptr=ansbuf.buf+HFIXEDSZ;
	while(qs-->0)
	{
		n=dn_skipname(ptr,koniec);
		if(n<0)
			BLAD("dn_skipname()");
		ptr+=n+QFIXEDSZ;
	}

	n=dn_expand(ansbuf.buf,koniec,ptr,junk,13);
	ptr += n;
	ptr += INT32SZ + INT32SZ;
	ptr+=3;

	_res.nscount=saved_nscount;
	for(n=0;n<_res.nscount;n++)
		_res.nsaddr_list[n]=saved_nss[n];

	if(!*ptr)
	{
		fprintf(stderr,"B³±d: Nie mo¿na by³o ustaliæ wersji na zdalnym serwerze.\n");
		exit(EXIT_FAILURE);
	}

	wypisz(AQUA "Wersja: "NORMAL "%s\n",ptr);

	return;
}

void help(void)
{
	fprintf(stderr,
"%s v%s (%s)
U¿ycie:
	%s < adres ip | nazwa hosta >

Opcje:
	-h       Wy¶wietla tê informacjê.
	-c       Traktuje argument jako zapis CIDR i okre¶la przedzia³
	         adresów, do których zapis pasuje.
	-v       Sprawdza wersjê zdalnego serwera nazw. (tylko named)
	-V       Wy¶wietla wersjê programu.\n",PROGRAMNAME,PROGVERION,EMAIL,PROGRAMNAME);
	return;
}





void ParsujParamentry(int argc,char** argv)
{
	int c;

	while( (c=getopt(argc,argv,":hVv:c:")) != -1 )
		switch(c)
		{
			case ':':
				fprintf(stderr,"Brakuj±cy parametr dla opcji -%c\n",optopt);
				exit(EXIT_FAILURE);
				break;
			case '?':
				fprintf(stderr,"Nieznana opcja: -%c\n",optopt);
				fprintf(stderr,"Wpisz %s -h , aby zobaczyæ dostêpne opcje.\n",PROGRAMNAME);
				exit(EXIT_FAILURE);
			case 'h':
				help();
				exit(EXIT_FAILURE);
			case 'V':
				printf("%s v%s (%s)\n",PROGRAMNAME,PROGVERION,EMAIL);
				exit(EXIT_SUCCESS);
				break;
			case 'v':
				wypisz("Sprawdzanie wersji serwera nazw na: "BOLD "%s" NORMAL "\n",optarg);
				SprawdzWersjeNameservera(optarg);
				exit(EXIT_SUCCESS);
				break;
			case 'c':
				LiczCIDR(optarg);
				exit(EXIT_SUCCESS);
				break;
		}

	--optind;
	if( argv[optind] && argv[optind][0]=='-' && argv[optind][1]=='-' )
	{
		help();
		exit(EXIT_SUCCESS);
	}

	return;
}



void SprawdzNadrzedneNameservery(const char* hostname, const char* ip)
{
	char *domena,*ptr;

	if( !hostname || !*hostname )
		return;

	domena=(char*)hostname;
	while( (domena=index(domena,'.')) )
	{
		domena++;
		ptr=index(domena,'.');
		if(ptr && *(ptr+1)!='\0')   // jest domena nadrzêdna
			WypiszNameservery(domena);
		else{
			return;
			// Mo¿e sprawdziæ w bazie WHOIS ?
		}
	}
}

void JakaKlasa(const char* ip)
{
	unsigned char bajt;
	char* klasa=(char*)NULL;
	int liczba,liczba2;
	int klasa_prywatna=0;

	if( !ip || !*ip )
		return;

	liczba=atoi(ip);
	liczba2=atoi(index(ip,'.')+1);
	if( liczba < 0 || liczba > 255 || liczba2 < 0 || liczba2 > 255 )
		return;
	bajt=(unsigned char)liczba;
	if( ! (bajt&128) )	// 0xxxxxxx
		klasa="A";
	else if( bajt>>6 == 2 )	// 10xxxxxx
		klasa="B";
	else if( bajt>>5 == 6 )	// 110xxxxx
		klasa="C";
	else if( bajt>>4 == 14 )	// 1110xxxx
		klasa="multicast";
	else if( bajt>>4 == 15 )	// 1111xxxx
		klasa="zarezerwowanej (?)";
	if( liczba == 10 )
		klasa_prywatna=1;
	else if( liczba == 172 && liczba2 == 16 )
		klasa_prywatna=1;
	else if( liczba == 192 && liczba2 == 168 )
		klasa_prywatna=1;

	if( klasa_prywatna )
		wypisz(" Adres %s w " AQUA "klasie prywatnej" NORMAL ": " GREEN "%s" NORMAL "\n",ip,klasa);
	else
		wypisz(" Adres %s w " AQUA "klasie publicznej" NORMAL ": " GREEN "%s" NORMAL "\n",ip,klasa);

	return;
}

char Wybierz(const char* mozliwosci,const char* pytanie, ...)
{
	struct termios tios;
	char wybor;
	va_list ap;

	if( tcgetattr(1,&tios) )
		exit(EXIT_FAILURE);

	tios.c_lflag &= ~ICANON;
	tios.c_lflag &= ~ECHO;
	if( tcsetattr(0,TCSANOW,&tios) )
		exit(EXIT_FAILURE);
	va_start(ap,pytanie);
	vprintf(pytanie,ap);
	va_end(ap);
	do
		wybor=getc(stdin);
	while( !index(mozliwosci,wybor) );

	tios.c_lflag |= ICANON;
	tios.c_lflag |= ECHO;
	if( tcsetattr(0,TCSANOW,&tios) )
		exit(EXIT_FAILURE);

	return wybor;

}

void PokazWhois(const char *hostname,const char *ip)
{
	int n,wybor;
	char *komenda,*domena;

// WHOIS dla adresów IP
	if( ! interactive || Wybierz("tn","Pokazaæ informacje z bazy WHOIS dla adresów ? [tn]\n") == 't' )
		{
			komenda=(char*)MALLOC(25);
			if( adresy )
				for( n=0 ; adresy[n] ; ++n )
				{
					wypisz("\nInformacje WHOIS dla adresu %s :\n\n",adresy[n]);
					fflush(stdout);
					strcpy(komenda,"whois -H ");
					strcat(komenda,adresy[n]);
					system(komenda);
				}
			else
			{
				wypisz("\nInformacje WHOIS dla adresu %s :\n\n",ip);
				fflush(stdout);
				strcpy(komenda,"whois -H ");
				strcat(komenda,ip);
				system(komenda);
			}
			free(komenda);
		}

	if( !hostname || !*hostname )
		return;

// WHOIS dla domeny
	domena = (char*)hostname+strlen(hostname)-1;
	while( domena>hostname && *domena != '.' )
		--domena;
	if( domena > hostname )
		--domena;
	while( domena>hostname && *domena != '.' )
		--domena;
	if( *domena == '.' )
		++domena;

	if( interactive )
		wybor=Wybierz("tn","Pokazaæ infromacje WHOIS dla domeny %s ? [tn]\n",domena);
	else
	{
		wypisz("Informacje WHOIS dla domeny %s:\n",domena);
		fflush(stdout);
	}

	komenda=(char*)MALLOC(80);
	strcpy(komenda,"whois -H ");
	strncat(komenda,domena,70);
	if( ! interactive || wybor == 't' )
		system(komenda);

	if( domena-2 >=hostname )
		domena-=2;
	else
	{
		free(komenda);
		return;
	}
	while( domena>hostname && *domena != '.' )
		--domena;

	if( interactive )
	{
		wybor=Wybierz("tn","Pokazaæ infromacje WHOIS dla domeny %s ? [tn]\n",domena);
		if( wybor == 'n' )
		{
			free(komenda);
			return;
		}
	}
	else
	{
		wypisz("Informacje WHOIS dla domeny %s:\n",domena);
		fflush(stdout);
	}


	strcpy(komenda,"whois -H ");
	strncat(komenda,domena,70);
	system(komenda);
	free(komenda);

	return;
}

int main(int argc,char** argv,char** envp)
{
	struct in_addr adr;
	struct hostent* hn;
	char *str_addr;
	int its_ip,n;

	if(argc<2)
		usage();

	res_init();
	signal(SIGSEGV,ooops);
	interactive = isatty(fileno(stdout)) ? 1 : 0;

	ParsujParamentry(argc,argv);

	baner();
	wypisz("Sprawdzanie: " BOLD "%s" NORMAL "\n\n",argv[1]);

	if(isip(argv[1])){
		ip=argv[1];
		hostname=rozwiaz_ip(argv[1]);
	}else{
		ip=rozwiaz_hostname(argv[1]);
		if(!ip){
			printf("Domena %s nie wskazuje na ¿aden adres.\n",argv[1]);
			exit(EXIT_FAILURE);
		}
		hostname=argv[1];
	}
	if( hostname && *hostname && hostname[strlen(hostname)-1]=='.' )
		hostname[strlen(hostname)-1]='\0';


	wypisz(AQUA " Nazwa domenowa:" NORMAL " %s\n",hostname?hostname:"Brak wpisu.");
	if(hostname){
		wypisz(AQUA "  TLD:" NORMAL " %s\n",CheckTld(hostname));
		SprawdzCzyAlias(hostname);
	}

	WypiszAdresy(hostname,ip);
	JakaKlasa(ip);
	WypiszAliasy(hostname,ip);
	WypiszMXy(hostname,ip);
	WypiszNameservery(hostname);
	SprawdzNadrzedneNameservery(hostname,ip);

	if(adresy)
		for(n=0;adresy[n];n++)
			SprawdzRev(adresy[n]);
	else
		SprawdzRev(ip);


	PokazWhois(hostname,ip);


	exit(EXIT_SUCCESS);


// Tego co poni¿ej ju¿ nie u¿ywamy

	if((str_addr=rozwiaz_ip(argv[1])))
	{
		printf("  Nazwa domenowa: %s\n",str_addr);
		its_ip=1;
		inet_aton(argv[1],&adr);
		hn=gethostbyaddr((char*)&adr,4,AF_INET);
	}
	else
		if((str_addr=rozwiaz_hostname(argv[1])))
		{
			hn=gethostbyname2(argv[1],AF_INET);
			its_ip=0;
			if(hn->h_addr_list[1]){
				printf("  Adresy IP:\n");
				for(n=0;hn->h_addr_list[n];n++)
					printf("    %s\n",inet_ntoa(*(struct in_addr*)(hn->h_addr_list[n])));
			}else
				printf("  Adres IP: %s\n",str_addr);
		}
		else
		{
			fprintf(stderr,"Nie uda³o siê rozwi±zaæ %s ani jako IP, ani jako nazwê hosta.\n",argv[1]);
			exit(EXIT_FAILURE);
		}

	if(hn->h_aliases[0])
		printf("  Aliasy:\n");

	for(n=0;hn->h_aliases[n];n++)
		printf("    %s\n",hn->h_aliases[n]);

	printf("\n");

	srand(time(NULL));

	printf("%d\n",(int)(254.0*(rand()+1.0)/RAND_MAX));


	exit(EXIT_SUCCESS);

}

/*



Tak to mia³o wygl±daæ w za³o¿eniach:



	Sprawdzanie: fjaksfajkfajsfdk
	Nazwa domenowa: host.domena1.domena2.tld
	(.tld) Kraj / Organizacja: np. Polska etc.

	Adres: fasfsfasfasfasf
		lub
	Adresy:
		432.42.52.532
		532.5.52.523
		523.52.2.5
	Aliasy:
		fasfasdf
		faasdfasf

	Poczta dla host.domena1.domena2.tld utrzymywana na:
		42.523.5.523 (fasfasdfa.sfdasfdasfd)

	Zapisaæ raport w ~/fasfadraport ? [tn]

	Strefa zwrotna dla `tu adresy` utrzymywana na:
		324.235.23.52 (fasdfasfd)
		35.52.52.52 (fasdfasfds)

	host.domena1.domena2.tld utrzywana na:
		242.412.4235.52 (faas.fasf.asfasfd)
		5.325.23.52 (fasfasfadf.dafadf)

	domena1.domena2.tld utrzywana na:
		242.412.4235.52 (faas.fasf.asfasfd)
		5.325.23.52 (fasfasfadf.dafadf)

	Sprawdziæ dane z bazy whois dla domeny i adresów IP? [tn]

	whois domena2.tld:

	whois `adresy`:

	Sprawdziæ strefê zwrotn± dla `adresy` ? [tn]

	Wpisy w strefie zwrotnej dla 5 s±siaduj±cych adresów:
	 2423.421.4124.432 (fsadfasfdasfd)
	 ...

	Wpisy w strefie zwrotnej dla 3 pierwszych i ostatnich adresów w klasie C:
	 2423.421.4124.432 (fsadfasfdasfd)
	 ...

	Wpisy w strefie zwrotnej dla 8 losowych adresów w strefie zwrotnej:
	 2423.421.4124.432 (fsadfasfdasfd)
	 ...




*/



/* vim: set sw=3 ts=3: */
