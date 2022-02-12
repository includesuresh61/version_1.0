#include<string>
string random_file_generator(string pcap_file){
	char a[10];
	time_t seed;
	srand(time(&seed));
	int Rnum=(rand()%10000 +1);
	sprintf(a,"%d.wav",Rnum);
	a[strlen(a)]='\0';
	int pos =pcap_file.find(".");
	string file=pcap_file.substr(0,pos);
	return (file.append(a));
}






