#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

static const char* VERSION = "0.1.0";

struct Color { const char *N="\033[1;36m",*T="\033[1;33m",*V="\033[32m",*G="\033[90m",*R="\033[0m"; bool off=false; };

static int typeNum(const std::string& t) {
    if(t=="A")return 1; if(t=="NS")return 2; if(t=="CNAME")return 5;
    if(t=="SOA")return 6; if(t=="MX")return 15; if(t=="TXT")return 16;
    if(t=="AAAA")return 28; return 1;
}
static const char* typeName(int t){switch(t){case 1:return"A";case 2:return"NS";case 5:return"CNAME";case 6:return"SOA";case 15:return"MX";case 16:return"TXT";case 28:return"AAAA";default:return"?";}}

static std::string encodeName(const std::string& name) {
    std::string out; size_t s=0;
    while(s<name.size()){size_t d=name.find('.',s);std::string l=(d==std::string::npos)?name.substr(s):name.substr(s,d-s);out+=(char)l.size();out+=l;if(d==std::string::npos)break;s=d+1;}
    out+='\0'; return out;
}

static std::pair<std::string,int> decodeName(const uint8_t* buf, int len, int off) {
    std::string name; bool jumped=false; int orig=off, jumps=0;
    while(jumps<20){if(off>=len)break;uint8_t b=buf[off];if(b==0){off++;break;}if((b&0xC0)==0xC0){if(off+1>=len)break;uint16_t p=((b&0x3F)<<8)|buf[off+1];if(!jumped)orig=off+2;off=p;jumped=true;jumps++;continue;}off++;if(off+b>len)break;if(!name.empty())name+='.';name+=std::string((const char*)buf+off,b);off+=b;}
    if(!jumped)orig=off; return {name,orig};
}

struct Record { std::string name,data; int type,ttl; };

static std::vector<Record> query(const std::string& domain, int qt, const std::string& server) {
    std::vector<Record> r;
    uint8_t pkt[512]={}; int pos=12;
    pkt[0]=0x12;pkt[1]=0x34; pkt[2]=0x01;pkt[3]=0x00; pkt[4]=0x00;pkt[5]=0x01; /* QD=1 */
    auto enc=encodeName(domain); memcpy(pkt+pos,enc.data(),enc.size());pos+=enc.size();
    pkt[pos++]=qt>>8; pkt[pos++]=qt&0xFF; pkt[pos++]=0; pkt[pos++]=1; /* QCLASS IN */

    struct sockaddr_in a={}; a.sin_family=AF_INET; a.sin_port=htons(53);
    std::string ns=server.empty()?"8.8.8.8":server;
    if(inet_pton(AF_INET,ns.c_str(),&a.sin_addr)!=1){struct hostent*h=gethostbyname(ns.c_str());if(!h)return r;memcpy(&a.sin_addr,h->h_addr,4);}

    int s=socket(AF_INET,SOCK_DGRAM,0); if(s<0)return r;
    struct timeval tv={3,0};setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));
    sendto(s,pkt,pos,0,(struct sockaddr*)&a,sizeof(a));

    uint8_t resp[4096]; socklen_t al=sizeof(a);
    int n=recvfrom(s,resp,sizeof(resp),0,(struct sockaddr*)&a,&al);close(s);
    if(n<12)return r;

    int qd=(resp[4]<<8)|resp[5], an=(resp[6]<<8)|resp[7]; pos=12;
    for(int i=0;i<qd;i++){auto[d,np]=decodeName(resp,n,pos);pos=np+4;}

    for(int i=0;i<an;i++){
        auto[d,p1]=decodeName(resp,n,pos); if(p1+10>n)break;
        int rt=(resp[p1]<<8)|resp[p1+1], ttl=(resp[p1+4]<<24)|(resp[p1+5]<<16)|(resp[p1+6]<<8)|resp[p1+7], rdl=(resp[p1+8]<<8)|resp[p1+9];
        pos=p1+10; std::string data;
        switch(rt){
            case 1:{char ip[16];snprintf(ip,16,"%d.%d.%d.%d",resp[pos],resp[pos+1],resp[pos+2],resp[pos+3]);data=ip;break;}
            case 28:{char ip[46];inet_ntop(AF_INET6,resp+pos,ip,46);data=ip;break;}
            case 2:case 5:{auto[nm,_]=decodeName(resp,n,pos);data=nm;break;}
            case 15:{int pr=(resp[pos]<<8)|resp[pos+1];auto[nm,_]=decodeName(resp,n,pos+2);data=std::to_string(pr)+" "+nm;break;}
            case 16:{int tl=resp[pos];data="\""+std::string((const char*)resp+pos+1,tl)+"\"";break;}
            case 6:{auto[m,_]=decodeName(resp,n,pos);auto[rn,__]=decodeName(resp,n,_);data=m+" "+rn;break;}
            default:data="["+std::to_string(rdl)+" bytes]";
        }
        pos+=rdl; r.push_back({d,data,rt,ttl});
    }
    return r;
}

int main(int argc, char* argv[]) {
    std::string domain, qtype="A", server; bool json=false, ver=false;
    for(int i=1;i<argc;i++){std::string a=argv[i];
        if(a=="-v"||a=="--version")ver=true; else if(a=="-J"||a=="--json")json=true;
        else if((a=="-t"||a=="--type")&&i+1<argc)qtype=argv[++i];
        else if((a=="-n"||a=="--server")&&i+1<argc)server=argv[++i];
        else if(a=="--no-color"){}
        else if(a[0]!='-')domain=a;
        else{fprintf(stderr,"doggocpp [-t TYPE] [-n SERVER] [-J] DOMAIN\n");return 1;}
    }
    if(ver){printf("doggocpp %s\n",VERSION);return 0;}
    if(domain.empty()){fprintf(stderr,"Usage: doggocpp DOMAIN\n");return 1;}
    for(auto& c:qtype)c=toupper(c);

    auto results=query(domain,typeNum(qtype),server);
    Color C; C.off=!isatty(STDOUT_FILENO);

    if(json){printf("{\"answers\":[");for(size_t i=0;i<results.size();i++){auto&r=results[i];
        printf("%s{\"name\":\"%s\",\"type\":\"%s\",\"ttl\":%d,\"data\":\"%s\"}",i?",":"",r.name.c_str(),typeName(r.type),r.ttl,r.data.c_str());}
        printf("]}\n");return 0;}

    printf("%s%-30s %-5s %-8s %s%s\n",C.N,"NAME","TYPE","TTL","VALUE",C.R);
    printf("%s──────────────────────────────────────────────────%s\n",C.G,C.R);
    for(auto&r:results) printf("%s%-30s%s %s%-5s%s %s%-8d%s %s%s%s\n",
        C.N,r.name.c_str(),C.R, C.T,typeName(r.type),C.R, C.G,r.ttl,C.R, C.V,r.data.c_str(),C.R);
    return 0;
}
