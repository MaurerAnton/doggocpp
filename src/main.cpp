#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <regex>
#include <sstream>
#include <unistd.h>

static const char* VERSION = "0.1.0";

static std::string runDrill(const std::string& domain, const std::string& qtype, const std::string& server) {
    std::string cmd = "drill -Q " + domain + " " + qtype;
    if (!server.empty()) cmd += " @" + server;
    cmd += " 2>&1";
    FILE* f = popen(cmd.c_str(), "r");
    if (!f) return "";
    char buf[4096]; std::string out;
    while (fgets(buf, sizeof(buf), f)) out += buf;
    pclose(f);
    return out;
}

static std::string colorize(const std::string& raw, bool noColor) {
    if (noColor) return raw;
    const char *N="\033[1;36m",*T="\033[1;33m",*V="\033[32m",*G="\033[90m",*R="\033[0m";
    std::string out;
    std::istringstream ss(raw); std::string line;
    std::regex reAns("^\\s*(\\S+\\.?)\\t+(\\d+)\\t+IN\\t+(\\S+)\\t+(.+)$");
    while (std::getline(ss,line)) {
        if (line.back()=='\r') line.pop_back();
        std::smatch m;
        if (std::regex_match(line,m,reAns) && m.size()>=5) {
            out += std::string(N)+m[1].str()+R+"  ";
            out += std::string(T)+m[3].str()+R+"  ";
            out += std::string(G)+m[2].str()+R+"  ";
            out += std::string(V)+m[4].str()+R+"\n";
        }
    }
    return out;
}

int main(int argc, char* argv[]) {
    std::string domain, qtype="A", server; bool json=false, ver=false;
    for (int i=1;i<argc;i++) {
        std::string a=argv[i];
        if (a=="-v"||a=="--version") ver=true;
        else if (a=="-J"||a=="--json") json=true;
        else if ((a=="-t"||a=="--type")&&i+1<argc) qtype=argv[++i];
        else if ((a=="-n"||a=="--server")&&i+1<argc) server=argv[++i];
        else if (a[0]!='-') domain=a;
        else {fprintf(stderr,"doggocpp: %s [-t TYPE] [-n SERVER] [-J] DOMAIN\n",argv[0]);return 1;}
    }
    if (ver) {printf("doggocpp %s\n",VERSION);return 0;}
    if (domain.empty()) {fprintf(stderr,"Usage: doggocpp DOMAIN\n");return 1;}
    for (auto& c:qtype) c=toupper(c);

    auto raw = runDrill(domain, qtype, server);
    if (json) {
        std::string out="{\"answers\":["; bool first=true;
        std::istringstream ss(raw); std::string line;
        std::regex re("^\\s*(\\S+\\.?)\\t+\\d+\\t+IN\\t+\\S+\\t+(.+)$");
        while (std::getline(ss,line)) {
            std::smatch m;
            if (std::regex_match(line,m,re)&&m.size()>=3) {
                if (!first) out+=",";
                out+="{\"name\":\""+m[1].str()+"\",\"data\":\""+m[2].str()+"\"}";
                first=false;
            }
        }
        out+="]}\n"; printf("%s",out.c_str()); return 0;
    }

    printf("\033[1;36m%-30s %-5s %-8s %s\033[0m\n","NAME","TYPE","TTL","VALUE");
    printf("\033[90m──────────────────────────────────────────────────\033[0m\n");
    printf("%s", colorize(raw, !isatty(STDOUT_FILENO)).c_str());
    return 0;
}
