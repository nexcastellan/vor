#ifndef _HTTP_CLIENT_H_
#define _HTTP_CLIENT_H_

#include <stdexcept>
#include <string>

// Implement ability to send an http request to a remote server,
// and get a response.  This is very simple and designed solely
// to grab user information from the ruby side.
class HttpClient {
public:
	static std::string request(const std::string& url);
	~HttpClient();

private:
	static std::string handle_request(const std::string& url);

	// Read response data from url.  Helper function for handle_request.
	static size_t read_response(void *ptr, size_t size, size_t nmemb, void *stream);

private:
	static const long URL_TIMEOUT = 60;

private:
	class HttpClientException: public std::runtime_error {
	public:
		HttpClientException(const int new_errno = 0);
		int err;
	};
	// Invalid url, hostname, or port specified.
	class HttpClientHostnameException : public HttpClientException {};
};

#endif
