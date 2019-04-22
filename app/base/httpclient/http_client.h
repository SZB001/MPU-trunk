#ifndef _HTTP_H_
#define _HTTP_H_

#define HTTP_API

#ifdef _cplusplus
extern "C" {
#endif

#define FT_SUPPORT_HTTPS 1
//#define USEOPENSSL

typedef enum http_request_method_e 
{ 
	M_GET = 0, 
	M_POST, 
	M_HEAD 
} http_request_method_e;

enum http_error_e
{
	ERR_OK = 0,

	ERR_INVALID_PARAM,
	ERR_OUT_MEMORY,
	ERR_OPEN_FILE,

	ERR_PARSE_REP,

	ERR_URL_INVALID,
	ERR_URL_INVALID_PROTO,
	ERR_URL_INVALID_HOST,
	ERR_URL_INVALID_IP,
	ERR_URL_RESOLVED_HOST,

	ERR_SOCKET_CREATE,
	ERR_SOCKET_SET_OPT,
	ERR_SOCKET_NOBLOCKING,
	ERR_SOCKET_CONNECT,
	ERR_SOCKET_CONNECT_TIMEOUT,
	ERR_SOCKET_SELECT,
	ERR_SOCKET_WRITE,
	ERR_SOCKET_READ,
	ERR_SOCKET_TIMEOUT,
	ERR_SOCKET_CLOSED,
	ERR_SOCKET_GET_OPT,

#if FT_SUPPORT_HTTPS
	ERR_SSL_CREATE_CTX,
	ERR_SSL_CREATE_SSL,
	ERR_SSL_SET_FD,
	ERR_SSL_CONNECT,
	ERR_SSL_WRITE,
	ERR_SSL_READ
#endif

};


#ifdef WIN32
	#include <WinSock2.h>
	#ifdef WINCE
		#pragma comment( lib, "ws2.lib") 
	#else
		#pragma comment( lib, "ws2_32.lib") 
	#endif
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <fcntl.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <unistd.h>
	#include <errno.h>
	#include <string.h>
	#include <sys/types.h>
	#include <sys/select.h>
	#include <time.h> 
	#include <ctype.h>
	#include <netdb.h>
	#include <strings.h>

	#define _stricmp    strcasecmp
	#define _strnicmp   strncasecmp    
    #define _strdup     strdup

#endif


#if FT_SUPPORT_HTTPS

#ifdef USEOPENSSL

#include <openssl/ssl.h>
#include <openssl/err.h>

#else 
#include "krypton.h"
#endif


#endif


#define DEFAULT_USER_AGENT_STR "User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64; rv:29.0) Gecko/20100101 Firefox/29.0\r\n"
#define CONNECT_STR "Connection: close\r\n"
#define ACCEPT_STR "Accept: */*\r\n"
#define CONTENT_LENGTH_STR "Content-Length"
#define CONTENT_TYPE_STR "Content-Type: application/x-www-form-urlencoded\r\n"
#define CONTENT_DISPOSITION_STR "Content-Disposition"
#define CRLF "\r\n"

enum parser_statue_e { PARSERD_NONE = 0, PARSERD_FIELD, PARSERD_VALUE, PARSERD_BODY };

enum proto_type_e { PROTO_HTTP = 0, PROTO_HTTPS };

#ifdef WIN32
	typedef SOCKET socket_t;
	#define HTTP_INVALID_SOCKET INVALID_SOCKET
	#define HTTP_EINTR WSAEINTR
	#define HTTP_EINPROGRESS WSAEINPROGRESS
	#define HTTP_EWOULDBLOCK WSAEWOULDBLOCK
	#define HTTP_EALREADY WSAEALREADY
#else
	typedef int socket_t;
	#define HTTP_INVALID_SOCKET -1
	#define HTTP_EINTR EINTR
	#define HTTP_EINPROGRESS EINPROGRESS
	#define HTTP_EWOULDBLOCK EWOULDBLOCK
	#define HTTP_EALREADY EALREADY
#endif

#define RECV_BUF_SIZE 4 * 1024


#ifdef WIN32
	#define socket_close closesocket
#else
	#define socket_close close
#endif

#define free_member(member) if((member)) { free(member); (member) = NULL; }
#define close_socket(fd) if(fd != HTTP_INVALID_SOCKET) { socket_close(fd); fd = HTTP_INVALID_SOCKET; }
#define close_file(pf) if(pf != NULL) { fclose(pf); pf = NULL; }

typedef int (*data_recv_cb_t)( int status, int received, int total, void* user);
typedef struct
{
	FILE* pf;
	char* filename;
	char* body;
	char* redirect_url;
	char* header_field;
	char* header_value;

	char* user;
	data_recv_cb_t recv_cb;
	
	unsigned long body_len;
	unsigned long content_length;
    unsigned long receive_length;
    unsigned long total_length;

	enum http_request_method_e method;
	enum proto_type_e proto_type;

	unsigned short field_size;
	unsigned short value_size;
	unsigned short cur_field_size;
	unsigned short cur_value_size;

#if FT_SUPPORT_HTTPS
	SSL_CTX *ctx;
	SSL *ssl;
#endif

	socket_t fd; 
	int timeout;
	
	short status_code;
	char parser_statue;
	char error_code;
	unsigned cancel	  : 1;
	unsigned exit	  : 1;
	unsigned download : 1;
	unsigned redirect : 1;
}ft_http_client_t;



HTTP_API int ft_http_init();
HTTP_API void ft_http_deinit();
HTTP_API ft_http_client_t* ft_http_new();
HTTP_API void ft_http_destroy(ft_http_client_t* http);
HTTP_API int ft_http_get_error_code(ft_http_client_t* http);
HTTP_API int ft_http_get_status_code(ft_http_client_t* http);
HTTP_API int ft_http_set_timeout(ft_http_client_t* http, int timeout);
HTTP_API const char* ft_http_sync_request(ft_http_client_t* http, const char *url);
HTTP_API const char* ft_http_sync_post(ft_http_client_t *http, const char *url, const char *data, int len);

HTTP_API int ft_http_sync_download_file( ft_http_client_t* http, const char* url, const char* filepath);

HTTP_API int ft_http_cancel_request(ft_http_client_t* http);

HTTP_API int ft_http_wait_done(ft_http_client_t* http);

HTTP_API int ft_http_set_data_recv_cb(ft_http_client_t* http, data_recv_cb_t cb, void* user);

HTTP_API int ft_http_exit(ft_http_client_t* http);

HTTP_API const char* ft_http_sync_post_file(ft_http_client_t* http, const char* url, const char* filepath);



#ifdef _cplusplus
}
#endif


#endif

