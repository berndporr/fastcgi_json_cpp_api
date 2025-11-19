#ifndef FAST_CGI_H
#define FAST_CGI_H

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <iostream>
#include <fcgio.h>
#include <thread>
#include <string>

/**
 * C++ wrapper around fastCGI which sends and receives JSON
 * in a jQuery friendly format.
 *
 * Copyright (C) 2021-2023  Bernd Porr <mail@berndporr.me.uk>
 * Apache License 2.0
 **/
class JSONCGIHandler {
public:
	JSONCGIHandler() = default;
	
	/**
	 * GET callback handler which needs to be implemented by the main
	 * program. This needs to provide the JSON payload.
	 **/
	class GETCallback {
	public:
		/**
		 * Needs to return the JSON data sent to the web browser.
		 * \return JSON data
		 **/
		virtual std::string getJSONString() = 0;
		/**
		 * The content type of the payload. That's by default
		 * "application/json" but can be overloaded if needed.
		 * \return MIME type
		 **/
		virtual std::string getContentType() { return "application/json"; }
	};


	/**
	 * Callback handler which needs to be implemented by the main
	 * program.
	 **/
	class POSTCallback {
	public:
		/**
		 * Receives the POST JSON data from the web browser.
		 * \param postArg POST data received from jQuery
		 **/
		virtual void postString(std::string postArg) = 0;
	};
	
	/**
	 * Opens the connection and starts the main thread.
	 * \param argGetCallback Callback handler for sending JSON
	 * \param argPostCallback Callback handler for receiving JSON
	 * \param socketpath Path of the socket which communicates to the webserver
	 **/
	void start(
		GETCallback* argGetCallback,
		POSTCallback* argPostCallback = nullptr,
		const char socketpath[] = "/tmp/fastcgisocket") {
		getCallback = argGetCallback;
		postCallback = argPostCallback;
		// set it to zero
		memset(&request, 0, sizeof(FCGX_Request));
		// init the connection
		FCGX_Init();
		// open the socket
		sock_fd = FCGX_OpenSocket(socketpath, 1024);
		if (sock_fd < 0) {
			std::string s = "Could not open socket. Error: "+sock_fd;
			throw s.c_str();
		}
		// making sure the nginx process can read/write to it
		chmod(socketpath, S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH);
		// init requests so that we can accept requests
		FCGX_InitRequest(&request, sock_fd, 0);
		// starting main loop
		mainThread = std::thread(&JSONCGIHandler::exec, this);
	}

	/**
	 * Shuts down the connection to the webserver and
	 * it also terminates the thread which is waiting for requests.
	 **/
	void stop() {
		if (!running) return;
		running = false;
		shutdown(sock_fd, SHUT_RDWR);
		FCGX_Free(&request, sock_fd);
		mainThread.join();
	}

	~JSONCGIHandler() {
		stop();
	}

 private:
	void exec() {
		running = true;
		while (running && (FCGX_Accept_r(&request) == 0)) {
			char * method = FCGX_GetParam("REQUEST_METHOD", request.envp);
			if (method == nullptr) {
				fprintf(stderr,"Please add 'include fastcgi_params;' to the nginx conf.\n");
				throw "JSONCGI parameters missing.\n";
			}
			if (strcmp(method, "GET") == 0) {
				// create the header
				std::string buffer = "Content-type: " + getCallback->getContentType();
				buffer = buffer + "; charset=utf-8\r\n";
				buffer = buffer + "\r\n";
				// append the data
				buffer = buffer + getCallback->getJSONString();
				buffer = buffer + "\r\n";
				// send the data to the web server
				FCGX_PutStr(buffer.c_str(), buffer.length(), request.out);
				FCGX_Finish_r(&request);
			}
			if ( (nullptr != postCallback) && (strcmp(method, "POST") == 0) ) {
			    long reqLen = 1;
			    char * content_length_str = FCGX_GetParam("CONTENT_LENGTH",
								      request.envp);
			    if (content_length_str) reqLen = atol(content_length_str)+1;
			    char* tmp = new char[reqLen];
			    FCGX_GetStr(tmp,reqLen,request.in);
			    tmp[reqLen - 1] = 0;
			    if (nullptr != postCallback) {
				postCallback->postString(tmp);
			    }
			    delete[] tmp;
			    // create the header
			    std::string buffer = "Content-type: text/html";
			    buffer = buffer + "; charset=utf-8\r\n";
			    buffer = buffer + "\r\n";
			    // append the data
			    buffer = buffer + "\r\n";
			    buffer = buffer + "<html></html>\r\n";
			    // send the data to the web server
			    FCGX_PutStr(buffer.c_str(), buffer.length(), request.out);
			    FCGX_Finish_r(&request);
			}
		}
	}

 private:
	FCGX_Request request;
	int sock_fd = 0;
	bool running = false;
	std::thread mainThread;
	GETCallback* getCallback = nullptr;
	POSTCallback* postCallback = nullptr;
};

#endif
