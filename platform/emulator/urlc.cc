/* -----------------------------------------------------------------------
 *  (c) Perdio Project, DFKI & SICS & UCL
 *  Author: tf
 *
 * -----------------------------------------------------------------------*/

/* urlc.cc
   declaration and definitions for the URL client class.
   definition for the openUrl function.
   */

/*
 * mm: modifications
 *  - passwd always user@ (don't use resolv)
 *  - don't use exception handling
 *  - bug fixed: ftp://scheidr@ftp/pub
 */


/* int openUrl(const char*); 
   int localizeUrl(const char* Url, char** fnp); 
   defined at the end
   */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/uio.h>
#include <sys/utsname.h>


/* defines */

#ifndef URL_CLIENT
  #include  "urlc.hh"
#endif

// #define TF_DEBUG 2

#ifndef TF_DEBUG
#define TF_PERROR(s)
#else
#if 1 <= TF_DEBUG
#define TF_PERROR(s) perror(s)
#else 
#define TF_PERROR(s)
#endif
#endif

#ifndef TF_DEBUG
#define TF_MSG(s) 
#else
#if 2 <= TF_DEBUG
/* messages to stderr */
#define TF_MSG(s) fprintf(stderr, "%s:%d: %s\n", \
			  __FILE__, __LINE__, (s))
#else
#define TF_MSG(s)
#endif
#endif

  /*
   * mm: don't use try/catch/throw
   */
#define throw2 {ret = 2; goto bomb; }
#define throw3 {ret = 3; goto bomb; }
#define throw4 {ret = 4; goto bomb; }
#define throw5 {ret = 5; goto bomb; }
#define throw6 {ret = 6; goto bomb; }
#define throw9 {ret = 9; goto bomb; }

#define TF_BUFLEN     1024 /* BUFSIZ may be too much */
#define URLC_REDTIMES    5 /* maximum number of redirection (RFC) */

/* ## define to 1 if you want to keep the temporary file */
#define TF_KEEP_TEMP 0

/* ## HTTP user agent reported to the server */
#define HTTP_USER_AGENT "tf_client/2.0"

/* ## distant socket read. keeps changes in a single place */
int tf_read_from_socket(int sockfd, char* buf, int len)
{
    int n = 0;
    n = read(sockfd, buf, len);
    return (n);
}


/* --------------------- class declaration ------------------------ */
/* actually, I do not like to put declaration in a .cc file, but this 
   is to keep all in a single file.
   see "Extending" comment at the end.  search for "##" for sensitive
   points.

   accepted URLs:
   http://<host>:<port>/<path>?<searchpart>
   ftp://<user>:<password>@<host>:<port>/<cwd1>/.../<cwdN>/<name>;type={a,i,d}
   file:/<dir1>/.../<dirN>/<name>
   */
class urlc
{
private:
    /* data section ## */
    char* proto;             // transfer protocol (one of URLC_kp)
    char* host;              // destination host
    char* user;              // user name
    char* pass;              // password 
    char  ftp_type;          // FTP transfer type
    unsigned short port;     // connection port 
    char* path;              // path to retrieve
    int ofd;                 // file id to return
    int sockfd;              
    char* temp_file_name;      // keeps a temporary file name
    int unlink_mode;         // != 0 to unlink the filename
    struct sockaddr_in lin;  // local in address
    int ftp_header_stat;     // last ftp reply status
    int ftp_last_reply;      // last reply code from the ftp server
    int ftp_stat;            // ftp state diagram marker
    int http_header_stat;    // last http status report
    char* http_redirect;     // points to the URL returned by an HTTP redirect

    /* annoying and support functions */
    int clean(void); // cleans the data fields
    int tmp_file_open(int mode); // opens a temporary file 
    int tcpip_open(const char* host, int port); // opens connection
    int writen(int sockfd, char* buf, int n); // write n bytes to socket
    int write3(int sockfd, const char* p1, int lp1, // writev limited clone
	       const char* p2, int lp2, const char* p3, int lp3);
    int descape(char* s); // de-escape the string (modifies it!)
    int http_req(void); // sends HTTP request
    int http_header_interp(char* line, int linenr);
    int http_get_header(char* buf, int* brem, int& n);
    int ftp_header_interp(char* line);
    int ftp_get_reply(char* buf, int* blen);

    /* ## internal interface for extension */
    int parse_file(const char* line);
    int get_file(void);

    int parse_http(const char* line);
    int get_http(void);

    int parse_ftp(const char* line);
    int anon_pass();
    int get_ftp(void);

    int parse(const char* line);
    int getURL(const char* line, int unlink_m);

public:
    urlc(void);
    ~urlc(void);
    int openURL(const char* Url);
    int localizeURL(const char* Url, char** fnp);
};


/* known protocols */
/* ##extension */
static char* URLC_kp[] = {"http://", "file:", "ftp://", NULL};
/* I love languages capable of true & full token pasting! */

/* characters not to be escaped in HTTP requests.
   if I well understood RFC 1945! ##
   */
static char URLC_hs[] = "%:@&=+$-_.!*'(),;/?";



/* -------------------- class definition ---------------------------*/

urlc::urlc(void)
{
    proto = NULL;
    host = NULL;
    user = NULL;
    pass = NULL;
    temp_file_name = NULL;
    unlink_mode = URLC_OK;
    ftp_type = 'I';
    ftp_header_stat = URLC_OK;
    port = 0;
    path = NULL;
    http_header_stat = URLC_OK;
    http_redirect = NULL;
}


int
urlc::clean(void)
{
    if(NULL != proto) {
	free(proto);
	proto = NULL;
    }
    if(NULL != host) {
	free(host);
	host = NULL;
    }
    if(NULL != user) {
	free(user);
	user = NULL;
    }
    if(NULL != pass) {
	free(pass);
	pass = NULL;
    }
    unlink_mode = URLC_OK;
    ftp_type = 'I';
    ftp_header_stat = URLC_OK;
    port = 0;
    if(NULL != path) {
	free(path);
	path = NULL;
    }
    http_header_stat = URLC_OK;
    if(NULL != http_redirect) {
	free(http_redirect);
	http_redirect = NULL;
    }
    return (0);
}


urlc::~urlc(void)
{
    clean();
}


/* opens a temporary file.
   if mode == URLC_UNLINK => remove the filename else 
   put name in temp_file_name.
   returns file descriptor number or error reason
   */
int 
urlc::tmp_file_open(int mode)
{
    int ofd = -1;
    char tn[L_tmpnam] = ""; // I like POSIX!
    if(NULL == tmpnam(tn)) { 
	TF_PERROR("tmpnam");
	return (URLC_EFILE);
    }
    do {
	errno = 0;
	ofd = open(tn, O_RDWR | O_CREAT | O_EXCL, 
		   S_IRUSR | S_IWUSR); // data destination
	if((-1 == ofd) && (EINTR == errno))
	    continue;
	if(0 < ofd)
	    break;
	TF_PERROR("open");
	return (URLC_EFILE);
    } while(1);
    if(URLC_UNLINK == mode) {
	if(-1 == unlink(tn)) {
	    TF_PERROR("unlink");
	    return (URLC_EFILE);
	}
    }
    else {
	if(NULL != temp_file_name)
	    free(temp_file_name);
	temp_file_name = (char*)malloc(1 + strlen(tn));
	if(NULL == temp_file_name)
	    return (URLC_EALLOC);
	strcpy(temp_file_name, tn);
    }

    return (ofd);
}


/* opens a TCP connection over IPv4.
   returns socket file descriptor or failure reason.
   standard (see Stevens). non-blocking.
   returns URLC_OK or reason.
   */
int
urlc::tcpip_open(const char* h, int p)
{
    struct sockaddr_in serv_addr;
    struct hostent* serv_hostent;
    int fd;

    if(NULL == (serv_hostent = gethostbyname(h))) {
	TF_PERROR("gethostbyname");
	return (URLC_ESOCK);
    }
    memset((char*)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = PF_INET;
    serv_addr.sin_addr.s_addr =
	inet_addr((serv_hostent->h_addr_list)[0]);
    serv_addr.sin_port = htons(p);
    memcpy((char*)&serv_addr.sin_addr, 
	   serv_hostent->h_addr_list[0], 
	   serv_hostent->h_length);
    fd = socket(PF_INET, SOCK_STREAM, 0);
    if(0 > fd) {
	TF_PERROR("socket");
	return (URLC_ESOCK);
    }
    while(-1 == connect(fd, (struct sockaddr*) &serv_addr, 
			sizeof(serv_addr))) {
	if(EINTR != errno) {
	    TF_PERROR("connect");
	    return (URLC_ESOCK);
	}
    }
    if(-1 == fcntl(fd, F_SETFL, O_NONBLOCK))
	TF_PERROR("fcntl");
    
    // save local address for later use (esp. ftp PORT)
    int lin_len = sizeof(lin);
    if(-1 == getsockname(fd, (struct sockaddr*) &lin, &lin_len))
	return (URLC_ESOCK);

    // simple, n'est pas?
    return (fd);
}


/* writes n bytes to socket.
   stolen shamelessly from Stevens 'Unix Network Programming', pp. 279-280
   returns URLC_OK or reason
   */
int
urlc::writen(int sockfd, char* buf, int n)
{
    int nwritten = 0;
    int nleft = n;
    while(0 < nleft) {
	errno = 0;
	nwritten = write(sockfd, buf, nleft);
	if(0 >= nwritten) {
	    switch(errno) {
	    case EINTR:
	    case EAGAIN:
	    case EINPROGRESS: // by non-blocking connect, if done
		continue;
	    default: 
		TF_PERROR("write");
		return (URLC_ESOCK);
	    }
	}
	nleft -= nwritten;
	buf += nwritten;
    }
    return (URLC_OK);
}


/* writes to socket three pointers. cheap(?) replacement for writev.
   very useful for ftp.
   */
int
urlc::write3(int sockfd, const char* p1, int lp1, 
	     const char* p2, int lp2, const char* p3, int lp3)
{
    int len = 0;
    char* p = NULL;
    int t = 0;
    
    // compute length
    if((NULL != p1) && (0 != *p1)) 
	len += lp1;
    if((NULL != p2) && (0 != *p2)) 
	len += lp2;
    if((NULL != p3) && (0 != *p3)) 
	len += lp3;
    p = (char*)malloc(len + 1); // +1 for \0, easy printing
    if(NULL == p)
	return (URLC_EALLOC);
    memcpy(p, p1, lp1);
    memcpy(p + lp1, p2, lp2);
    memcpy(p + lp1 + lp2, p3, lp3);
    p[len] = 0;
    TF_MSG(p);
    
    t = writen(sockfd, p, len);
    free(p);
    return (t);
}


/* de-escape a %-escaped string. 
   modifies the parameter!
   returns URLC_OK or error
   */
int
urlc::descape(char* s)
{
    char* p = NULL;
    int i;
    int j;
    char* d1 = NULL;
    char* d2 = NULL;
    char he[] = "0123456789abcdef";

    // check corectness
    for(i = 0; 0 != s[i]; i++) {
	if('%' != s[i])
	    continue;
	i++;
	if((0 == s[i]) || (NULL == strchr(he, tolower(s[i]))))
	    return (URLC_EINVAL);
	i++;
	if((0 == s[i]) || (NULL == strchr(he, tolower(s[i]))))
	    return (URLC_EINVAL);
    }
    p = (char*)malloc(strlen(s) + 1);
    if(NULL == p)
	return (URLC_EALLOC);
    strcpy(p, s);
    for(i = 0, j = 0; 0 != p[i]; i++, j++) {
	if('%' != p[i]) {
	    s[j] = p[i];
	    continue;
	}
	i++;
	d1 = strchr(he, tolower(p[i]));
	i++;
	d2 = strchr(he, tolower(p[i]));
	s[j] = ((int)(d1 - he)) * 16 + ((int)(d2 - he));
    }
    s[j] = 0;
    return (URLC_OK);
}


/* takes a string as argument and tries to obtain 
   protocol name.  directs subsidiaries to fill other appropriate fields.
   returns URLC_OK on success or reason on error.
   */
int
urlc::parse(const char* line0)
{
    int i = 0;
    int j = 0;
    char* pline = NULL; // no limit on URL size, so no automatic alloc possible
    char* line = NULL;

    // sanity checks
    if((NULL == line0) || (0 == line0[0]))
	return (URLC_EEMPTY);
    pline = (char*)malloc(1+strlen(line0));
    if(NULL == pline) 
	return (URLC_EALLOC);
    strcpy(pline, line0);
    line = pline;
    while(isspace(*line))
	line++; // skip whites
    for(i = strlen(line)-1; (0 <= i) && (isspace(line[i])); i--)
	line[i] = 0; /* kill trailing spaces */


    int ret=0;
    {
	// for each known protocol
	for(i = 0; (NULL != URLC_kp[i]) && (0 != URLC_kp[i][0]); i++) {
	    // case-insensitive protocol scheme comparison
	    for(j = 0; (0 != URLC_kp[i][j]) && (0 != line[j]) && 
		    (tolower(URLC_kp[i][j]) == tolower(line[j])); j++);
	    if(0 != URLC_kp[i][j])
		continue; /* next protocol */

	    proto = (char*)malloc(1+strlen(URLC_kp[i]));
	    if(NULL == proto) 
		throw4;
	    strcpy(proto, URLC_kp[i]);
	    line += strlen(proto);
	    if(0 == strcmp("http://", URLC_kp[i])) { 
		if(URLC_OK != parse_http(line)) 
		    throw5
		else // success
		    break; 
	    }
	    if(0 == strcmp("file:", URLC_kp[i])) {
		if(URLC_OK != parse_file(line))
		    throw5
		else 
		    break;
	    }
	    if(0 == strcmp("ftp://", URLC_kp[i])) {
		if(URLC_OK != parse_ftp(line))
		    throw5
		else
		    break;
	    }
	    /* other protocols tested here */
	    /* ##extension */
	    throw5; // we slould not arrive here!
	}
	if((NULL == URLC_kp[i]) || (0 == URLC_kp[i][0])) 
	    throw5; // protocol not found
    }

bomb:
    if(NULL != pline)
	free(pline);
    switch (ret) {
    case 0:
      return (URLC_OK);
    case 4:
      return (URLC_EALLOC);
    case 5:
      if(NULL != proto) {
	free(proto);
	proto = NULL;
      }
      return (URLC_EPARSE);
    default:
      return (URLC_EUNKNOWN);
    }
}


/* tries to obtain the desired path of a "file:" URL.
   returns URLC_OK on success, reason on error.
   */
int
urlc::parse_file(const char* line)
{

    if(NULL != path) {
	free(path);
	path = NULL;
    }
    path = (char*)malloc(strlen(line)+1);
    if(NULL == path)
	return (URLC_EALLOC);
    strcpy(path, line);

    return (URLC_OK);
}


/* obtains the file descriptor for the requested file.
   it could be made simpler, but is like this to fit the scheme.
   returns URLC_OK on success, reason on error.
   */
int
urlc::get_file(void)
{
    if((NULL == path) || (0 == path[0]))
	return (URLC_EEMPTY);
    ofd = open(path, O_RDONLY);
    if(-1 == ofd) {
	TF_PERROR("open");
	return (URLC_EFILE);
    }

    return (URLC_OK);
}

int
urlc::anon_pass()
{
  // passwd: user@
  struct passwd* pp = NULL;
  pp = getpwuid(getuid()); 	    
  pass = (char*)malloc(1 + strlen(pp->pw_name) + 1); 
  if(NULL == pass) 
    return (URLC_EALLOC);
  strcpy(pass, pp->pw_name);
  strcat(pass, "@");
  return (URLC_OK);
}



/* it assumes a partial RFC1738-compliant URL in the format:
   ftp://[user[:password]@]host[/[path[;type={a,i,d}]]]
   we avoid (## deliberately) port specification.
   special character :@/; in path, user and password must 
   be % escaped _before_ given to this client
   */
int
urlc::parse_ftp(const char* line)
{
    char* p_collon = NULL; /* positions in line */
    char* p_at = NULL;
    char* p_slash = NULL;
    char* p_semi = NULL;
    char he[] = "0123456789abcdef";
    int i = 0;

    if((NULL == line) || (0 == *line))
	return (URLC_EEMPTY);

    if(NULL != path) {
	free(path);
	path = NULL;
    }
    if(NULL != host) {
	free(host);
	host = NULL;
    }
    if(NULL != user) { 
	free(user);
	user = NULL;
    }
    if(NULL != pass) {
	free(pass);
	pass = NULL;
    }
    ftp_type='I';
    port = 21;

    int ret=0;
    {
	p_collon = strchr(line, ':');
	p_at = strchr(line, '@'); 
	if(NULL != p_at) // we have a pass
	    p_slash = strchr(p_at + 1, '/');
	else
	    p_slash = strchr(line, '/'); 
	p_semi = strchr(line, ';'); // to avoid masking by pass

	// sanity checks
	if((NULL != p_collon) && (0 == p_collon[1]))
	    throw2;
	if((NULL != p_at) && (0 == p_at[1]))
	    throw2;
	if((NULL != p_semi) && (0 == p_semi[1]))
	    throw2;

	if((NULL != p_slash) && (NULL != p_semi) && (p_slash > p_semi))
	    throw2; // / after ;
	if((NULL != p_collon) && (NULL != p_semi) && (p_collon > p_semi))
	    throw2; // : after ;
	if((NULL != p_at) && (NULL != p_semi) && (p_at > p_semi))
	    throw2; // @ after ;
	if((NULL != p_slash) && (NULL != p_at) && (p_at > p_slash))
	    throw2; // @ after /
	if((NULL != p_collon) && (NULL != p_slash) && (p_collon > p_slash)) 
	    throw2; // : after /
	if((NULL != p_collon) && (NULL != p_at) && (p_collon > p_at))
	    throw2; // : after @
	if((NULL != p_collon) && (NULL == p_at))
	    throw2; // : but no @

	// space allocation
	if(NULL != p_semi) {
	    *p_semi = 0;
	    p_semi++;
	    if(p_semi != strstr(p_semi, "type="))
		return (2); // invalid type specifier 
	    p_semi += strlen("type=");
	    if((0 == p_semi[0]) || (NULL == strchr("aid", p_semi[0])))
		return (2); // no valid(?) type specifier
	    ftp_type = 'I'; // ## stupid, isn't it?
	}
	if(NULL == p_slash) { // no path specified
	    path = NULL;
	    throw2;
	}
	else {
	    *p_slash = 0;
	    p_slash++;
	    path = (char*)malloc(1 + strlen(p_slash));
	    if(NULL == path) 
		throw3;
	    strcpy(path, p_slash);
	}
	for(i = 0; 0 != path[i]; i++) {
	    if('%' != path[i])
		continue;
	    i++;
	    if((0 == path[i]) || (NULL == strchr(he, tolower(path[i]))))
		throw2;
	    i++;
	    if((0 == path[i]) || (NULL == strchr(he, tolower(path[i]))))
		throw2;
	}
	if((NULL != p_collon) && (NULL != p_at)) { // user & pass
	    *p_collon = 0;
	    p_collon++;
	    *p_at = 0;
	    p_at++;
	    user = (char*)malloc(1 + strlen(line));
	    if(NULL == user)
		throw3;
	    strcpy(user, line);
	    if(URLC_OK != descape(user))
	        throw2;
	    pass = (char*)malloc(1 + strlen(p_collon));
	    if(NULL == pass)
		throw3;
	    strcpy(pass, p_collon);
	    if(URLC_OK != descape(pass))
		throw2;
	    if(0 == *p_at) // null host?
		throw2;
	    host = (char*)malloc(1 + strlen(p_at));
	    if(NULL == host) 
		throw3;
	    strcpy(host, p_at);
	}
	if((NULL == p_collon) && (NULL != p_at)) { // just user
	    *p_at = 0;
	    p_at++;
	    user = (char*)malloc(1 + strlen(line));
	    if(NULL == user)
		throw3;
	    strcpy(user, line); 
	    if(URLC_OK != descape(user))
		throw2;

	    int err = anon_pass();
	    if (ret != 0) {
	      clean();
	      return err;
	    }

	    if(0 == *p_at) // null host?
		throw2;
	    host = (char*)malloc(1 + strlen(p_at));
	    if(NULL == host) 
		throw3;
	    strcpy(host, p_at);
	}
	if((NULL == p_at) && (NULL == p_collon)) { // no user/pass

	    user = (char*)malloc(1 + strlen("anonymous"));
	    if(NULL == user) 
		throw3;
	    strcpy(user, "anonymous"); // hardwired by RFC1738

	    int err = anon_pass();
	    if (ret != 0) {
	      clean();
	      return err;
	    }

	    host = (char*)malloc(1 + strlen(line));
	    if(NULL == host) 
		throw3;
	    strcpy(host, line);
	}

    }

    // fprintf(stderr, "user: %s; pass: %s; host: %s; path: %s; type: %c\n",
    //         user, pass, host, path, ftp_type);
    return (URLC_OK);

bomb:
    clean();
    switch (ret) {
    case 2:
      return (URLC_EPARSE);
    case 3:
      return (URLC_EALLOC);
    default:
      return (URLC_EUNKNOWN);
    }
}


/* tries to figure out what's happening in the line. 
   simple heuristics about reply code. see RFC 959
   */
int
urlc::ftp_header_interp(char* line)
{
    int ftp_new_reply;
    if((NULL == line) || (0 >= strlen(line)))
	return (URLC_ERESP);
    TF_MSG(line);

    if( (isdigit(line[0]))
	&& ((0 != line[1]) && (isdigit(line[1])))
	&& ((0 != line[2]) && (isdigit(line[2])))) {
	ftp_new_reply = 100 * (line[0] - '0') + 
	                 10 * (line[1] - '0') + 
	                  1 * (line[2] - '0');
    }
    else {
	if(URLC_AGAIN == ftp_header_stat)
	    return (ftp_header_stat);
	else // header(?) line, but not in the middle of multi-line reply
	    return (URLC_ERESP);
    }

    if('-' == line[3]) { 
	if(URLC_OK == ftp_header_stat) {
	    // first line for a multi-line reply
	    ftp_last_reply = ftp_new_reply;
	}
	    ftp_header_stat = URLC_AGAIN;
	    return (ftp_header_stat);
    }
    if( (URLC_OK == ftp_header_stat) && (' ' == line[3])) // one-line reply
	ftp_last_reply = ftp_new_reply;
    if( (URLC_AGAIN == ftp_header_stat) && (' ' == line[3])) { 
        // last line in a multi-line reply
	if(ftp_new_reply != ftp_last_reply) {
	    // not the same code ending the bracket
	    // RFC959 forces offset inner codes
	    ftp_header_stat = URLC_ERESP;
	    return (ftp_header_stat);
	}
	else
	    ftp_header_stat = URLC_OK;
    }
    // reply code interpretation
    // ## for more specific or finer, work on ftp_last_reply
    switch(line[0]) {
    case '1': // Positive Preliminary Reply
    case '2': // Positive Completion reply
	return (URLC_OK);
    case '3': // Positive Intermediate reply
	return (URLC_INTERM);
    case '4': // Transient Negative Completion reply
	return (URLC_LATER);
    case '5': // Permanent Negative Completion reply
	return (URLC_ERESP);
    default:
	return (URLC_EUNKNOWN);
    }
}


/* gets reply from the socket. manipulates the reception buffer.
   knows to get multi-line replies.
   assumes buffer is has TF_BUFLEN size.
   */
int
urlc::ftp_get_reply(char* buf, int* blen)
{
    if(NULL == buf) 
	return (URLC_EEMPTY);
    int n = 0;
    int start = 0;
    int i = 0;

    while(1) {
	for(i = start; (TF_BUFLEN > i) && (i < *blen) 
		       && ('\n' != buf[i]); i++);
	if(TF_BUFLEN == i) // line too long
	    return (URLC_ERESP);
	if(i == *blen) { // no \n by now
	    start += i;
	    n = tf_read_from_socket(sockfd, buf + *blen, 
				    TF_BUFLEN - *blen);
	    if(0 == n) // EOF??
		return (URLC_ERESP);
	    if(-1 == n) {
		switch(errno) { 
		case EINTR:
		case EAGAIN:
		    continue;
		default:
		    TF_PERROR("read");
		    return (URLC_ESOCK);
		}
	    }
	    *blen += n;
	    continue;
	}
	// here if \n found
	buf[i] = 0;
	n = ftp_header_interp(buf);
	switch(n) {
	case URLC_LATER:
	case URLC_ERESP:
	    return (n);
	}
	i++;
	*blen -= i;
	memcpy(buf, buf + i, *blen);
	start = 0;
	if(URLC_AGAIN == n)
	    continue;
	if((URLC_OK == n) || (URLC_INTERM == n))
	    return (n);
    }
}


/* simple ftp client. knows only to retrieve a single binary image.
   gets data and puts them in a temp file (ofd).
   returns URLC_OK on asuccess or reason.
   */
int
urlc::get_ftp(void)
{
    ofd = -1; // preparing for desasters
    sockfd = tcpip_open(host, port);
    if(0 > sockfd)
	return (URLC_ESOCK);

    char buf[TF_BUFLEN];
    int blen = 0;
    int n = 0;

    // greetings
    n = ftp_get_reply(buf, &blen); 
    if(URLC_OK != n)
	return (n);

    // USER
    n = write3(sockfd, "USER ", 5, user, strlen(user), "\r\n", 2);
    if(URLC_OK != n)
	return (n);
    n = ftp_get_reply(buf, &blen);
    if((URLC_OK != n) && (URLC_INTERM != n))
	return (n);

    // PASS
    if(URLC_INTERM == n) {
	n = write3(sockfd, "PASS ", 5, pass, strlen(pass), "\r\n", 2);
	if(URLC_OK != n)
	    return (n);
	n = ftp_get_reply(buf, &blen);
	if(URLC_OK != n)
	    return (n);
    }

    // CWDs
    // parses path and pass to write3 slices between 
    // current position (p) and the next / (p2)
    char* p = path;
    char* p2 = NULL;
    char* pn = NULL;

    while(1) {
	p2 = strchr(p, '/');
	if(NULL == p2) // no more slashes
	    break; 
	pn = (char*)malloc(p2-p+1);
	if(NULL == pn)
	    return (URLC_EALLOC);
	strncpy(pn, p, p2 - p);
	pn[p2 - p] = 0; // braindead strncpy
	descape(pn);
	n = write3(sockfd, "CWD ", 4, pn, strlen(pn), "\r\n", 2);
	free(pn);
	if(URLC_OK != n) 
	    return (n);
	n = ftp_get_reply(buf, &blen);
	if(URLC_OK != n)
	    return (n);
	p = p2 + 1; // prepares for next round
    }
    
    // TYPE 
    n = write3(sockfd, "TYPE ", 5, &ftp_type, 1, "\r\n", 2);
    if(URLC_OK != n)
	return (n);
    n = ftp_get_reply(buf, &blen);
    if(URLC_OK != n)
	return (n);

    // PORT. the trickiest part. really!
    // we must send bytes in decimal for local IP addr and listening port 
    // "ip3,ip2,ip1,ip0,p1,p0"
    struct hostent* hp;
    struct utsname unp;
    n = uname(&unp);
    if(0 > n) /* braindead Solaris, returns >0 if OK. POSIX says 0 */
	return (URLC_EINVAL);
    hp = gethostbyname(unp.nodename);
    if(NULL == hp)
	return (URLC_EINVAL);
    char port_val[25] = ""; // space for constructing the PORT parameter
    for(n = 0; n < 25; n++)
	port_val[n] = 0; // preventive fill

    // use the sin_addr obtained by tcpip_open,
    // so it works for multi-interface boxes
    strcpy(port_val, inet_ntoa(lin.sin_addr)); // local address
    for(n = 0; 0 != port_val[n]; n++) { // commas instead of dots
	if('.' == port_val[n])
	    port_val[n] = ',';
    }
    int sockfd2 = -1; // socket for listening 
    struct sockaddr_in local_addr;
    int local_addr_len = sizeof(local_addr);
    struct sockaddr_in rem_addr;
    int rem_addr_len = sizeof(rem_addr);

    // we assume that the kernel is not stupid 
    sockfd2 = socket(PF_INET, SOCK_STREAM, 0);
    if(0 > sockfd2)
	return (URLC_ESOCK);
    memset((char*)&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = PF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = 0;
    n = bind(sockfd2, (struct sockaddr*)&local_addr, sizeof(local_addr));
    if(0 > n) {
	TF_PERROR("bind");
	return (URLC_ESOCK);
    }
    getsockname(sockfd2, (struct sockaddr*)&local_addr, &local_addr_len);
    n = listen(sockfd2, 1);
    if(0 > n)
	return (URLC_ESOCK);

    // wise enough, htons, isn't it? it swaps, I don't care 
    // Intel is little-^H^H^H^H^H^H small-endian
    sprintf(port_val, "%s,%d,%d", port_val, 
	    (unsigned short)((htons(local_addr.sin_port) >> 8) & 0xff),
	    (unsigned short)(htons(local_addr.sin_port) & 0xff));
    TF_MSG(port_val);
    n = write3(sockfd, "PORT ", 5, port_val, strlen(port_val), "\r\n", 2);
    if(URLC_OK != n) 
	return (URLC_ESOCK);
    n = ftp_get_reply(buf, &blen);
    if(URLC_OK != n)
	return (URLC_ERESP);
    // RETR
    n = write3(sockfd, "RETR ", 5, p, strlen(p), "\r\n", 2);
    if(URLC_OK != n)
	return (n);
    n = ftp_get_reply(buf, &blen);
    if(URLC_OK != n)
	return (n);

    // ## blocks on accept
    int newsockfd = -1;
    newsockfd = accept(sockfd2, (struct sockaddr*) &rem_addr, 
		       &rem_addr_len);
    if(-1 == newsockfd) {
	TF_PERROR("accept");
	return (URLC_ESOCK);
    }

    // verify if it comes from whom we wanted to come
    struct sockaddr_in pcin; // peer control connection address
    int pcin_len = sizeof(pcin);
    // fills from control connection
    n = getpeername(sockfd, (struct sockaddr*) &pcin, &pcin_len);
    if(-1 == n) // strange, weird
	return (URLC_ESOCK);
    if( (pcin.sin_addr.s_addr != rem_addr.sin_addr.s_addr) 
	|| (20 != htons(rem_addr.sin_port))) {
	TF_MSG("ftp data connection attack.");
	return (URLC_EAUTH);
    }

    if(-1 == fcntl(newsockfd, F_SETFL, O_NONBLOCK))
	TF_PERROR("fcntl");

    ofd = tmp_file_open(unlink_mode);
    if(0 > ofd)
	return (ofd);

    while(1) { 
	n = write(ofd, buf, blen);
	if(-1 == n) {
	    switch(errno) {
	    case EINTR:
		continue;
	    default:
		close(ofd);
		close(sockfd2);
		close(newsockfd);
		ofd = -1;
		return (URLC_EFILE);
	    }
	}
	blen -= n;
	if(0 < n)
	    continue; // try again to write
	n = tf_read_from_socket(newsockfd, buf, TF_BUFLEN);
	if(0 == n)
	    break;
	if(-1 == n) {
	    switch(errno) {
	    case EINTR:
	    case EAGAIN:
		continue;
	    default:
		return (URLC_ESOCK);
	    }
	}
	blen = n;
    }
    close(newsockfd);
    close(sockfd2);

    // QUIT. be polite, kid, say bye-bye! to miss.
    n = write3(sockfd, "QUIT ", 5, NULL, 0, NULL, 0);
    if(URLC_OK != n)
	return (n);
    n = ftp_get_reply(buf, &blen);
    if(URLC_OK != n)
	return (URLC_ERESP); // too late, but to be consistent
    close(sockfd);

    return (URLC_OK);
}

/* it assumes a standard HTTP/1.0 query in the format:
   http://host[:port][/[path]] as in RFC1738.
   (query embedded in path)
   returns URLC_OK on successful parse, reason otherwise.
   */
int
urlc::parse_http(const char* line)
{
    char* p_collon = NULL; /* positions in line */
    char* p_slash = NULL;

    if(NULL != path) {
	free(path);
	path = NULL;
    }
    if(NULL != host) {
	free(host);
	host = NULL;
    }

    if((NULL == line) || (0 == *line)) // emtpy line?
	return (URLC_EEMPTY);
    p_collon = strchr(line, ':');
    p_slash = strchr(line, '/');
    
    // sanity checks
    if((NULL != p_collon) && (0 == p_collon[1])) // nothing after :
	return (URLC_EINVAL);
    if((NULL != p_slash) && (NULL != p_collon)) { 
	if(p_slash < p_collon) // / before :
	    return (URLC_EINVAL);
	if(p_slash == 1+p_collon) // :/
	    return (URLC_EINVAL);
    }
    
    if(NULL != p_slash) {
	*p_slash = 0;
	p_slash++;
	if(0 == *p_slash) 
	    p_slash = NULL;
    }
    if(NULL != p_collon) {
	*p_collon = 0;
	p_collon++;
	if(0 == *p_collon)
	    p_collon = NULL;
    }

	host = (char*)malloc(1+strlen(line));
	if(NULL == host) 
	    return (URLC_EALLOC);
	strcpy(host, line);

     int ret = 0;
     {
	if(NULL == p_collon)
	    port = 80;
	else {
	    long i = 0;
	    i = strtol(p_collon, NULL, 10); // ANSI 
	    if((0 >= i) || (USHRT_MAX < i)) 
		throw2;
	    port = (unsigned short)i;
	}

	if(NULL == p_slash) {
	    path = (char*)malloc(2);
	    if(NULL == path) 
		throw5;
	    path[0] = '/';
	    path[1] = 0;
	    return (URLC_OK);
	}
	path = (char*)malloc(1+1+(3*strlen(p_slash))); 
	// /, string with escapes , \0 
	if(NULL == path) 
	    throw5;
	path[0] = '/';
	char* pp = path+1;
	int j;
	char he[] = "0123456789abcdef";
	for(j = 0; 0 != p_slash[j]; j++, pp++) {
	    if((isalnum(p_slash[j])) || 
	       (NULL != strchr(URLC_hs, p_slash[j])))  {
		*pp = p_slash[j];
	    }
	    else {
		*pp = '%';
		pp++;
		*pp = he[(int)((p_slash[j]>>4) & 0x0f)];
		pp++;
		*pp = he[(int)(p_slash[j] & 0x0f)];
	    }
	}
	*pp = 0;
    }

     return (URLC_OK);

bomb:
     clean();
     switch (ret) {
     case 2:
       return (URLC_EINVAL);
     case 5:
       return (URLC_EALLOC);
    default:
      return (URLC_EUNKNOWN);
     }
}


/* makes the HTTP Request (in HTTP/1.0 format).
   returns URLC_OK on success, reason otherwise.
   unfortunately, writev (gather write) is not a POSIX standard.
   also, writev depends on the size of the first mbuf to accept all
   the headers in a single call.
 */
int
urlc::http_req(void)
{
    int n;
    int tot_len = 0;
    char* p = NULL;
    char* req_form[] = { "GET ", path, " HTTP/1.0\r\n",
			 "User-Agent: ", HTTP_USER_AGENT, "\r\n", 
			 "From: tf@info.ucl.ac.be\r\n", 
			 "\r\n", 
			 NULL
    };

    // compute total length
    for(n = 0; NULL != req_form[n]; n++) 
	tot_len += strlen(req_form[n]);
    p = (char*)malloc(tot_len+1); // +1 for \0
    if(NULL == p) // alloc problems
	return (URLC_EALLOC);
    p[0] = 0; // ""
    for(n = 0; NULL != req_form[n]; n++) {
	strcat(p, req_form[n]);
	TF_MSG(req_form[n]);
    }
    if(URLC_OK != writen(sockfd, p, tot_len)) {
	    free(p);
	    return (URLC_ESOCK);
    }

    return (URLC_OK);
}


/* interprets an HTTP header line.
   returns URLC_OK on success, reason otherwise. 
 */
int
urlc::http_header_interp(char* line, int linenr)
{
    TF_MSG(line);
    if(0 == linenr) { // first line
	char* l = line;
	char http_ver[] = "HTTP/";
	
	if(line != strstr(line, http_ver)) // malformed first line
	    return (URLC_ERESP);
	while((0 != *l) && (!isspace(*l))) // skip protocol version
	    l++;
	while((0 != *l) && (isspace(*l))) // skip spaces
	    l++;
	if((0 == *l) || (!isdigit((int)(*l)))) // we want code
	    return (URLC_ERESP);
	switch(*l) { // detect status code class 
	case '2': // successful
	    break;
	case '3': // redirection
	    http_header_stat = URLC_REDIRECT;
	    return (URLC_OK);
	case '1': // informational
	case '4': // client error
	case '5': // server error
	default:  // human error, undoubtly!
	    return (URLC_ERESP);
	}
	return (URLC_OK);
    }

    char* fv = NULL;
    
    for(fv = line; (0 != fv) && (!isspace(*fv)) && (':' != (*fv)); fv++);
    if(0 == (*fv)) // line too short, no :
	return (URLC_ERESP);
    if(isspace(*fv)) // missing : 
	    return (URLC_ERESP);
    fv++;
    while((0 != (*fv)) && (isspace(*fv))) // skip spaces until field value
	fv++;
    if(0 == (*fv)) // no field value
	return (URLC_ERESP);
    // at line starts the field name 
    // fv points to field value
    
    if((URLC_REDIRECT == http_header_stat) && 
       (line == strstr(line, "Location:"))) { // redirect
	if(NULL != http_redirect) {
	    free(http_redirect);
	    http_redirect = NULL;
	}
	http_redirect = (char*)malloc(strlen(fv)+1);
	if(NULL == http_redirect)
	    return (URLC_EALLOC);
	strcpy(http_redirect, fv);
	return (URLC_REDIRECT);
    }

    return (URLC_OK);
}


/* gets HTTP reply.
   assumes buffer has TF_BUFLEN size
   */
int
urlc::http_get_header(char* buf, int* brem, int& n)
{
    int n1 = 0;
    char* p = buf;
    int linenr = 0; // header line number
    int i = 0;

    while(1) { // start read headers
	errno = 0;
	n1 = tf_read_from_socket(sockfd, p, *brem);
	if(0 == n1) // EOF??
	    return (URLC_ERESP);
	if(-1 == n1) {
	    switch(errno) {
	    case EINTR:
	    case EAGAIN:
		continue;
	    default:
		TF_PERROR("read");
		return (URLC_ESOCK);
	    }
	}
	n += n1;
	do {
	    if('\n' == buf[0]) { // end of headers by empty line
		n--;
		memmove(buf, buf+1, n); // expensive, by now
		return(URLC_OK);
	    }
	    if(('\r' == buf[0]) && ('\n' == buf[1])) { 
		n -= 2;
		memmove(buf, buf+2, n);
		return (URLC_OK);
	    }
	    for(i = 0; (TF_BUFLEN > i) && (i < n) && 
		    ('\n' != buf[i]); i++);
	    if(TF_BUFLEN == i) // we reached buffer's end
	      return (URLC_EALLOC);
	    if(i == n) { // no \n until now
		*brem = TF_BUFLEN-n;
		p = buf+n;
		break; // try to append to existing chars
	    }
	    buf[i] = 0;
	    if((0 < i) && ('\r' == buf[i-1]))
		buf[i-1] = 0;
	    i++;
	    n -= i;
	    p = buf+i;
	    if(URLC_REDIRECT == http_header_interp(buf, linenr)) 
		return (URLC_REDIRECT);
	    memmove(buf, p, n);
	    p = buf+n;
	    *brem = TF_BUFLEN-n;
	    linenr++;
	} while(0 < n); 
    }
    
    return (URLC_OK);
}


/* small HTTP client. 
   gets the data and put them in a temp file (ofd).
   returns URLC_OK on success, reason on error. 
 */
int
urlc::get_http(void)
{
    int n = 0;
    int n2 = 0;
    int brem  = TF_BUFLEN;
    char buf[TF_BUFLEN] = "";

    sockfd = tcpip_open(host, port);
    if(0 > sockfd)
	return (URLC_ESOCK);
    n = http_req();
    if(URLC_OK != n)
	return (n);
    n = http_get_header(buf, &brem, n2);
    if(URLC_OK != n) {
	ofd = -1;
	close(sockfd);
	return (n);
    }
    ofd = tmp_file_open(unlink_mode);
    if(0 > ofd) {
	close(sockfd);
	return (ofd);
    }

    int ret=0;
    {
	while(1) { // start read contents
	    errno = 0;
	    if((0 < n2) && 
	       (n2 != write(ofd, buf, n2))) {
		TF_PERROR("write");
		throw9;
	    }
            errno = 0;
	    n2 = tf_read_from_socket(sockfd, buf, TF_BUFLEN);
	    if(0 == n2) // EOF
		break;
	    if(-1 == n2) {
		switch(errno) {
		case EINTR:
		case EAGAIN: // == EWOULDBLOCK(?)
		    continue;
		default:
		    TF_PERROR("read");
		    throw6;
		}
	    }
	}
    }

    close(sockfd);
    return (URLC_OK);

bomb:
    while(1) {
      errno = 0;
      if(-1 == close(sockfd))
	if(EINTR == errno)
	  continue;
	else break;
    }
    ofd = -1;
    switch (ret) {
    case 6:
      return (URLC_ESOCK);
    case 9:
      return (URLC_EFILE);
    default:
      return (URLC_EUNKNOWN);
    }
}


/* top-level access function to get a URL content into a file.
   returns a valid file descriptor on success or 
   negative error status (URLC_*).
 */
int
urlc::getURL(const char* line, int unlink_m)
{
    int n = 0;
    int t = 0;

    while(URLC_REDTIMES > t) { // no more than five redirects, says HTTP
	if(0 == t) // first time
	    n = parse(line);
	else
	    n = parse(http_redirect);
	if(URLC_OK != n) 
	    return (URLC_EPARSE);
	ofd = -1;
	
	if(URLC_UNLINK == unlink_m)
	    unlink_mode = URLC_UNLINK;
	else
	    unlink_mode = 0;

	if(0 == strcmp("file:", proto)) 
	    n = get_file();
	if(0 == strcmp("http://", proto))
	    n = get_http();
	if(0 == strcmp("ftp://", proto))
	    n = get_ftp();
	/* other protocols tested here */
	/* ##extension */

	switch(n) {
	case URLC_REDIRECT:
	    t++;
	    continue;
	case URLC_OK:
	    clean();
	    lseek(ofd, 0, SEEK_SET);
	    return (ofd);
	default:
	    clean();
	    ofd = -1;
	    return (n);
	}
    }
}


int
urlc::openURL(const char* line)
{
    return (getURL(line, URLC_UNLINK));
}


int
urlc::localizeURL(const char* line, char** fnp)
{
    int fd = -1;

    fd = getURL(line, URLC_OK);
    if(0 > fd) // some error
	return (fd);
    close(fd);
    *fnp = temp_file_name;

    return (URLC_OK);
}


/* ##Extending the URL client (search also "##extension" comments):
   1. add to static URLC_kp a string containing the URL prefix 
      indicating the new scheme;
   2. write a "int parse_<scheme>(const char* url);" function returning 
      URLC_OK for successing parsing, which prepares class data members 
      for later. if in need, add new members to the class;
   3. write a "int get_<scheme>(void);" function returning URLC_OK if
      getting ok.  put in ofd member a (int) file descriptor pointing to 
      the file containing data;
   4. in the urlc::parse() add a new "if" test for the new scheme;
   5. in the urlc::getURL() add a new "if" test for th  new scheme;
   6. compile it with:
      g++ -ansi -pedantic -W -Wall -fhandle-exceptions \
          -I/usr/include/g++ -c urlc.cc -o urlc.o
   6. test it;
   7. enjoy it.
   */


/* ---------------- the aim of all this stuff ------------------- */
int openUrl(const char* Url)
{
    urlc s;

    return (s.openURL(Url));
}


int localizeUrl(const char* Url, char** fnp)
{
    urlc s;
    
    return (s.localizeURL(Url, fnp));
}
