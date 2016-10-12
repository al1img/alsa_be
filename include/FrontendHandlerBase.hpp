/*
 * FrontendHandlerBase.h
 *
 *  Created on: Oct 11, 2016
 *      Author: al1
 */

#ifndef INCLUDE_FRONTENDHANDLERBASE_HPP_
#define INCLUDE_FRONTENDHANDLERBASE_HPP_

#include <exception>
#include <string>

class BackendBase;

class FrontendHandlerException : public std::exception
{
public:
	FrontendHandlerException(const std::string& msg) : mMsg(msg) {};

	const char* what() const throw() { return mMsg.c_str(); };

private:
	std::string mMsg;
};

class FrontendHandlerBase
{
public:
	FrontendHandlerBase(int domId, const BackendBase& backend);
	virtual ~FrontendHandlerBase();

private:
	int mDomId;
	const BackendBase& mBackend;

	std::string mXsBackendPath;
	std::string mXsFrontendPath;
	std::string mXsRemovePath;

	const std::string getXsBackendPath();
	const std::string getXsFrontendPath();
	const std::string getXsRemovePath();
};


#endif /* INCLUDE_FRONTENDHANDLERBASE_HPP_ */
