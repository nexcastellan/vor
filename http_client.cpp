#include "http_client.h"

#include <curl/curl.h>
#include <curl/easy.h>
#include <curl/types.h>
#include <iostream>

#include "program_options.h"

struct MemoryStruct {
  	char *memory;
  	size_t size;
};

HttpClient::HttpClientException::HttpClientException(const int new_err) :
	std::runtime_error("HttpClientException"), err(new_err)
{}

HttpClient::~HttpClient() {
}

std::string
HttpClient::request(const std::string& url) {
	unsigned int retries = 0;
	while (retries < 3) {
		try {
			return handle_request(url);
			break;
		} catch(HttpClientHostnameException e) {
			std::cout << "hostname exception" << std::endl;
			throw e;
		} catch(HttpClientException e) {
			++retries;
			std::cout << "Failed " << url << ", try " << retries << std::endl;
			// Back off a bit
			usleep(retries * retries * 10 * 1000);
		}
	}
	return "";
}

std::string
HttpClient::handle_request(const std::string& url) {
	if (url.find("http://") != 0) {
		throw HttpClientHostnameException();
	}
	
	std::string retval;
	
	//parse the host and uri
	std::string host = url.substr(7); // skip the "http://" bit
	size_t separator = host.find('/');
	if ((separator == std::string::npos) || (separator == 0)) {
		throw HttpClientHostnameException();
	}
 	// "www.nexopia.com/userinfo" => "userinfo"
	std::string rest = host.substr(separator);
	// "www.nexopia.com/userinfo" => "www.nexopia.com"
	host = host.substr(0, separator);
	
	// Do the request
	CURLcode res;
	CURL *curl = curl_easy_init();
	if (curl) {
		struct MemoryStruct chunk;
		chunk.memory = 0;
		chunk.size = 0;
		
		char error_buffer[CURL_ERROR_SIZE + 1]; // Store errors here
		
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, URL_TIMEOUT);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &read_response);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, static_cast<void *>(&chunk));
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "vor-agent");
		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);
		
		res = curl_easy_perform(curl);
		if (res != 0) {
			std::cout << "curl error on url " << url << ": " << error_buffer;
			std::cout << std::endl;
			throw HttpClientException(res);
		}
		
		if (chunk.memory != 0) {
			retval = chunk.memory; // Copy data out
			free(chunk.memory);
		}
		
		curl_easy_cleanup(curl);
	}
	
	return retval;
}

size_t
HttpClient::read_response(void *ptr, size_t size, size_t nmemb, void *data) {
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)data;

	mem->memory = static_cast<char *>(realloc(mem->memory, mem->size + realsize + 1));
	if (mem->memory) {
		memcpy(&(mem->memory[mem->size]), ptr, realsize);
		mem->size += realsize;
		mem->memory[mem->size] = 0;
	}
	return realsize;

}

